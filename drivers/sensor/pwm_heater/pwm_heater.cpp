// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#define DT_DRV_COMPAT pwm_heater

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "OF/drivers/sensor/pwm_heater.h"
#include "OF/lib/utils/PID.hpp"

LOG_MODULE_REGISTER(pwm_heater, CONFIG_PWM_HEATER_LOG_LEVEL);

struct pwm_heater_config {
    pwm_dt_spec pwm;
	const device *temp_sensor;
    sensor_channel temp_channel;
};

struct pwm_heater_data {
	struct k_work_delayable work{};
	OF::PIDController<OF::Positional, float, OF::WithIntegralLimit, OF::WithOutputLimit> pid;
	int32_t target_temp{};      /* 目标温度 (单位: 摄氏度x100) */
	int32_t current_temp{};     /* 当前温度 (单位: 摄氏度x100) */
	uint32_t pwm_period_us{};   /* PWM周期 (微秒) */
	uint32_t duty_cycle_us{};   /* 当前占空比 (微秒) */
	uint32_t max_duty_us{};     /* 最大占空比 (微秒) */
	bool enabled{};             /* 加热器使能状态 */

	pwm_heater_data() :
		pid(CONFIG_PWM_HEATER_PID_KP / 1000.0f,
		    CONFIG_PWM_HEATER_PID_KI / 1000.0f,
		    CONFIG_PWM_HEATER_PID_KD / 1000.0f,
		    1000.0f,     // 输出限幅在1000范围内
		    std::nullopt,
		    1000000.0f)  // 积分限幅
	{}
};

static void pwm_heater_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct pwm_heater_data *data = CONTAINER_OF(dwork, struct pwm_heater_data, work);
	const struct device *dev = CONTAINER_OF(data, struct device, data);
    const auto config = static_cast<const pwm_heater_config*>(dev->config);
    sensor_value temp_val{};
	float pid_output;
	uint32_t new_duty;

    /* 读取当前温度 */
    if (const int ret = sensor_channel_get(config->temp_sensor, config->temp_channel, &temp_val); ret < 0) {
		LOG_ERR("无法读取温度传感器: %d", ret);
		goto reschedule;
	}

	/* 转换为摄氏度x100 */
	data->current_temp = temp_val.val1 * 100 + temp_val.val2 / 10000;

	LOG_DBG("当前温度: %d.%02d°C, 目标: %d.%02d°C",
		data->current_temp / 100, data->current_temp % 100,
		data->target_temp / 100, data->target_temp % 100);

	if (!data->enabled) {
		/* 加热器禁用时，设置PWM为0 */
		pwm_set_dt(&config->pwm, data->pwm_period_us, 0);
		goto reschedule;
	}

	/* 使用PID控制器计算输出 */
	pid_output = data->pid.compute(static_cast<float>(data->target_temp) / 100.0f, static_cast<float>(data->current_temp) / 100.0f);

	/* 将PID输出转换为PWM占空比 */
	if (pid_output < 0) {
		/* 温度过高，停止加热 */
		new_duty = 0;
	} else {
		/* 将PID输出(0-1000)映射到PWM占空比(0-max_duty_us) */
		new_duty = (static_cast<uint32_t>(pid_output) * data->max_duty_us) / 1000;
	}

	if (new_duty != data->duty_cycle_us) {
		LOG_DBG("调整PWM占空比: %u/%u us", new_duty, data->pwm_period_us);
		data->duty_cycle_us = new_duty;
		pwm_set_dt(&config->pwm, data->pwm_period_us, new_duty);
	}

reschedule:
	k_work_schedule(&data->work, K_MSEC(CONFIG_PWM_HEATER_UPDATE_INTERVAL_MS));
}

static int pwm_heater_init(const device *dev)
{
    const auto config = static_cast<const pwm_heater_config*>(dev->config);
    const auto data = static_cast<pwm_heater_data*>(dev->data);

    if (!device_is_ready(config->pwm.dev)) {
		LOG_ERR("PWM设备未就绪");
		return -ENODEV;
	}

	if (!device_is_ready(config->temp_sensor)) {
		LOG_ERR("温度传感器未就绪");
		return -ENODEV;
	}

	/* 直接使用Kconfig设置所有参数 */
	data->target_temp = CONFIG_PWM_HEATER_TARGET_TEMP;
	data->current_temp = 0;
	data->pwm_period_us = 100;
	data->duty_cycle_us = 0;
	data->max_duty_us = (data->pwm_period_us * CONFIG_PWM_HEATER_MAX_DUTY_CYCLE_PCT) / 100;
	data->enabled = false;  // 默认初始状态为禁用

	/* 重置PID控制器 */
	data->pid.reset();

	/* 初始化工作队列 */
	k_work_init_delayable(&data->work, pwm_heater_work_handler);

	/* 初始化PWM引脚 */
    if (const int ret = pwm_set_dt(&config->pwm, data->pwm_period_us, 0); ret < 0) {
		LOG_ERR("无法设置PWM输出: %d", ret);
		return ret;
	}

	/* 启动控制循环 */
	k_work_schedule(&data->work, K_MSEC(CONFIG_PWM_HEATER_UPDATE_INTERVAL_MS));

	LOG_INF("PWM加热器初始化完成，目标温度: %d.%02d°C",
		data->target_temp / 100, data->target_temp % 100);

	return 0;
}

/* 实现API函数 */
int pwm_heater_get_current_temp(const device *dev, int32_t *temp_celsius)
{
    const auto data = static_cast<pwm_heater_data*>(dev->data);

	if (temp_celsius == nullptr) {
		return -EINVAL;
	}

	*temp_celsius = data->current_temp;
	return 0;
}

int pwm_heater_enable(const struct device *dev)
{
    const auto data = static_cast<pwm_heater_data*>(dev->data);
	data->enabled = true;
	LOG_INF("加热器已启用，目标温度: %d.%02d°C",
		data->target_temp / 100, data->target_temp % 100);
	return 0;
}

int pwm_heater_disable(const struct device *dev)
{
    const auto data = static_cast<pwm_heater_data*>(dev->data);
    const auto config = static_cast<const pwm_heater_config*>(dev->config);

	data->enabled = false;
	/* 立即关闭加热 */
	LOG_INF("加热器已禁用");
	return pwm_set_dt(&config->pwm, data->pwm_period_us, 0);
}

#define PWM_HEATER_INIT(inst)                                    \
	static const struct pwm_heater_config pwm_heater_config_##inst = { \
		.pwm = PWM_DT_SPEC_INST_GET(inst),                      \
		.temp_sensor = DEVICE_DT_GET(DT_INST_PHANDLE(inst, temp_sensor)), \
		.temp_channel = static_cast<enum sensor_channel>(DT_INST_PROP(inst, temp_channel)),       \
	};                                                          \
                                                               \
	static struct pwm_heater_data pwm_heater_data_##inst;       \
                                                               \
	DEVICE_DT_INST_DEFINE(inst,                                 \
			      pwm_heater_init,                          \
			      NULL,                                     \
			      &pwm_heater_data_##inst,                  \
			      &pwm_heater_config_##inst,                \
			      POST_KERNEL,                              \
			      CONFIG_PWM_HEATER_INIT_PRIORITY,          \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PWM_HEATER_INIT)
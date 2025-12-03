// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#define DT_DRV_COMPAT pwm_heater

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <OF/drivers/sensor/pwm_heater.h>
#include <OF/utils/CCM.h>
#include "heater_pid.h"

LOG_MODULE_REGISTER(pwm_heater, CONFIG_PWM_HEATER_LOG_LEVEL);

struct pwm_heater_config
{
    struct pwm_dt_spec pwm;
    const struct device* temp_sensor;
    enum sensor_channel temp_channel;
};

struct pwm_heater_data
{
    struct k_work_delayable work;
    const struct device* dev;
    PID_t pid; /* stack-allocated PID object */
    int32_t target_temp; /* target temperature (centi-degrees C) */
    int32_t current_temp; /* current temperature (centi-degrees C) */
    uint32_t pwm_period_us; /* PWM period in microseconds */
    uint32_t duty_cycle_us; /* current duty cycle in microseconds */
    uint32_t max_duty_us; /* maximum duty cycle in microseconds */
    bool enabled; /* heater enabled state */
};

static int apply_pwm_setting(const struct pwm_heater_config* config,
                             struct pwm_heater_data* data,
                             uint32_t duty_cycle_us)
{
    int ret = pwm_set_dt(&config->pwm,
                         PWM_USEC(data->pwm_period_us),
                         PWM_USEC(duty_cycle_us));

    if (ret == 0)
    {
        uint32_t percent = (data->pwm_period_us == 0)
                               ? 0
                               : (duty_cycle_us * 100U) / data->pwm_period_us;
        LOG_DBG("PWM applied: period=%uus duty=%uus (%u%%)",
                data->pwm_period_us, duty_cycle_us, percent);
    }
    else
    {
        LOG_ERR("pwm_set_dt failed (period=%uus duty=%uus, err=%d)",
                data->pwm_period_us, duty_cycle_us, ret);
    }

    return ret;
}

static const PID_Params_t base_pid_params = {
    .Kp = CONFIG_PWM_HEATER_PID_KP / 1000.0f,
    .Ki = CONFIG_PWM_HEATER_PID_KI / 1000.0f,
    .Kd = CONFIG_PWM_HEATER_PID_KD / 1000.0f,
    .IntegralLimit = 500.0f,
    .Deadband = CONFIG_PWM_HEATER_TEMP_TOLERANCE / 100.f,
};

static float pwm_heater_compute_max_output(void)
{
#if IS_ENABLED(CONFIG_PWM_HEATER_PID_AUTO_MAX_OUTPUT)
    return MAX(1, CONFIG_PWM_HEATER_MAX_DUTY_CYCLE_PCT);
#else
    return MAX(1, CONFIG_PWM_HEATER_PID_MAX_OUTPUT);
#endif
}

static void pwm_heater_work_handler(struct k_work* work)
{
    struct k_work_delayable* dwork = k_work_delayable_from_work(work);
    struct pwm_heater_data* data = CONTAINER_OF(dwork, struct pwm_heater_data, work);
    const struct device* dev = data->dev;
    const struct pwm_heater_config* config = dev->config;
    struct sensor_value temp_val;
    float pid_output;
    uint32_t new_duty;

    /* Refresh sensor sample before reading the channel */
    if (sensor_sample_fetch(config->temp_sensor) < 0)
    {
        LOG_ERR("Failed to fetch temperature sample");
        goto reschedule;
    }

    if (sensor_channel_get(config->temp_sensor, config->temp_channel, &temp_val) < 0)
    {
        LOG_ERR("Failed to read temperature sensor");
        goto reschedule;
    }

    LOG_DBG("Sensor raw value: val1=%d, val2=%d", temp_val.val1, temp_val.val2);

    /* Convert to centi-degrees Celsius */
    const double temp_double = sensor_value_to_double(&temp_val);
    data->current_temp = (int32_t)(temp_double * 100.0);

    LOG_DBG("Temperature: %d.%02d°C",
            data->current_temp / 100, abs(data->current_temp % 100));

    if (!data->enabled)
    {
        /* Heater disabled: ensure PWM is zero */
        apply_pwm_setting(config, data, 0);
        goto reschedule;
    }

    const float target_c = (float)(data->target_temp) / 100.0f;
    const float current_c = (float)(data->current_temp) / 100.0f;

    LOG_DBG("PID cfg: Kp=%.3f Ki=%.3f Kd=%.3f Max=%.1f Deadband=%.2f Integral=%.2f",
            (double)data->pid.p.Kp,
            (double)data->pid.p.Ki,
            (double)data->pid.p.Kd,
            (double)data->pid.p.MaxOutput,
            (double)data->pid.p.Deadband,
            (double)data->pid.integral);
    LOG_DBG("PID inputs: target=%.2f°C current=%.2f°C error=%.2f°C",
            (double)target_c,
            (double)current_c,
            (double)(target_c - current_c));

    /* Compute PID output */
    pid_output = PID_Compute(&data->pid, target_c, current_c);

    /* Map PID output to PWM duty */
    const float pid_span = (data->pid.p.MaxOutput > 0.0f) ? data->pid.p.MaxOutput : 1.0f;
    float clamped_output = pid_output;
    if (clamped_output < 0.0f)
    {
        clamped_output = 0.0f;
    }
    else if (clamped_output > pid_span)
    {
        clamped_output = pid_span;
    }

    new_duty = (uint32_t)((clamped_output / pid_span) * data->max_duty_us);

    LOG_DBG("PID output %.2f -> duty %u/%u us (prev %u) (span %.2f)",
            (double)pid_output, new_duty, data->pwm_period_us, data->duty_cycle_us,
            (double)pid_span);

    if (new_duty != data->duty_cycle_us)
    {
        data->duty_cycle_us = new_duty;
        apply_pwm_setting(config, data, new_duty);
    }
    else
    {
        LOG_DBG("PWM duty unchanged, keeping last setting");
    }

reschedule:
    k_work_schedule(&data->work, K_MSEC(CONFIG_PWM_HEATER_UPDATE_INTERVAL_MS));
}

static int pwm_heater_init(const struct device* dev)
{
    const struct pwm_heater_config* config = dev->config;
    struct pwm_heater_data* data = dev->data;

    data->dev = dev;

    if (!device_is_ready(config->pwm.dev))
    {
        LOG_ERR("PWM device not ready");
        return -ENODEV;
    }

    if (!device_is_ready(config->temp_sensor))
    {
        LOG_ERR("Temperature sensor device not ready");
        return -ENODEV;
    }

    /* Set parameters from Kconfig */
    data->target_temp = CONFIG_PWM_HEATER_TARGET_TEMP;
    data->current_temp = 0;
    data->pwm_period_us = 20000;
    data->duty_cycle_us = 0;
    data->max_duty_us = (data->pwm_period_us * CONFIG_PWM_HEATER_MAX_DUTY_CYCLE_PCT) / 100;
    data->enabled = false; // default disabled

    /* Initialize PID controller */
    PID_Params_t pid_params = base_pid_params;
    pid_params.MaxOutput = pwm_heater_compute_max_output();
    PID_Init(&data->pid, &pid_params);

    /* Initialize work item */
    k_work_init_delayable(&data->work, pwm_heater_work_handler);

    /* Initialize PWM output */
    if (apply_pwm_setting(config, data, 0) < 0)
    {
        return -EIO;
    }

    /* Start control loop */
    k_work_schedule(&data->work, K_MSEC(CONFIG_PWM_HEATER_UPDATE_INTERVAL_MS));

    LOG_DBG("PWM heater initialized, target temperature: %d.%02d°C",
            data->target_temp / 100, data->target_temp % 100);

#if IS_ENABLED(CONFIG_PWM_HEATER_AUTOSTART)
    pwm_heater_enable(dev);
#endif

    return 0;
}

/* 实现API函数 */
int pwm_heater_get_current_temp(const struct device* dev, int32_t* temp_celsius)
{
    const struct pwm_heater_data* data = dev->data;

    if (temp_celsius == NULL)
    {
        return -EINVAL;
    }

    *temp_celsius = data->current_temp;
    return 0;
}

int pwm_heater_enable(const struct device* dev)
{
    struct pwm_heater_data* data = dev->data;
    data->enabled = true;
    LOG_DBG("Heater enabled, target temperature: %d.%02d°C",
            data->target_temp / 100, data->target_temp % 100);
    return 0;
}

int pwm_heater_disable(const struct device* dev)
{
    struct pwm_heater_data* data = dev->data;
    const struct pwm_heater_config* config = dev->config;

    data->enabled = false;
    /* Turn off heater immediately */
    LOG_DBG("Heater disabled");
    return apply_pwm_setting(config, data, 0);
}

#define PWM_HEATER_INIT(inst)                                    \
    static const struct pwm_heater_config pwm_heater_config_##inst = { \
        .pwm = PWM_DT_SPEC_INST_GET(inst),                      \
        .temp_sensor = DEVICE_DT_GET(DT_INST_PHANDLE(inst, temp_sensor)), \
        .temp_channel = DT_INST_PROP(inst, temp_channel),       \
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

DT_INST_FOREACH_STATUS_OKAY (PWM_HEATER_INIT)

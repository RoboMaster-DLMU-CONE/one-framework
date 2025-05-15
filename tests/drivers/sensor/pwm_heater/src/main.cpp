// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause
#include <zephyr/device.h>

#include "OF/drivers/sensor/pwm_heater.h"
#include "OF/drivers/utils/status_leds.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }
    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);

    led_api->set_heartbeat(status_led_dev);

    const device* accel = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
    const device* gyro = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));
    if (!device_is_ready(accel) || !device_is_ready(gyro))
    {
        LOG_ERR("BMI088 devices not ready");
        return -1;
    }
    // 初始化PWM加热器
    // 初始化PWM加热器
    const struct device* heater = DEVICE_DT_GET(DT_NODELABEL(pwm_heater));
    if (!device_is_ready(heater))
    {
        LOG_ERR("PWM加热器设备未就绪");
        return -1;
    }

    // 启用加热器 - 使用Kconfig中预设的目标温度
    int ret = pwm_heater_enable(heater);
    if (ret < 0)
    {
        LOG_ERR("启用加热器失败: %d", ret);
        return -1;
    }

    LOG_INF("程序开始运行，加热器已启动");

    while (true)
    {
        k_sleep(K_MSEC(100));
    }
}
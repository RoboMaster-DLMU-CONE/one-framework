// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <cmath>

#include "OF/drivers/sensor/pwm_heater.h"
#include "OF/drivers/output/status_leds.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* status_led_de v = DEVICE_DT_GET(DT_NODELABEL(status_leds));
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
    const device* heater = DEVICE_DT_GET(DT_NODELABEL(pwm_heater));
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

    int32_t current_temp;

    while (true)
    {
        // 读取温度 (保留原有代码)
        ret = pwm_heater_get_current_temp(heater, &current_temp);
        if (ret == 0) {
            if (current_temp < 0) {
                LOG_INF("当前温度: -%d.%02d°C",
                        abs(current_temp) / 100, abs(current_temp % 100));
            } else {
                LOG_INF("当前温度: %d.%02d°C",
                        current_temp / 100, current_temp % 100);
            }
        } else {
            LOG_ERR("读取温度失败: %d", ret);
        }

        // 读取加速度计数据
        struct sensor_value accel_x, accel_y, accel_z;
        if (sensor_sample_fetch(accel) == 0) {
            if (sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x) == 0 &&
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y) == 0 &&
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z) == 0) {

                double ax = sensor_value_to_double(&accel_x);
                double ay = sensor_value_to_double(&accel_y);
                double az = sensor_value_to_double(&accel_z);

                LOG_INF("加速度 (m/s²): X=%.2f, Y=%.2f, Z=%.2f", ax, ay, az);
                }
        } else {
            LOG_ERR("无法读取加速度计数据");
        }

        // 读取陀螺仪数据
        struct sensor_value gyro_x, gyro_y, gyro_z;
        if (sensor_sample_fetch(gyro) == 0) {
            if (sensor_channel_get(gyro, SENSOR_CHAN_GYRO_X, &gyro_x) == 0 &&
                sensor_channel_get(gyro, SENSOR_CHAN_GYRO_Y, &gyro_y) == 0 &&
                sensor_channel_get(gyro, SENSOR_CHAN_GYRO_Z, &gyro_z) == 0) {

                double gx = sensor_value_to_double(&gyro_x);
                double gy = sensor_value_to_double(&gyro_y);
                double gz = sensor_value_to_double(&gyro_z);

                LOG_INF("角速度 (rad/s): X=%.2f, Y=%.2f, Z=%.2f", gx, gy, gz);
                }
        } else {
            LOG_ERR("无法读取陀螺仪数据");
        }
        k_sleep(K_MSEC(500));
    }
}
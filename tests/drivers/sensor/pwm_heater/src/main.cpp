// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <cmath>

#include "OF/drivers/sensor/pwm_heater.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* accel = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
    const device* gyro = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));
    if (!device_is_ready(accel) || !device_is_ready(gyro))
    {
        LOG_ERR("BMI088 devices not ready");
        return -1;
    }
    // Initialize PWM heater
    const device* heater = DEVICE_DT_GET(DT_NODELABEL(pwm_heater));
    if (!device_is_ready(heater))
    {
        LOG_ERR("PWM heater device not ready");
        return -1;
    }

    // int ret = pwm_heater_enable(heater);
    // if (ret < 0)
    // {
    // LOG_ERR("Failed to enable heater: %d", ret);
    // return -1;
    // }

    LOG_INF("Program started, heater enabled");

    int32_t current_temp;

    while (true)
    {
        // Read temperature (keep existing logic)
        int ret = pwm_heater_get_current_temp(heater, &current_temp);
        if (ret == 0)
        {
            if (current_temp < 0)
            {
                LOG_INF("Current temperature: -%d.%04d°C",
                        abs(current_temp) / 100, abs(current_temp % 100));
            }
            else
            {
                LOG_INF("Current temperature: %d.%02d°C",
                        current_temp / 100, current_temp % 100);
            }
        }
        else
        {
            LOG_ERR("Failed to read temperature: %d", ret);
        }

        // Read accelerometer data
        sensor_value accel_x, accel_y, accel_z;
        if (sensor_sample_fetch(accel) == 0)
        {
            if (sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x) == 0 &&
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y) == 0 &&
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z) == 0)
            {
                double ax = sensor_value_to_double(&accel_x);
                double ay = sensor_value_to_double(&accel_y);
                double az = sensor_value_to_double(&accel_z);

                LOG_INF("Acceleration (m/s^2): X=%.2f, Y=%.2f, Z=%.2f", ax, ay, az);
            }
        }
        else
        {
            LOG_ERR("Failed to read accelerometer data");
        }

        // Read gyroscope data
        struct sensor_value gyro_x, gyro_y, gyro_z;
        if (sensor_sample_fetch(gyro) == 0)
        {
            if (sensor_channel_get(gyro, SENSOR_CHAN_GYRO_X, &gyro_x) == 0 &&
                sensor_channel_get(gyro, SENSOR_CHAN_GYRO_Y, &gyro_y) == 0 &&
                sensor_channel_get(gyro, SENSOR_CHAN_GYRO_Z, &gyro_z) == 0)
            {
                double gx = sensor_value_to_double(&gyro_x);
                double gy = sensor_value_to_double(&gyro_y);
                double gz = sensor_value_to_double(&gyro_z);

                LOG_INF("Angular rate (rad/s): X=%.2f, Y=%.2f, Z=%.2f", gx, gy, gz);
            }
        }
        else
        {
            LOG_ERR("Failed to read gyroscope data");
        }
        k_sleep(K_MSEC(500));
    }
}

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/drivers/utils/status_leds.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const struct device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }
    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);

    led_api->set_heartbeat_always_on(status_led_dev);

    const struct device* accel = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
    const struct device* gyro = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));
    if (!device_is_ready(accel) || !device_is_ready(gyro))
    {
        LOG_ERR("BMI088 devices not ready");
        return -1;
    }

    LOG_INF("开始读取BMI088传感器数据...");

    while (true)
    {
        struct sensor_value accel_val[3], gyro_val[3];

        // 读取加速度计数据
        int ret = sensor_sample_fetch(accel);
        if (ret == 0)
        {
            ret = sensor_channel_get(accel, SENSOR_CHAN_ACCEL_XYZ, accel_val);
            if (ret == 0)
            {
                LOG_INF("Accel: x=%d.%06d, y=%d.%06d, z=%d.%06d",
                        accel_val[0].val1, abs(accel_val[0].val2),
                        accel_val[1].val1, abs(accel_val[1].val2),
                        accel_val[2].val1, abs(accel_val[2].val2));
            }
            else
            {
                LOG_ERR("无法获取加速度计数据: %d", ret);
            }
        }
        else
        {
            LOG_ERR("无法读取加速度计: %d", ret);
        }

        // 读取陀螺仪数据
        ret = sensor_sample_fetch(gyro);
        if (ret == 0)
        {
            ret = sensor_channel_get(gyro, SENSOR_CHAN_GYRO_XYZ, gyro_val);
            if (ret == 0)
            {
                LOG_INF("Gyro: x=%d.%06d, y=%d.%06d, z=%d.%06d",
                        gyro_val[0].val1, abs(gyro_val[0].val2),
                        gyro_val[1].val1, abs(gyro_val[1].val2),
                        gyro_val[2].val1, abs(gyro_val[2].val2));
            }
            else
            {
                LOG_ERR("无法获取陀螺仪数据: %d", ret);
            }
        }
        else
        {
            LOG_ERR("无法读取陀螺仪: %d", ret);
        }

        k_sleep(K_MSEC(100));
    }

    return 0;
}

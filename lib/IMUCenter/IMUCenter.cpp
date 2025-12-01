#include <OF/lib/IMUCenter/IMUCenter.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(IMUCenter, CONFIG_IMU_CENTER_LOG_LEVEL);

namespace OF
{
    IMUCenter IMUCenter::m_instance;

    IMUCenter& IMUCenter::getInstance()
    {
        return m_instance;
    }

    IMUData IMUCenter::getData()
    {
        IMUData data{};
        const device* accel = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
        const device* gyro = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));
        sensor_value accel_xyz[3];
        if (sensor_channel_get(accel, SENSOR_CHAN_ACCEL_XYZ, accel_xyz) == 0)
        {
            auto& [x, y, z] = data.accel;
            x = sensor_value_to_float(&accel_xyz[0]);
            y = sensor_value_to_float(&accel_xyz[1]);
            z = sensor_value_to_float(&accel_xyz[2]);
            LOG_INF("Accel: %f, %f, %f (raw: %d.%06d, %d.%06d, %d.%06d)",
                    x, y, z,
                    accel_xyz[0].val1, accel_xyz[0].val2,
                    accel_xyz[1].val1, accel_xyz[1].val2,
                    accel_xyz[2].val1, accel_xyz[2].val2);
        }
        else
        {
            LOG_ERR("Can't read accel data: %s", strerror(errno));
        }
        sensor_value gyro_xyz[3];
        if (sensor_sample_fetch(gyro) == 0)
        {
            if (sensor_channel_get(gyro, SENSOR_CHAN_GYRO_XYZ, gyro_xyz) == 0)
            {
                auto& [x, y, z] = data.gyro;
                x = sensor_value_to_float(&gyro_xyz[0]);
                y = sensor_value_to_float(&gyro_xyz[1]);
                z = sensor_value_to_float(&gyro_xyz[2]);
                LOG_INF("Gyro: %f, %f, %f", x, y, z);
            }
        }
        else
        {
            LOG_ERR("Can't read gyro data: %s", strerror(errno));
        }
        return data;
    }

    IMUCenter::IMUCenter()
    {
#ifdef CONFIG_IMU_CENTER_USE_BMI08X
        const device* dev = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
        if (!device_is_ready(dev))
        {
            LOG_ERR("device is not ready");
            k_panic();
        }
#endif
    }

}

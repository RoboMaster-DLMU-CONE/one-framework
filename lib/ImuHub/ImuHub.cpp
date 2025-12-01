#include <OF/lib/ImuHub/ImuHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(ImuHub, CONFIG_IMU_HUB_LOG_LEVEL);

namespace OF
{
    ImuHub ImuHub::m_instance;

    ImuHub& ImuHub::getInstance()
    {
        return m_instance;
    }

    IMUData ImuHub::getData()
    {
        return m_data_buf.read();
    }

    void ImuHub::workHandler(struct k_work* work)
    {
        ImuHub* instance = CONTAINER_OF(work, ImuHub, m_work);
        instance->workLoop();
    }

    void ImuHub::workLoop()
    {
        IMUData data{};
        bool new_data_ok = true;

        sensor_value accel_xyz[3];
        if (sensor_sample_fetch(m_accel_dev) == 0)
        {
            if (sensor_channel_get(m_accel_dev, SENSOR_CHAN_ACCEL_XYZ, accel_xyz) == 0)
            {
                data.accel.x = sensor_value_to_float(&accel_xyz[0]);
                data.accel.y = sensor_value_to_float(&accel_xyz[1]);
                data.accel.z = sensor_value_to_float(&accel_xyz[2]);
            }
            else
            {
                LOG_ERR("Failed to read accel channel");
                new_data_ok = false;
            }
        }
        else
        {
            LOG_ERR("Failed to fetch accel sample");
            new_data_ok = false;
        }

        sensor_value gyro_xyz[3];
        if (sensor_sample_fetch(m_gyro_dev) == 0)
        {
            if (sensor_channel_get(m_gyro_dev, SENSOR_CHAN_GYRO_XYZ, gyro_xyz) == 0)
            {
                data.gyro.x = sensor_value_to_float(&gyro_xyz[0]);
                data.gyro.y = sensor_value_to_float(&gyro_xyz[1]);
                data.gyro.z = sensor_value_to_float(&gyro_xyz[2]);
            }
            else
            {
                LOG_ERR("Failed to read gyro channel");
                new_data_ok = false;
            }
        }
        else
        {
            LOG_ERR("Failed to fetch gyro sample");
            new_data_ok = false;
        }

        if (new_data_ok)
        {
            m_data_buf.write(data);
            LOG_DBG("Updated IMU: A(%.2f, %.2f, %.2f) G(%.2f, %.2f, %.2f)",
                    data.accel.x, data.accel.y, data.accel.z,
                    data.gyro.x, data.gyro.y, data.gyro.z);
        }

        // Schedule the next execution based on sampling frequency
        constexpr int32_t delay_ms = 1000 / CONFIG_IMU_HUB_SAMPLING_FREQUENCY;
        k_work_reschedule(&m_work, K_MSEC(delay_ms));
    }

    ImuHub::ImuHub() :
        m_accel_dev(nullptr), m_gyro_dev(nullptr)
    {
        m_accel_dev = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
        m_gyro_dev = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));

        if (!device_is_ready(m_accel_dev))
        {
            LOG_ERR("Accel device not ready");
            k_panic();
        }
        if (!device_is_ready(m_gyro_dev))
        {
            LOG_ERR("Gyro device not ready");
            k_panic();
        }

        // Initialize the delayed work
        k_work_init_delayable(&m_work, ImuHub::workHandler);

        // Schedule the initial work execution
        int32_t delay_ms = 1000 / CONFIG_IMU_HUB_SAMPLING_FREQUENCY;
        k_work_reschedule(&m_work, K_MSEC(delay_ms));

        LOG_DBG("ImuHub initialized and work scheduled");
    }

}

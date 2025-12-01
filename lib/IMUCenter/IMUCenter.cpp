#include <OF/lib/IMUCenter/IMUCenter.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(IMUCenter, CONFIG_IMU_CENTER_LOG_LEVEL);

namespace OF
{
    static K_THREAD_STACK_DEFINE(m_stack, CONFIG_IMU_CENTER_THREAD_STACK_SIZE);
    IMUCenter IMUCenter::m_instance;

    IMUCenter& IMUCenter::getInstance()
    {
        return m_instance;
    }

    IMUData IMUCenter::getData()
    {
        return m_data_buf.read();
    }

    void IMUCenter::threadEntry(void* p1, void* p2, void* p3)
    {
        static_cast<IMUCenter*>(p1)->threadLoop();
    }

    void IMUCenter::threadLoop()
    {
        // Calculate sleep time based on sampling frequency
        int32_t sleep_ms = 1000 / CONFIG_IMU_CENTER_SAMPLING_FREQUENCY;
        k_timeout_t sleep_time = K_MSEC(sleep_ms);

        while (true)
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
            k_sleep(sleep_time);
        }
    }

    IMUCenter::IMUCenter() :
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
        k_tid_t tid = k_thread_create(&m_thread, m_stack,
                                      K_THREAD_STACK_SIZEOF(m_stack),
                                      IMUCenter::threadEntry,
                                      this, nullptr, nullptr,
                                      K_PRIO_PREEMPT(CONFIG_IMU_CENTER_THREAD_PRIORITY),
                                      0,
                                      K_NO_WAIT);
        k_thread_name_set(tid, "IMU_Sampling");
        LOG_DBG("IMUCenter initialized and thread started");
    }

}

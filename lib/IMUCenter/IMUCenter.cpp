#include <OF/lib/IMUCenter/IMUCenter.hpp>


namespace OF
{
    IMUCenter IMUCenter::m_instance;

    IMUCenter& IMUCenter::getInstance()
    {
        return m_instance;
    }

    void IMUCenter::start()
    {
        m_tid = k_thread_create(&m_thread_handle, &m_thread_stack, 1024, thread_entry, this, nullptr, nullptr, 0, 0,
                                K_NO_WAIT);
    }

    void IMUCenter::thread_entry(void* p1, void* p2, void* p3)
    {
        static_cast<IMUCenter*>(p1)->process_loop();
    }

    void IMUCenter::process_loop()
    {
        while (true)
        {
            k_sem_take(&m_sem, K_FOREVER);
#ifdef CONFIG_IMU_CENTER_USE_BMI08X
            const device* accel = DEVICE_DT_GET_ANY(bmi088_accel);
            const device* gyro = DEVICE_DT_GET_ANY(bmi088_gyro);

#endif
            sensor_sample_fetch(accel);


        }
    }

    void IMUCenter::trigger_callback(const device* dev, const sensor_trigger* trigger)
    {
        k_sem_give(&getInstance().m_sem);
    }

    IMUCenter::IMUCenter()
    {
        k_sem_init(&m_sem, 0, 1);
#ifdef CONFIG_IMU_CENTER_USE_BMI08X
        const device* dev = DEVICE_DT_GET_ANY(bmi088_accel);
        constexpr sensor_trigger trig{
            .type = SENSOR_TRIG_DATA_READY,
            .chan = SENSOR_CHAN_ALL
        };
        sensor_trigger_set(dev, &trig, trigger_callback);
#endif

    }

}

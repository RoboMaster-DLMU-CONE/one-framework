#ifndef OF_LIB_IMU_CENTER_HPP
#define OF_LIB_IMU_CENTER_HPP

#include <OF/utils/SeqlockBuf.hpp>

#include <zephyr/drivers/sensor.h>

namespace OF
{
    struct IMUData
    {
        struct Quaternion
        {
            float w, x, y, z;
        } quat;

        struct Euler
        {
            float yaw, pitch, roll;
        };

        struct Vector3
        {
            float x, y, z;
        };

        Vector3 gyro, accel;
        float temp;
    };

    class IMUCenter
    {
    public:
        IMUCenter(const IMUCenter&) = delete;
        IMUCenter operator=(const IMUCenter&) = delete;

        static IMUCenter& getInstance();

    private:
        mutable k_thread m_thread_handle{};
        k_thread_stack_t m_thread_stack{};
        k_tid_t m_tid{nullptr};
        k_sem m_sem;

        void start();
        static void thread_entry(void* p1, void* p2, void* p3);
        void process_loop();
        static void trigger_callback(const device* dev, const sensor_trigger* trigger);

        static IMUCenter m_instance;
        IMUCenter();
        SeqlockBuf<IMUData> m_data;
    };
}

#endif //OF_LIB_IMU_CENTER_HPP

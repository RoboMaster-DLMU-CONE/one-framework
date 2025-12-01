#ifndef OF_LIB_IMU_CENTER_HPP
#define OF_LIB_IMU_CENTER_HPP

#include <OF/utils/SeqlockBuf.hpp>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

namespace OF
{
    struct IMUData
    {
        struct Vector3
        {
            float x, y, z;
        };

        struct Quaternion
        {
            float w, x, y, z;
        } quat;

        Vector3 gyro;
        Vector3 accel;
        float temp;
    };

    class IMUCenter
    {
    public:
        IMUCenter(const IMUCenter&) = delete;
        IMUCenter operator=(const IMUCenter&) = delete;

        static IMUCenter& getInstance();

        IMUData getData();

    private:
        IMUCenter();
        ~IMUCenter() = default;

        static IMUCenter m_instance;

        static void threadEntry(void* p1, void* p2, void* p3);
        void threadLoop();

        struct k_thread m_thread{};

        const struct device* m_accel_dev;
        const struct device* m_gyro_dev;

        // 顺序锁缓存，用于线程间数据传输
        SeqlockBuf<IMUData> m_data_buf; // 假设 SeqlockBuf 在 OF::Utils 命名空间下
    };
}

#endif //OF_LIB_IMU_CENTER_HPP

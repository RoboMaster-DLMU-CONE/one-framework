#ifndef OF_LIB_IMU_HUB_HPP
#define OF_LIB_IMU_HUB_HPP

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

    class ImuHub
    {
    public:
        ImuHub(const ImuHub&) = delete;
        ImuHub operator=(const ImuHub&) = delete;

        static ImuHub& getInstance();

        IMUData getData();

    private:
        ImuHub();
        ~ImuHub() = default;

        static ImuHub m_instance;

        static void workHandler(struct k_work* work);
        void workLoop();

        struct k_work_delayable m_work{};

        const struct device* m_accel_dev;
        const struct device* m_gyro_dev;

        // 顺序锁缓存，用于线程间数据传输
        SeqlockBuf<IMUData> m_data_buf; // 假设 SeqlockBuf 在 OF::Utils 命名空间下
    };
}

#endif //OF_LIB_IMU_HUB_HPP

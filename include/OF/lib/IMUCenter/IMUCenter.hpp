#ifndef OF_LIB_IMU_CENTER_HPP
#define OF_LIB_IMU_CENTER_HPP

#include <OF/utils/SeqlockBuf.hpp>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

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

        static IMUData getData();

    private:
        static IMUCenter m_instance;
        IMUCenter();
    };
}

#endif //OF_LIB_IMU_CENTER_HPP

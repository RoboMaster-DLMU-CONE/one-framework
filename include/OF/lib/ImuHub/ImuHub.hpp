#ifndef OF_LIB_IMU_HUB_HPP
#define OF_LIB_IMU_HUB_HPP

#include <OF/lib/HubManager/HubBase.hpp>

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

    class ImuHub : public HubBase<ImuHub, IMUData>
    {
    public:
        ImuHub(const ImuHub&) = delete;
        ImuHub operator=(const ImuHub&) = delete;

        [[nodiscard]] constexpr char* getName() const override
        {
            return const_cast<char*>("ImuHub");
        }

        void setup();

    private:
        friend class HubBase<ImuHub, IMUData>;
        ImuHub();
        ~ImuHub() = default;

        static void workHandler(struct k_work* work);
        void workLoop();

        struct k_work_delayable m_work{};
    };
}

#endif //OF_LIB_IMU_HUB_HPP

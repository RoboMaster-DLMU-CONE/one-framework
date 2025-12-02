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

    class ImuHub final : public HubBase<ImuHub, IMUData>
    {
    public:
        ImuHub(const ImuHub&) = delete;
        ImuHub operator=(const ImuHub&) = delete;

        [[nodiscard]] const char* getName() const override
        {
            return "ImuHub";
        }

        void setup();
        static void async_worker_thread(void* p1, void* p2, void* p3);

    private:
        friend class HubBase;
        ImuHub();
        ~ImuHub() = default;

        static void workHandler(struct k_work* work);
        void workLoop();

        static void process_imu_data(int result, uint8_t* buf, uint32_t buf_len, void* userdata);
        void handle_axis_update(sensor_channel channel, const struct sensor_three_axis_data& data);
        static void process_completion_queue();

        k_work_delayable m_work{};
    };
}

#endif //OF_LIB_IMU_HUB_HPP

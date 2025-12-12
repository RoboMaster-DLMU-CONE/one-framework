#include <OF/lib/Node/Node.hpp>
#include "ChassisData.hpp"
#include "GimbalData.hpp"

using namespace OF;

extern Topic<GimbalData>& topic_gimbal;
ONE_TOPIC_REGISTER(ChassisData, topic_chassis, "chassis_data");

class ChassisNode : public Node<ChassisNode>
{
public:
    struct Meta
    {
        static constexpr size_t stack_size = 2048;
        static constexpr int priority = 5;
        static constexpr const char* name = "chassis";
    };

    bool init() { return true; }

    void run()
    {
        float x{}, y{};
        while (true)
        {
            x += 1.5f;
            y -= 1.5f;
            topic_chassis.write({x, y});

            const auto [gimbal_yaw] = topic_gimbal.read();
            printk("Chassis: Read from Gimbal: %f\n", static_cast<double>(gimbal_yaw));
            k_msleep(100);
        }
    }

    void cleanup()
    {
    }
};

ONE_NODE_REGISTER(ChassisNode);


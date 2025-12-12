#include <OF/lib/Node/Node.hpp>
#include "GimbalData.hpp"
#include "ChassisData.hpp"

using namespace OF;
extern Topic<ChassisData>& topic_chassis;
ONE_TOPIC_REGISTER(GimbalData, topic_gimbal, "gimbal_data");


class GimbalNode : public Node<GimbalNode>
{
public:
    struct Meta
    {
        static constexpr size_t stack_size = 1024;
        static constexpr int priority = 5;
        static constexpr const char* name = "gimbal";
    };

    bool init() { return true; }

    void run()
    {
        float yaw{};
        while (true)
        {
            yaw += 0.1;
            topic_gimbal.write({yaw});

            const auto [x, y] = topic_chassis.read();
            printk("Gimbal: Read from chassis: %f, %f \n", static_cast<double>(x), static_cast<double>(y));
            k_msleep(100);
        }
    }

    void cleanup()
    {
    }
};

ONE_NODE_REGISTER(GimbalNode);

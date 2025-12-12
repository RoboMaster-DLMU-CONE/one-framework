#include <OF/lib/Node/Node.hpp>

using namespace OF;

struct ChassisData
{
    int x;
    float y;
};

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
        while (true)
        {
            printk("hello!\n");
            k_msleep(100);
        }
    }

    void cleanup()
    {
    }
};

ONE_NODE_REGISTER(ChassisNode);
ONE_TOPIC_REGISTER(ChassisData, topic_chassis, "chassis_data");

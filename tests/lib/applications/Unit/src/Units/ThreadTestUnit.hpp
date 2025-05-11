#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <zephyr/kernel.h>
using namespace OF;

class ThreadTestUnit final : public Unit
{
public:
    ThreadTestUnit() : threadRunning(false), threadExited(false), initCalled(false), syncSem() {}

    AUTO_UNIT_TYPE(ThreadTestUnit)

    static consteval std::string_view name() { return "ThreadTestUnit"; }
    static consteval std::string_view description() { return "Unit for thread testing"; }
    static consteval size_t stackSize() { return 2048; }
    static consteval uint8_t priority() { return 7; }

    void init() override
    {
        initCalled = true;
        k_sem_init(&syncSem, 0, 1);
    }

    void run() override;


    void requestStop() { shouldStop = true; }

    // 测试检查变量
    bool threadRunning;
    bool threadExited;
    bool initCalled;
    k_sem syncSem;
};

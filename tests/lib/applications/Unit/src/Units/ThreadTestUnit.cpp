#include "ThreadTestUnit.hpp"
#include <OF/lib/utils/PID.hpp>

[[noreturn]] void ThreadTestUnit::run()
{
    threadRunning = true;
    // 通知测试线程已启动
    k_sem_give(&syncSem);

    // 模拟工作循环
    int counter = 0;
    while (true)
    {
        counter++;
        pid.compute(100, counter);
        k_sleep(K_MSEC(50));
    }
}

REGISTER_UNIT(ThreadTestUnit)

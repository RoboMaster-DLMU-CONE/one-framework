#include "ThreadTestUnit.hpp"
void ThreadTestUnit::run()
{
    threadRunning = true;
    // 通知测试线程已启动
    k_sem_give(&syncSem);

    // 模拟工作循环
    int counter = 0;
    while (!shouldStop && counter < 5)
    {
        k_sleep(K_MSEC(100));
        counter++;
    }

    threadExited = true;
}

REGISTER_UNIT(ThreadTestUnit)

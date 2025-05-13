// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef THREADTESTUNIT_HPP
#define THREADTESTUNIT_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <zephyr/kernel.h>

#include "OF/lib/utils/PID.hpp"
using namespace OF;

// ThreadTestUnit.hpp 示例重构
class ThreadTestUnit final : public Unit
{
public:
    ThreadTestUnit() :
        threadRunning(false), threadExited(false), initCalled(false), syncSem()
    {
    }

    DEFINE_UNIT_DESCRIPTOR(ThreadTestUnit, "ThreadTestUnit", "Unit for thread testing", 2048, 7)

    void init() override
    {
        initCalled = true;
        k_sem_init(&syncSem, 0, 1);
    }

    [[noreturn]] void run() override;

    void requestStop() { shouldStop = true; }

    // 测试检查变量
    bool threadRunning;
    bool threadExited;
    bool initCalled;
    k_sem syncSem;

    PIDController<Positional, int> pid{1, 1, 1};

private:
    std::atomic<bool> shouldStop = false;
};

#endif

// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Unit/Unit.hpp>
#include <OF/lib/Unit/UnitRegistry.hpp>

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(unit, CONFIG_UNIT_LOG_LEVEL);


namespace OF
{
    void unitEntryFunction(void* unit, void*, void*)
    {
        const auto pUnit = static_cast<Unit*>(unit);
        LOG_DBG("线程 %s 开始运行", pUnit->getName().data());
        pUnit->state = UnitState::RUNNING;
        k_yield();
        while (pUnit->shouldRun())
        {
            pUnit->run();
        }
        pUnit->cleanup();
        pUnit->state = UnitState::STOPPED;
    }
    /**
     * @brief Unit 类的 cleanup 方法默认实现。
     * @details
     */
    void Unit::cleanupBase()
    {
        if (stack != nullptr)
        {
            // 终止线程（如果尚未终止）
            k_thread_abort(&thread);
            // 释放栈内存
            k_thread_stack_free(stack);
            stack = nullptr;
        }
    }
    /**
     * @brief Unit 类的析构函数默认实现。
     */
    Unit::~Unit() = default;

    void Unit::initBase()
    {

        LOG_DBG("初始化 %s Unit", getName().data());
        state = UnitState::INITIALIZING;
        if (stack = k_thread_stack_alloc(getStackSize(), 0); stack == nullptr)
        {
            LOG_ERR("无法为Unit %s分配栈内存", getName().data());
            state = UnitState::ERROR;
            return;
        }
        k_thread_create(&thread, stack, getStackSize(), unitEntryFunction, this, nullptr, nullptr, getPriority(), 0,
                        K_NO_WAIT);
        k_thread_name_set(&thread, getName().data());
        shouldStop.store(false, std::memory_order_release);

        k_yield();
    }

    /**
     * @brief 启动所有已注册的单元。
     * @details 此函数负责初始化单元注册表，创建所有已注册单元的实例，
     *          并将这些实例交给 UnitThreadManager 来初始化和启动它们各自的线程。
     *          此函数具有幂等性，多次调用只有第一次有效。
     */
    void StartUnits()
    {
        static bool initialized = false;
        if (initialized)
        {
            return;
        }
        for (auto& [name, unit] : UnitRegistry::initialize())
        {
            LOG_DBG("Starting %s", name.data());
            unit->init();
        }
        initialized = true;
    }


} // namespace OF

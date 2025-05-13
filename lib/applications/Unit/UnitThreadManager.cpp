// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(unit_thread_manager, CONFIG_UNIT_LOG_LEVEL);

namespace OF
{
    /**
     * @brief 为所有单元初始化线程
     *
     */
    void UnitThreadManager::initializeThreads(const std::unordered_map<std::string_view, std::unique_ptr<Unit>>& units)
    {
        for (const auto& [name, unit] : units)
        {
            // 初始化单元
            unit->init();
            unit->_stack = k_thread_stack_alloc(unit->getStackSize(), 0);
            if (unit->_stack == nullptr)
            {
                LOG_ERR("无法为Unit %s分配栈内存", unit->getName().data());
                continue;
            }

            k_thread_create(&unit->_thread, unit->_stack, unit->getStackSize(), threadEntryFunction, unit.get(),
                            nullptr, nullptr, unit->getPriority(), 0, K_NO_WAIT);
            if (k_thread_name_set(&unit->_thread, unit->getName().data()) != 0)
            {
                LOG_WRN("Failed to set thread name for unit %s", unit->getName().data());
            }

            unit->stats.isRunning = true;
        }
    }

    /**
     * @brief 线程入口函数
     *
     */
    void UnitThreadManager::threadEntryFunction(void* unit, void*, void*)
    {

        const auto pUnit = static_cast<Unit*>(unit);
        pUnit->run();
    }

} // namespace OF

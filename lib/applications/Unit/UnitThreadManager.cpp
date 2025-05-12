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
    void UnitThreadManager::initializeThreads(const std::vector<std::unique_ptr<Unit>>& units)
    {
        const size_t unitCount = units.size();
        const auto unitInfos = UnitRegistry::getUnits();
        for (size_t i = 0; i < unitCount; i++)
        {
            const auto& unit = units[i];
            // 查找匹配的UnitInfo
            UnitInfo* info = nullptr;
            for (auto& unitInfo : unitInfos)
            {
                if (unitInfo.typeId == unit->getTypeId())
                {
                    info = &unitInfo;
                    break;
                }
            }
            if (info == nullptr)
            {
                // 未找到对应的UnitInfo，使用默认值
                continue;
            }
            // 初始化单元
            unit->init();

            unit->stack = k_thread_stack_alloc(info->stackSize, 0);
            if (unit->stack == nullptr)
            {
                LOG_ERR("无法为Unit %s分配栈内存", info->name.data());
                continue;
            }
            k_thread_create(&unit->thread, unit->stack, info->stackSize, threadEntryFunction, unit.get(), nullptr,
                            nullptr, info->priority, 0, K_NO_WAIT);

            UnitRegistry::registerThreadMapping(info->name, i);
            info->isRunning = true;
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

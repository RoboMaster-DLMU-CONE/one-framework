#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>
namespace OF
{
#ifndef OF_TOTAL_REGISTERED_UNITS
#error "OF_TOTAL_REGISTERED_UNITS is not defined by CMake. Check CMake configuration."
#endif

    K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, OF_TOTAL_REGISTERED_UNITS, CONFIG_MAIN_STACK_SIZE);
    static std::array<bool, OF_TOTAL_REGISTERED_UNITS> stack_used = {false};

    void UnitThreadManager::initializeThreads(const std::vector<std::unique_ptr<Unit>>& units)
    {
        const size_t unitCount = units.size();
        const auto unitInfos = UnitRegistry::getUnits();
        for (size_t i = 0; i < unitCount; i++)
        {
            const auto& unit = units[i];
            // 查找匹配的UnitInfo
            const UnitInfo* info = nullptr;
            for (const auto& unitInfo : unitInfos)
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
            // 找到可用栈
            for (size_t stackIdx = 0; stackIdx < OF_TOTAL_REGISTERED_UNITS; stackIdx++)
            {
                if (!stack_used[stackIdx])
                {
                    // 标记为已使用
                    stack_used[stackIdx] = true;

                    // 创建线程
                    k_thread_create(&unit->thread, thread_stacks[stackIdx],
                                    K_THREAD_STACK_SIZEOF(thread_stacks[stackIdx]), threadEntryFunction, unit.get(),
                                    nullptr, nullptr, info->priority, 0, K_NO_WAIT);

                    // 更新状态
                    UnitRegistry::updateUnitStatus(i, true);
                    break;
                }
            }
        }
    }

    void UnitThreadManager::threadEntryFunction(void* unit, void*, void*)
    {
        const auto pUnit = static_cast<Unit*>(unit);
        pUnit->run();
    }
} // namespace OF

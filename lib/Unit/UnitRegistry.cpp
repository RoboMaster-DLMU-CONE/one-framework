// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Unit/UnitRegistry.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(unit_registry, CONFIG_UNIT_LOG_LEVEL);

extern "C" {
const extern OF::UnitRegistry::UnitFactoryFunction __start_unit_registry[];
const extern OF::UnitRegistry::UnitFactoryFunction __stop_unit_registry[];
}

namespace OF
{
    std::vector<UnitRegistry::UnitFactoryFunction> UnitRegistry::g_unitFactories;
    std::vector<UnitRegistry::UnitRegistrationFunction> UnitRegistry::g_registrationFunctions;
    std::unordered_map<std::string_view, std::unique_ptr<Unit>> UnitRegistry::g_units;

#ifdef CONFIG_UNIT_THREAD_ANALYZER
    /**
     * @brief 用于延迟工作的Zephyr工作项
     */
    static k_work_delayable stats_work;

    /**
     * @brief 延迟工作处理函数
     *
     * @details 此函数每隔5秒调用一次，用于更新所有Unit的资源使用统计
     *
     */
    static void stats_work_handler(k_work*)
    {
        UnitRegistry::updateAllUnitStats();
        k_work_schedule(&stats_work, K_SECONDS(5));
    }
#endif

    /**
     * @brief 初始化所有注册的单元
     *
     * @details 清理注册表，并调用所有已注册的注册函数，以便注册所有Unit类型
     */
    std::unordered_map<std::string_view, std::unique_ptr<Unit>>& UnitRegistry::initialize()
    {
        g_units.clear();
        g_unitFactories.clear();

        for (auto* it = __start_unit_registry; it != __stop_unit_registry; ++it)
        {
            g_unitFactories.push_back(*it);
        }

        for (const auto& factory : g_unitFactories)
        {
            auto unit = factory();
            const auto name = unit->getName();
            if (g_units.contains(name))
            {
                LOG_ERR("Unit名称冲突: %s", name.data());
                k_panic();
            }
            g_units[name] = std::move(unit);
        }

        // 清理注册函数和工厂
        g_registrationFunctions.clear();
        g_registrationFunctions.shrink_to_fit();
        g_unitFactories.clear();
        g_unitFactories.shrink_to_fit();

#ifdef CONFIG_UNIT_THREAD_ANALYZER
        k_work_init_delayable(&stats_work, stats_work_handler);
        k_work_schedule(&stats_work, K_SECONDS(5));
#endif
        return g_units;
    }

    /**
     * @brief 通过名称查找单元信息
     *
     * @param name 要查找的单元名称
     * @return std::optional<Unit*> 找到时返回指向Unit的指针，否则返回空optional
     */
    std::optional<Unit*> UnitRegistry::findUnit(const std::string_view name)
    {
        if (const auto it = g_units.find(name); it != g_units.end())
        {
            return it->second.get();
        }
        return std::nullopt;
    }

    void UnitRegistry::tryStartUnit(const std::string_view name)
    {
        if (const auto unitOpt = findUnit(name))
        {
            unitOpt.value()->init();
        }
    }

    void UnitRegistry::tryStopUnit(const std::string_view name)
    {
        if (const auto unitOpt = findUnit(name))
        {
            unitOpt.value()->tryStop();
        }
    }

    void UnitRegistry::tryRestartUnit(const std::string_view name)
    {
        if (const auto unitOpt = findUnit(name))
        {
            auto* unit = unitOpt.value();
            unit->tryStop();
            size_t counter = 0;
            while (unit->state != UnitState::STOPPED)
            {
                counter++;
                k_sleep(K_MSEC(10));
                if (counter * 10 >= CONFIG_UNIT_RESTART_WAITING_TIME)
                {
                    LOG_WRN("重启Unit：%s 失败。", unit->getName().data());
                    return;
                }
            }
            unit->init();
        }
    }

    /**
     * @brief 更新所有单元的资源使用统计
     *
     * @details 调用Zephyr线程分析器获取所有线程的资源使用情况
     */
    void UnitRegistry::updateAllUnitStats()
    {
#ifdef CONFIG_UNIT_THREAD_ANALYZER
        thread_analyzer_run(threadStatCallback, 0);
#endif
    }

#ifdef CONFIG_UNIT_THREAD_ANALYZER
    /**
     * @brief 线程统计回调函数
     *
     * @details 此函数被Zephyr线程分析器调用，用于收集线程性能统计数据
     *
     * @param info 线程分析器提供的统计信息
     */
    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void UnitRegistry::threadStatCallback(thread_analyzer_info* info)
    {
        if (info == nullptr)
            return;
        const auto unit = findUnit(info->name);
        if (!unit)
            return;
        unit.value()->stats.cpuUsage = info->utilization;
        unit.value()->stats.memoryUsage = info->stack_used;
    }
#endif

} // namespace OF

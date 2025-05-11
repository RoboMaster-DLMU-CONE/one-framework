#include "OF/lib/applications/Unit/UnitRegistry.hpp"

namespace OF
{
    std::vector<UnitInfo> UnitRegistry::g_unitInfos;
    std::vector<std::unique_ptr<Unit> (*)()> UnitRegistry::g_unitFactories;
    std::vector<UnitRegistry::UnitRegistrationFunction> UnitRegistry::g_registrationFunctions;
    std::unordered_map<std::string_view, size_t> UnitRegistry::g_nameToUnitMap;


    void UnitRegistry::addRegistrationFunction(const UnitRegistrationFunction func)
    {
        g_registrationFunctions.push_back(func);
    }

#ifdef CONFIG_UNIT_THREAD_ANALYZER
    static struct k_work_delayable stats_work;

    static void stats_work_handler(struct k_work* work)
    {
        UnitRegistry::updateAllUnitStats();
        k_work_schedule(&stats_work, K_SECONDS(5));
    }
#endif

    void UnitRegistry::initialize()
    {
#ifndef OF_TOTAL_REGISTERED_UNITS
#error "OF_TOTAL_REGISTERED_UNITS is not defined by CMake. Check CMake configuration."
#endif
        g_unitInfos.clear();
        g_unitFactories.clear();

        // 执行所有注册函数
        for (const auto func : g_registrationFunctions)
        {
            func();
        }
#ifdef CONFIG_UNIT_THREAD_ANALYZER
        k_work_init_delayable(&stats_work, stats_work_handler);
        k_work_schedule(&stats_work, K_SECONDS(5));
#endif
    }

    std::span<const UnitInfo> UnitRegistry::getUnits() { return g_unitInfos; }

    std::vector<std::unique_ptr<Unit>> UnitRegistry::createAllUnits()
    {
        std::vector<std::unique_ptr<Unit>> units;
        units.reserve(g_unitFactories.size());
        for (const auto& factory : g_unitFactories)
        {
            units.push_back(factory());
        }
        return units;
    }

    std::optional<UnitInfo*> UnitRegistry::findUnit(const std::string_view name)
    {
        if (const auto it = g_nameToUnitMap.find(name); it != g_nameToUnitMap.end())
        {
            return &g_unitInfos[it->second];
        }
        return std::nullopt;
    }

    void UnitRegistry::updateUnitStatus(const size_t idx, const bool running)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].isRunning = running;
        }
    }

    void UnitRegistry::updateUnitStats(const size_t idx, const uint32_t cpu, const uint32_t mem)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].stats.cpuUsage = cpu;
            g_unitInfos[idx].stats.memoryUsage = mem;
        }
    }

    void UnitRegistry::updateAllUnitStats()
    {
#ifdef CONFIG_UNIT_THREAD_ANALYZER
        thread_analyzer_run(threadStatCallback, 0);
#endif
    }

    void UnitRegistry::registerThreadMapping(const std::string_view name, const size_t unitIndex)
    {
        g_nameToUnitMap[name] = unitIndex;
    }

    void UnitRegistry::threadStatCallback(thread_analyzer_info* info)
    {
        if (info == nullptr)
            return;
        if (const auto it = g_nameToUnitMap.find(info->name); it != g_nameToUnitMap.end())
        {
            uint32_t cpuUsage = 0;

#ifdef CONFIG_THREAD_RUNTIME_STATS
            // 如果启用了线程运行时统计
            cpuUsage = info->utilization; // 直接使用Zephyr提供的利用率值
#elif defined(CONFIG_SCHED_THREAD_USAGE)
            // 如果只启用了线程使用率追踪
            if (info->usage.execution_cycles > 0)
            {
                cpuUsage = (uint32_t)((info->usage.total_cycles * 100) / info->usage.execution_cycles);
            }
#endif
            updateUnitStats(it->second, cpuUsage, info->stack_used);
        }
    }
} // namespace OF

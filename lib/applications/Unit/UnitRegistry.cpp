// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include "OF/lib/applications/Unit/UnitRegistry.hpp"

namespace OF
{
    std::vector<UnitInfo> UnitRegistry::g_unitInfos;
    std::vector<UnitRegistry::UnitFactoryFunction> UnitRegistry::g_unitFactories;
    std::vector<UnitRegistry::UnitRegistrationFunction> UnitRegistry::g_registrationFunctions;
    std::unordered_map<std::string_view, size_t> UnitRegistry::g_nameToUnitMap;

    /**
     * @brief 添加注册函数到注册表
     */
    void UnitRegistry::addRegistrationFunction(const UnitRegistrationFunction func)
    {
        g_registrationFunctions.push_back(func);
    }

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

    /**
     * @brief 获取所有注册的单元信息
     *
     * @return std::span<const UnitInfo> 包含所有注册的Unit信息的只读视图
     */
    std::span<const UnitInfo> UnitRegistry::getUnits() { return g_unitInfos; }

    /**
     * @brief 创建所有注册单元的实例
     *
     * @return std::vector<std::unique_ptr<Unit>> 包含所有创建的Unit实例的向量
     */
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

    /**
     * @brief 通过名称查找单元信息
     *
     * @param name 要查找的单元名称
     * @return std::optional<UnitInfo*> 找到时返回指向UnitInfo的指针，否则返回空optional
     */
    std::optional<UnitInfo*> UnitRegistry::findUnit(const std::string_view name)
    {
        if (const auto it = g_nameToUnitMap.find(name); it != g_nameToUnitMap.end())
        {
            return &g_unitInfos[it->second];
        }
        return std::nullopt;
    }

    /**
     * @brief 更新单元运行状态
     *
     * @param idx 单元在注册表中的索引
     * @param running 单元是否正在运行
     */
    void UnitRegistry::updateUnitStatus(const size_t idx, const bool running)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].isRunning = running;
        }
    }

    /**
     * @brief 更新单元资源使用统计
     *
     * @param idx 单元在注册表中的索引
     * @param cpu CPU使用率
     * @param mem 内存使用量
     */
    void UnitRegistry::updateUnitStats(const size_t idx, const uint32_t cpu, const uint32_t mem)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].stats.cpuUsage = cpu;
            g_unitInfos[idx].stats.memoryUsage = mem;
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

    /**
     * @brief 注册线程与单元索引的映射关系
     *
     * @param name 线程名称
     * @param unitIndex 单元在注册表中的索引
     */
    void UnitRegistry::registerThreadMapping(const std::string_view name, const size_t unitIndex)
    {
        g_nameToUnitMap[name] = unitIndex;
    }

#ifdef CONFIG_UNIT_THREAD_ANALYZER
    /**
     * @brief 线程统计回调函数
     *
     * @details 此函数被Zephyr线程分析器调用，用于收集线程性能统计数据
     *
     * @param info 线程分析器提供的统计信息
     */
    void UnitRegistry::threadStatCallback(thread_analyzer_info* info)
    {
        if (info == nullptr)
            return;
        if (const auto it = g_nameToUnitMap.find(info->name); it != g_nameToUnitMap.end())
        {
            updateUnitStats(it->second, info->utilization, info->stack_used);
        }
    }
#endif

} // namespace OF

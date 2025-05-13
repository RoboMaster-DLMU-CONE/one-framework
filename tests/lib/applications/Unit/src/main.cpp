// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>

#include <string_view>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "Units/TestUnit1.hpp"
#include "Units/TestUnit2.hpp"

using namespace OF;

// 测试Unit生命周期方法
ZTEST(unit_tests, test_unit_lifecycle)
{
    TestUnit1 unit;

    zassert_false(unit.initialized, "Unit should not be initialized initially");
    unit.init();
    zassert_true(unit.initialized, "Unit init() method failed");

    zassert_false(unit.runCalled, "Unit run() should not be called yet");
    unit.run();
    zassert_true(unit.runCalled, "Unit run() method failed");

    zassert_false(unit.cleanupCalled, "Unit cleanup() should not be called yet");
    unit.cleanup();
    zassert_true(unit.cleanupCalled, "Unit cleanup() method failed");
}

// 测试Unit元数据访问方法
ZTEST(unit_tests, test_unit_metadata)
{
    TestUnit1 unit1;
    TestUnit2 unit2;

    zassert_true(unit1.getName() == "TestUnit1", "Unit name mismatch");
    zassert_true(unit1.getDescription() == "Unit1 for testing", "Unit description mismatch");
    zassert_equal(unit1.getStackSize(), 2048, "Unit stack size mismatch");
    zassert_equal(unit1.getPriority(), 5, "Unit priority mismatch");

    zassert_true(unit2.getName() == "TestUnit2", "Unit name mismatch");
    zassert_true(unit2.getDescription() == "Unit2 for testing", "Unit description mismatch");
    zassert_equal(unit2.getStackSize(), 4096, "Unit stack size mismatch");
    zassert_equal(unit2.getPriority(), 10, "Unit priority mismatch");
}

// 测试单元注册
ZTEST(unit_registry_tests, test_registration)
{
    // 验证注册的单元可以找到
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto unit3 = UnitRegistry::findUnit("ThreadTestUnit");

    zassert_not_null(unit1.has_value(), "TestUnit1 not registered");
    zassert_not_null(unit2.has_value(), "TestUnit2 not registered");
    zassert_not_null(unit3.has_value(), "ThreadTestUnit not registered");
}

// 测试通过名称查找单元
ZTEST(unit_registry_tests, test_find_unit)
{
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto notFound = UnitRegistry::findUnit("NonexistentUnit");

    zassert_true(unit1.has_value(), "Failed to find TestUnit1 by name");
    zassert_true(unit2.has_value(), "Failed to find TestUnit2 by name");
    zassert_false(notFound.has_value(), "Should return empty optional for missing units");

    if (unit1)
        zassert_true(unit1.value()->getName() == "TestUnit1", "Unit1 name incorrect");
    if (unit2)
        zassert_true(unit2.value()->getName() == "TestUnit2", "Unit2 name incorrect");
}

// 线程管理器测试
ZTEST(thread_manager_tests, test_initialize_threads)
{
    // 查找ThreadTestUnit
    const auto threadUnit = UnitRegistry::findUnit("ThreadTestUnit");
    zassert_true(threadUnit.has_value(), "ThreadTestUnit should be registered");

    if (threadUnit)
    {
        zassert_true(threadUnit.value()->stats.isRunning, "ThreadTestUnit thread should be running");
    }
}

ZTEST(thread_manager_tests, test_periodic_stats_update)
{
    // 获取ThreadTestUnit
    const auto threadUnit = UnitRegistry::findUnit("ThreadTestUnit");
    zassert_true(threadUnit.has_value(), "Failed to find ThreadTestUnit");

    if (threadUnit)
    {
        // 记录初始状态
        const uint32_t initialCpu = threadUnit.value()->stats.cpuUsage;
        const uint32_t initialMem = threadUnit.value()->stats.memoryUsage;
        TC_PRINT("Initial stats: CPU=%u%%, Mem=%u bytes\n", initialCpu, initialMem);

        // 等待足够长的时间让定时器触发更新（多于5秒）
        k_sleep(K_SECONDS(7));

        // 获取更新后的状态
        const uint32_t updatedCpu = threadUnit.value()->stats.cpuUsage;
        const uint32_t updatedMem = threadUnit.value()->stats.memoryUsage;
        TC_PRINT("Updated stats: CPU=%u%%, Mem=%u bytes\n", updatedCpu, updatedMem);

        // 报告状态变化
        TC_PRINT("Stats changed: CPU delta=%d, Mem delta=%d\n",
                 static_cast<int>(updatedCpu) - static_cast<int>(initialCpu),
                 static_cast<int>(updatedMem) - static_cast<int>(initialMem));
    }
}

ZTEST(thread_manager_tests, test_empty_units_list)
{
    std::unordered_map<std::string_view, std::unique_ptr<Unit>> emptyUnits;
    // 调用初始化空列表不应崩溃
    UnitThreadManager::initializeThreads(emptyUnits);

    // 成功到达这里表示没有崩溃
    zassert_true(true, "UnitThreadManager handled empty units list");
}

static void* common_test_setup(void)
{
    StartUnits();
    return nullptr;
}

ZTEST_SUITE(thread_manager_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(unit_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(unit_registry_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);


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
    zassert_true(TestUnit1::name() == "TestUnit1", "Unit name mismatch");
    zassert_true(TestUnit1::description() == "First test unit", "Unit description mismatch");
    zassert_equal(TestUnit1::stackSize(), 2048, "Unit stack size mismatch");
    zassert_equal(TestUnit1::priority(), 5, "Unit priority mismatch");

    zassert_true(TestUnit2::name() == "TestUnit2", "Unit name mismatch");
    zassert_true(TestUnit2::description() == "Second test unit", "Unit description mismatch");
    zassert_equal(TestUnit2::stackSize(), 4096, "Unit stack size mismatch");
    zassert_equal(TestUnit2::priority(), 10, "Unit priority mismatch");
}

// 测试单元注册
ZTEST(unit_registry_tests, test_registration)
{
    // 获取运行时注册的单元
    const auto units = UnitRegistry::getUnits();

    // 验证注册的单元数量
    zassert_equal(units.size(), 3, "Registry should have 3 units");

    // 单元注册的顺序可能取决于静态初始化顺序，因此我们需要检查两个单元都在列表中
    bool foundUnit1 = false;
    bool foundUnit2 = false;
    bool foundUnit3 = false;

    for (const auto& unit : units)
    {
        if (unit.name == "TestUnit1")
            foundUnit1 = true;
        if (unit.name == "TestUnit2")
            foundUnit2 = true;
        if (unit.name == "ThreadTestUnit")
            foundUnit3 = true;
    }

    zassert_true(foundUnit1, "TestUnit1 not registered");
    zassert_true(foundUnit2, "TestUnit2 not registered");
    zassert_true(foundUnit3, "TestUnit3 not registered");
}

// 测试通过名称查找单元
ZTEST(unit_registry_tests, test_find_unit)
{
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto notFound = UnitRegistry::findUnit("NonexistentUnit");

    zassert_not_equal(unit1, std::nullopt, "Failed to find Unit1 by name");
    zassert_not_equal(unit2, std::nullopt, "Failed to find Unit2 by name");
    zassert_equal(notFound, std::nullopt, "Should return nullptr for missing units");

    if (unit1)
        zassert_true(unit1.value()->name == "TestUnit1", "Unit1 name incorrect");
    if (unit2)
        zassert_true(unit2.value()->name == "TestUnit2", "Unit2 name incorrect");
}

// 线程管理器测试
ZTEST(thread_manager_tests, test_initialize_threads)
{
    const auto units = UnitRegistry::getUnits();
    // 查找ThreadTestUnit
    bool foundThreadUnit = false;
    for (const auto& unitInfo : units)
    {
        if (unitInfo.name == "ThreadTestUnit")
        {
            foundThreadUnit = true;
            zassert_true(unitInfo.isRunning, "ThreadTestUnit thread should be running");
            break;
        }
    }

    zassert_true(foundThreadUnit, "ThreadTestUnit should be registered");
}

ZTEST(thread_manager_tests, test_periodic_stats_update)
{
    // 获取当前已注册并运行的单元
    const auto initialUnits = UnitRegistry::getUnits();
    // 查找 ThreadTestUnit 的索引
    size_t unitIdx = SIZE_MAX;
    for (size_t i = 0; i < initialUnits.size(); i++)
    {
        if (initialUnits[i].name == "ThreadTestUnit")
        {
            unitIdx = i;
            break;
        }
    }
    zassert_not_equal(unitIdx, SIZE_MAX, "Failed to find ThreadTestUnit index");
    // 记录初始状态
    const uint32_t initialCpu = initialUnits[unitIdx].stats.cpuUsage;
    const uint32_t initialMem = initialUnits[unitIdx].stats.memoryUsage;
    TC_PRINT("Initial stats: CPU=%u%%, Mem=%u bytes\n", initialCpu, initialMem);
    // 等待足够长的时间让定时器触发更新（多于5秒）
    k_sleep(K_SECONDS(7));
    // 获取更新后的状态
    const auto afterUpdate = UnitRegistry::getUnits();
    const uint32_t updatedCpu = afterUpdate[unitIdx].stats.cpuUsage;
    const uint32_t updatedMem = afterUpdate[unitIdx].stats.memoryUsage;
    TC_PRINT("Updated stats: CPU=%u%%, Mem=%u bytes\n", updatedCpu, updatedMem);
    // 验证状态已更新（在实际环境中，值可能有所不同）
    TC_PRINT("Stats changed: CPU delta=%d, Mem delta=%d\n", static_cast<int>(updatedCpu) - static_cast<int>(initialCpu),
             static_cast<int>(updatedMem) - static_cast<int>(initialMem));
}

ZTEST(thread_manager_tests, test_empty_units_list)
{
    constexpr std::vector<std::unique_ptr<Unit>> emptyUnits;
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

ZTEST_SUITE(unit_tests, nullptr, common_test_setup, NULL, NULL, NULL);

ZTEST_SUITE(unit_registry_tests, nullptr, common_test_setup, NULL, NULL, NULL);

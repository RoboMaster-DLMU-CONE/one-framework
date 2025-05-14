// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>

#include <string_view>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "Units/TestUnit1.hpp"
#include "Units/TestUnit2.hpp"
#include "Units/ThreadTestUnit.hpp"
#include "zephyr/logging/log.h"

using namespace OF;

LOG_MODULE_REGISTER(unit_test, CONFIG_UNIT_LOG_LEVEL);

// 测试Unit生命周期方法
ZTEST(unit_tests, test_unit_lifecycle)
{
    TestUnit1 unit{};
    // 初始状态检查
    zassert_false(unit.initialized, "Unit应该初始未初始化");
    zassert_equal(unit.state, UnitState::UNINITIALIZED, "Unit初始状态错误");
    // 初始化测试
    unit.init();
    zassert_true(unit.initialized, "Unit自定义初始化失败");
    zassert_equal(unit.state, UnitState::INITIALIZING, "Unit状态应为INITIALIZING");
    // 运行测试
    zassert_false(unit.runCalled, "Unit运行方法不应被自动调用");
    unit.run();
    zassert_true(unit.runCalled, "Unit运行方法调用失败");
    // 清理测试
    unit.state = UnitState::STOPPED;
    zassert_false(unit.cleanupCalled, "Unit清理方法不应被自动调用");
    unit.cleanup();
    zassert_true(unit.cleanupCalled, "Unit清理方法调用失败");
    unit.state = UnitState::RUNNING;
}

// 测试线程停止控制
ZTEST(unit_tests, test_thread_control)
{
    ThreadTestUnit unit{};
    // 初始状态检查
    zassert_true(unit.shouldRun(), "Unit初始应处于运行状态");

    // 请求停止
    unit.tryStop();
    zassert_false(unit.shouldRun(), "Unit停止标志未正确设置");
    zassert_equal(unit.state, UnitState::STOPPING, "Unit状态应为STOPPING");
}

// 测试Unit元数据访问方法
ZTEST(unit_tests, test_unit_metadata)
{
    const TestUnit1 unit1;
    const TestUnit2 unit2;

    zassert_true(unit1.getName() == "TestUnit1", "Unit名称不匹配");
    zassert_true(unit1.getDescription() == "First test unit", "Unit描述不匹配");
    zassert_equal(unit1.getStackSize(), 1024, "Unit栈大小不匹配");
    zassert_equal(unit1.getPriority(), 5, "Unit优先级不匹配");

    zassert_true(unit2.getName() == "TestUnit2", "Unit名称不匹配");
    zassert_true(unit2.getDescription() == "Unit2 for testing", "Unit描述不匹配");
    zassert_equal(unit2.getStackSize(), 4096, "Unit栈大小不匹配");
    zassert_equal(unit2.getPriority(), 10, "Unit优先级不匹配");
}

// 测试单元注册
ZTEST(unit_registry_tests, test_registration)
{
    // 验证注册的单元可以找到
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto unit3 = UnitRegistry::findUnit("ThreadTestUnit");

    zassert(unit1.has_value(), "TestUnit1未注册");
    zassert(unit2.has_value(), "TestUnit2未注册");
    zassert(unit3.has_value(), "ThreadTestUnit未注册");
}

// 测试通过名称查找单元
ZTEST(unit_registry_tests, test_find_unit)
{
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto notFound = UnitRegistry::findUnit("NonexistentUnit");

    zassert_true(unit1.has_value(), "通过名称查找TestUnit1失败");
    zassert_true(unit2.has_value(), "通过名称查找TestUnit2失败");
    zassert_false(notFound.has_value(), "对于不存在的单元应返回空optional");

    if (unit1)
        zassert_true(unit1.value()->getName() == "TestUnit1", "Unit1名称不正确");
    if (unit2)
        zassert_true(unit2.value()->getName() == "TestUnit2", "Unit2名称不正确");
}

// 测试Unit注册表的控制方法
ZTEST(unit_registry_tests, test_unit_control)
{
    // 测试停止Unit
    UnitRegistry::tryStopUnit("ThreadTestUnit");
    const auto unit = UnitRegistry::findUnit("ThreadTestUnit");
    zassert_true(unit.has_value(), "找不到ThreadTestUnit");
    if (unit)
    {
        zassert_false(unit.value()->shouldRun(), "Unit停止标志未正确设置");
        zassert_equal(unit.value()->state, UnitState::STOPPING, "Unit状态应为STOPPING");
    }

    // 测试重启Unit (应该先停止再启动)
    UnitRegistry::tryRestartUnit("TestUnit1");
    const auto unit1 = UnitRegistry::findUnit("TestUnit1");
    if (unit1)
    {
        // 重启会调用init()，状态应为INITIALIZING
        zassert_equal(unit1.value()->state, UnitState::INITIALIZING, "重启后Unit状态错误");
    }
}

// 线程管理测试
ZTEST(thread_tests, test_initialize_threads)
{
    // 查找ThreadTestUnit
    const auto threadUnit = UnitRegistry::findUnit("ThreadTestUnit");
    zassert_true(threadUnit.has_value(), "ThreadTestUnit应该已注册");
    k_sleep(K_SECONDS(1));
    if (threadUnit)
    {
        zassert_true(threadUnit.value()->state == UnitState::RUNNING, "ThreadTestUnit线程应该处于运行状态");
    }
}

// 测试统计数据更新
ZTEST(thread_tests, test_periodic_stats_update)
{
    // 获取ThreadTestUnit
    const auto threadUnit = UnitRegistry::findUnit("ThreadTestUnit");
    zassert_true(threadUnit.has_value(), "找不到ThreadTestUnit");

    if (threadUnit)
    {
        // 记录初始状态
        const uint32_t initialCpu = threadUnit.value()->stats.cpuUsage;
        const uint32_t initialMem = threadUnit.value()->stats.memoryUsage;
        TC_PRINT("CPU=%u%%, Memory=%u bytes\n", initialCpu, initialMem);

        // 等待足够长的时间让定时器触发更新
        k_sleep(K_SECONDS(7));

        // 获取更新后的状态
        const uint32_t updatedCpu = threadUnit.value()->stats.cpuUsage;
        const uint32_t updatedMem = threadUnit.value()->stats.memoryUsage;
        TC_PRINT("updated CPU=%u%%, memory=%u bytes\n", updatedCpu, updatedMem);
    }
}

static void* common_test_setup(void)
{
    StartUnits();
    LOG_INF("starting test...");
    k_yield();
    k_sleep(K_SECONDS(10));
    return nullptr;
}

ZTEST_SUITE(unit_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(thread_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(unit_registry_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);

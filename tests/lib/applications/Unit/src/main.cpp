
#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
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
    zassert_equal(units.size(), 2, "Registry should have 2 units");

    // 单元注册的顺序可能取决于静态初始化顺序，因此我们需要检查两个单元都在列表中
    bool foundUnit1 = false;
    bool foundUnit2 = false;

    for (const auto& unit : units)
    {
        if (unit.name == "TestUnit1")
            foundUnit1 = true;
        if (unit.name == "TestUnit2")
            foundUnit2 = true;
    }

    zassert_true(foundUnit1, "TestUnit1 not registered");
    zassert_true(foundUnit2, "TestUnit2 not registered");
}

// 测试通过名称查找单元
ZTEST(unit_registry_tests, test_find_unit)
{
    const auto* unit1 = UnitRegistry::findUnit("TestUnit1");
    const auto* unit2 = UnitRegistry::findUnit("TestUnit2");
    const auto* notFound = UnitRegistry::findUnit("NonexistentUnit");

    zassert_not_null(unit1, "Failed to find Unit1 by name");
    zassert_not_null(unit2, "Failed to find Unit2 by name");
    zassert_equal(notFound, nullptr, "Should return nullptr for missing units");

    if (unit1)
        zassert_true(unit1->name == "TestUnit1", "Unit1 name incorrect");
    if (unit2)
        zassert_true(unit2->name == "TestUnit2", "Unit2 name incorrect");
}

// 测试创建单元实例
ZTEST(unit_registry_tests, test_create_units)
{
    const auto units = UnitRegistry::createAllUnits();
    zassert_equal(units.size(), 2, "Should create 2 unit instances");

    // 初始化并运行所有单元
    for (const auto& unit : units)
    {
        unit->init();
        unit->run();
    }

    // 使用类型ID查找实例
    bool foundTestUnit1 = false;
    for (const auto& unit : units)
    {
        // 使用类型ID检查
        if (unit->getTypeId() == TestUnit1::TYPE_ID)
        {
            foundTestUnit1 = true;
            // 安全转换，无需RTTI
            const auto* testUnit1 = static_cast<TestUnit1*>(unit.get()); // NOLINT(*-pro-type-static-cast-downcast)
            zassert_true(testUnit1->initialized, "TestUnit1 init() method should have been called");
            zassert_true(testUnit1->runCalled, "TestUnit1 run() method should have been called");
        }
    }

    zassert_true(foundTestUnit1, "TestUnit1 instance not created");
}

// 测试更新单元状态
ZTEST(unit_registry_tests, test_update_status)
{
    // 获取当前单元索引
    const auto units = UnitRegistry::getUnits();
    size_t unit1Idx = SIZE_MAX;
    size_t unit2Idx = SIZE_MAX;

    for (size_t i = 0; i < units.size(); i++)
    {
        if (units[i].name == "TestUnit1")
            unit1Idx = i;
        if (units[i].name == "TestUnit2")
            unit2Idx = i;
    }

    zassert_not_equal(unit1Idx, SIZE_MAX, "Failed to find Unit1 index");
    zassert_not_equal(unit2Idx, SIZE_MAX, "Failed to find Unit2 index");

    // 更新运行状态
    UnitRegistry::updateUnitStatus(unit1Idx, true);

    // 验证更新后状态
    auto updatedUnits = UnitRegistry::getUnits();
    zassert_true(updatedUnits[unit1Idx].isRunning, "Unit1 status not updated correctly");
    zassert_false(updatedUnits[unit2Idx].isRunning, "Unit2 status should remain unchanged");

    // 测试无效索引
    UnitRegistry::updateUnitStatus(99, true); // 不应该崩溃
}

// 测试更新单元统计信息
ZTEST(unit_registry_tests, test_update_stats)
{
    // 获取当前单元索引
    auto units = UnitRegistry::getUnits();
    size_t unit1Idx = SIZE_MAX;
    size_t unit2Idx = SIZE_MAX;

    for (size_t i = 0; i < units.size(); i++)
    {
        if (units[i].name == "TestUnit1")
            unit1Idx = i;
        if (units[i].name == "TestUnit2")
            unit2Idx = i;
    }

    // 更新Unit1统计信息
    UnitRegistry::updateUnitStats(unit1Idx, 50, 1024);
    UnitRegistry::updateUnitStats(unit2Idx, 75, 2048);

    // 验证更新
    auto updatedUnits = UnitRegistry::getUnits();
    zassert_equal(updatedUnits[unit1Idx].stats.cpuUsage, 50, "CPU usage not updated correctly");
    zassert_equal(updatedUnits[unit1Idx].stats.memoryUsage, 1024, "Memory usage not updated correctly");
    zassert_equal(updatedUnits[unit2Idx].stats.cpuUsage, 75, "CPU usage not updated correctly");
    zassert_equal(updatedUnits[unit2Idx].stats.memoryUsage, 2048, "Memory usage not updated correctly");

    // 测试输出
    TC_PRINT("Unit1: CPU=%u%%, Mem=%u bytes\n", updatedUnits[unit1Idx].stats.cpuUsage,
             updatedUnits[unit1Idx].stats.memoryUsage);
    TC_PRINT("Unit2: CPU=%u%%, Mem=%u bytes\n", updatedUnits[unit2Idx].stats.cpuUsage,
             updatedUnits[unit2Idx].stats.memoryUsage);

    // 测试无效索引
    UnitRegistry::updateUnitStats(99, 100, 4096); // 不应该崩溃
}

ZTEST_SUITE(unit_tests, nullptr, NULL, NULL, NULL, NULL);

void* initializeUnitRegistry()
{
    UnitRegistry::initialize();
    // 可以添加调试输出
    const auto units = UnitRegistry::getUnits();
    TC_PRINT("Found %zu registered units\n", units.size());

    return nullptr;
}

ZTEST_SUITE(unit_registry_tests, nullptr, initializeUnitRegistry, NULL, NULL, NULL);

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

using namespace OF;

// Test Unit implementations
class TestUnit1 final : public Unit {
public:
  void init() override { initialized = true; }
  void run() override { runCalled = true; }
  void cleanup() override { cleanupCalled = true; }

  static consteval std::string_view name() { return "TestUnit1"; }
  static consteval std::string_view description() { return "First test unit"; }
  static consteval size_t stackSize() { return 2048; }
  static consteval uint8_t priority() { return 5; }

  bool initialized = false;
  bool runCalled = false;
  bool cleanupCalled = false;
};

class TestUnit2 final : public Unit {
public:
  void init() override {}
  void run() override {}

  static consteval std::string_view name() { return "TestUnit2"; }
  static consteval std::string_view description() { return "Second test unit"; }
  static consteval size_t stackSize() { return 4096; }
  static consteval uint8_t priority() { return 10; }
};

extern "C" {

// Test Unit instance lifecycle methods
ZTEST(unit_tests, test_unit_lifecycle) {
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

// Test Unit metadata accessor methods
ZTEST(unit_tests, test_unit_metadata) {
  zassert_true(TestUnit1::name() == "TestUnit1", "Unit name mismatch");
  zassert_true(TestUnit1::description() == "First test unit", "Unit description mismatch");
  zassert_equal(TestUnit1::stackSize(), 2048, "Unit stack size mismatch");
  zassert_equal(TestUnit1::priority(), 5, "Unit priority mismatch");

  zassert_true(TestUnit2::name() == "TestUnit2", "Unit name mismatch");
  zassert_true(TestUnit2::description() == "Second test unit", "Unit description mismatch");
  zassert_equal(TestUnit2::stackSize(), 4096, "Unit stack size mismatch");
  zassert_equal(TestUnit2::priority(), 10, "Unit priority mismatch");
}

// Test unit registration
ZTEST(unit_registry_tests, test_registration) {
  UnitRegistry::reset();
  const size_t idx1 = UnitRegistry::registerUnit<TestUnit1>();
  const size_t idx2 = UnitRegistry::registerUnit<TestUnit2>();

  zassert_equal(idx1, 0, "First unit should have index 0");
  zassert_equal(idx2, 1, "Second unit should have index 1");
  zassert_equal(UnitRegistry::getUnitCount(), 2, "Registry should have 2 units");

  const auto& units = UnitRegistry::getUnits();
  zassert_true(units[0].name == "TestUnit1", "Unit1 name not correctly registered");
  zassert_true(units[1].name == "TestUnit2", "Unit2 name not correctly registered");

  zassert_equal(units[0].stackSize, 2048, "Unit1 stack size not correctly registered");
  zassert_equal(units[1].priority, 10, "Unit2 priority not correctly registered");
}

// Test finding units by name
ZTEST(unit_registry_tests, test_find_unit) {
  UnitRegistry::registerUnit<TestUnit1>();
  UnitRegistry::registerUnit<TestUnit2>();

  const auto& unit1 = UnitRegistry::findUnit("TestUnit1");
  const auto& unit2 = UnitRegistry::findUnit("TestUnit2");
  const auto& notFound = UnitRegistry::findUnit("NonexistentUnit");

  zassert_true(unit1.name == "TestUnit1", "Failed to find Unit1 by name");
  zassert_true(unit2.name == "TestUnit2", "Failed to find Unit2 by name");
  zassert_true(notFound.name == "NotFound", "Should return NotFound unit for missing units");
}

// Test updating unit status
ZTEST(unit_registry_tests, test_update_status) {
  // Initially both units should be marked as not running
  const auto& units = UnitRegistry::getUnits();
  zassert_false(units[0].isRunning, "Unit1 should not be running initially");
  zassert_false(units[1].isRunning, "Unit2 should not be running initially");

  // Update running status
  UnitRegistry::updateUnitStatus(0, true);
  zassert_true(units[0].isRunning, "Unit1 status not updated correctly");
  zassert_false(units[1].isRunning, "Unit2 status should remain unchanged");

  // Test invalid index
  UnitRegistry::updateUnitStatus(99, true); // Should not crash
}

// Test updating unit statistics
ZTEST(unit_registry_tests, test_update_stats) {
  const auto& units = UnitRegistry::getUnits();

  // Update statistics for Unit1
  UnitRegistry::updateUnitStats(0, 50, 1024);
  zassert_equal(units[0].stats.cpuUsage, 50, "CPU usage not updated correctly");
  zassert_equal(units[0].stats.memoryUsage, 1024, "Memory usage not updated correctly");

  // Update statistics for Unit2
  UnitRegistry::updateUnitStats(1, 75, 2048);
  zassert_equal(units[1].stats.cpuUsage, 75, "CPU usage not updated correctly");
  zassert_equal(units[1].stats.memoryUsage, 2048, "Memory usage not updated correctly");

  // Test with debug output
  TC_PRINT("Unit1: CPU=%u%%, Mem=%u bytes\n", units[0].stats.cpuUsage, units[0].stats.memoryUsage);
  TC_PRINT("Unit2: CPU=%u%%, Mem=%u bytes\n", units[1].stats.cpuUsage, units[1].stats.memoryUsage);

  // Test invalid index
  UnitRegistry::updateUnitStats(99, 100, 4096); // Should not crash
}

ZTEST_SUITE(unit_tests, nullptr, NULL, NULL, NULL, NULL);

static void *registry_setup(void) {
  UnitRegistry::reset();
  return nullptr;
}

ZTEST_SUITE(unit_registry_tests, nullptr, registry_setup, NULL, NULL, NULL);

}
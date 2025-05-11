#ifndef TESTUNIT1_HPP
#define TESTUNIT1_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
using namespace OF;

// 测试Unit实现1
class TestUnit1 final : public Unit
{
public:
    AUTO_UNIT_TYPE(TestUnit1)

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

#endif // TESTUNIT1_HPP

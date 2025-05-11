
#ifndef TESTUNIT2_HPP
#define TESTUNIT2_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
using namespace OF;

class TestUnit2 final : public Unit
{
public:
    AUTO_UNIT_TYPE(TestUnit2)
    void init() override {}
    void run() override {}

    static consteval std::string_view name() { return "TestUnit2"; }
    static consteval std::string_view description() { return "Second test unit"; }
    static consteval size_t stackSize() { return 4096; }
    static consteval uint8_t priority() { return 10; }
};


#endif // TESTUNIT2_HPP

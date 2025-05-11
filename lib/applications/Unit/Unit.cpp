#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>

namespace OF
{

    void Unit::cleanup() {}

    Unit::~Unit() = default;

    void StartUnits()
    {
        UnitRegistry::initialize();
        const auto units = UnitRegistry::createAllUnits();
        UnitThreadManager::initializeThreads(units);
        static std::vector<std::unique_ptr<Unit>> g_units = units;
    }


} // namespace OF

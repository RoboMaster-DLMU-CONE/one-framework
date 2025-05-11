#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>

namespace OF
{

    void Unit::cleanup() {}

    Unit::~Unit() = default;

    static std::vector<std::unique_ptr<Unit>> g_units;

    void StartUnits()
    {
        static bool initialized = false;
        if (initialized)
        {
            return;
        }

        UnitRegistry::initialize();
        g_units = std::move(UnitRegistry::createAllUnits());
        UnitThreadManager::initializeThreads(g_units);
        initialized = true;
    }


} // namespace OF

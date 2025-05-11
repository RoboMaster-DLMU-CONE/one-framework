#include "OF/lib/applications/Unit/UnitRegistry.hpp"

namespace OF
{
    std::vector<UnitInfo> UnitRegistry::g_unitInfos;
    std::vector<std::unique_ptr<Unit> (*)(void)> UnitRegistry::g_unitFactories;
    std::vector<UnitRegistry::UnitRegistrationFunction> UnitRegistry::g_registrationFunctions;

    std::span<const UnitInfo> UnitRegistry::getUnits() { return g_unitInfos; }

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

    const UnitInfo* UnitRegistry::findUnit(std::string_view name)
    {
        for (const auto& info : g_unitInfos)
        {
            if (info.name == name)
            {
                return &info;
            }
        }
        return nullptr;
    }

    void UnitRegistry::updateUnitStatus(size_t idx, bool running)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].isRunning = running;
        }
    }

    void UnitRegistry::updateUnitStats(size_t idx, uint32_t cpu, uint32_t mem)
    {
        if (idx < g_unitInfos.size())
        {
            g_unitInfos[idx].stats.cpuUsage = cpu;
            g_unitInfos[idx].stats.memoryUsage = mem;
        }
    }
} // namespace OF

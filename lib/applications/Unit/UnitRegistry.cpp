#include <OF/lib/applications/Unit/UnitRegistry.hpp>

namespace OF {

const UnitInfo &UnitRegistry::findUnit(const std::string_view name) {
  for (size_t i = 0; i < unitCount_; i++) {
    if (units_[i].name == name) {
      return units_[i];
    }
  }

  static UnitInfo notFound = {.name = "NotFound",
                              .description = "Unit not found",
                              .stackSize = 0,
                              .priority = 0,
                              .isRunning = false,
                              .stats = {}};
  return notFound;
}

void UnitRegistry::updateUnitStatus(const size_t idx, const bool running) {
  if (idx < unitCount_) {
    units_[idx].isRunning = running;
  }
}

void UnitRegistry::updateUnitStats(const size_t idx, const uint32_t cpu,
                                   const uint32_t mem) {
  if (idx < unitCount_) {
    units_[idx].stats.cpuUsage = cpu;
    units_[idx].stats.memoryUsage = mem;
  }
}

} // namespace OF
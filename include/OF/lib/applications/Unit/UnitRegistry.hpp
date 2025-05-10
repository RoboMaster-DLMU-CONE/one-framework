#ifndef UNITREGISTRY_HPP
#define UNITREGISTRY_HPP
#include "Unit.hpp"
#include "UnitInfo.hpp"
#include <array>

namespace OF {
class UnitRegistry {
private:
  static constinit inline std::array<UnitInfo, CONFIG_MAX_UNIT> units_{};
  static inline size_t unitCount_{0};

public:
  template <typename T> consteval static size_t registerUnit() {
    static_assert(std::is_base_of_v<Unit, T>, "");
    const size_t idx = unitCount_++;

    units_[idx] = {.name = T::name(),
                   .description = T::description(),
                   .stackSize = T::stackSize(),
                   .priority = T::priority(),
                   .isRunning = false,
                   .stats = {}};
    return idx;
  }
  static const auto &getUnits() { return units_; }
  static size_t getUnitCount() { return unitCount_; }
  static const UnitInfo &findUnit(std::string_view name);
  static void updateUnitStatus(size_t idx, bool running);
  static void updateUnitStats(size_t idx, uint32_t cpu, uint32_t mem);
};
} // namespace OF

#endif // UNITREGISTRY_HPP

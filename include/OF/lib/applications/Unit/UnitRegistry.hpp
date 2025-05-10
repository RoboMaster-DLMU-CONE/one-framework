#ifndef UNITREGISTRY_HPP
#define UNITREGISTRY_HPP
#include "Unit.hpp"
#include "UnitInfo.hpp"
#include <array>

namespace OF {
class UnitRegistry {
private:
  static inline std::array<UnitInfo, CONFIG_MAX_UNIT> units_{};
  static inline size_t unitCount_{0};

public:
  // Registration happens at runtime but uses compile-time unit properties
  template <typename T> static size_t registerUnit() {
    static_assert(std::is_base_of_v<Unit, T>, "Must inherit from Unit");

    // This is non-constexpr but it's fine since the function is not constexpr
    const size_t idx = unitCount_++;

    // These properties are still computed at compile time
    units_[idx] = {.name = T::name(),
                   .description = T::description(),
                   .stackSize = T::stackSize(),
                   .priority = T::priority(),
                   .isRunning = false,
                   .stats = {}};
    return idx;
  }
  static void reset() { unitCount_ = 0; }
  static const auto &getUnits() { return units_; }
  static size_t getUnitCount() { return unitCount_; }
  static const UnitInfo &findUnit(std::string_view name);
  static void updateUnitStatus(size_t idx, bool running);
  static void updateUnitStats(size_t idx, uint32_t cpu, uint32_t mem);
};
} // namespace OF

#endif // UNITREGISTRY_HPP
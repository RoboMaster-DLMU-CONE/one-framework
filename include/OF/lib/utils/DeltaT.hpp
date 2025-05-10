#ifndef DELTAT_HPP
#define DELTAT_HPP
#include <concepts>
#include <zephyr/kernel.h>

/**
 * @namespace OF
 * @brief One Framework namespace
 */
namespace OF {
template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

static constexpr uint64_t CYCLES_PER_SEC = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

/**
 * @class DeltaT
 * @brief Class for measuring time differences
 * @tparam T The arithmetic type for time values
 */
template <Arithmetic T = double> class DeltaT {
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
  uint64_t last_time_cycles;
#else
  uint32_t last_time_cycles;
#endif

public:
  DeltaT()
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
      : last_time_cycles(k_cycle_get_64())
#else
      : last_time_cycles(k_cycle_get_32())
#endif
  {
  }

  T getDeltaMS() {
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    const uint64_t current_cycles = k_cycle_get_64();
#else
    const uint32_t current_cycles = k_cycle_get_32();
#endif
    const auto delta_cycles = current_cycles - last_time_cycles;
    last_time_cycles = current_cycles;

    double ms_value =
        static_cast<double>(delta_cycles * 1000.0 / CYCLES_PER_SEC);

    if constexpr (std::is_integral_v<T>) {
      return ms_value < 1.0 ? (delta_cycles > 0 ? 1 : 0)
                            : static_cast<T>(ms_value);
    } else {
      return static_cast<T>(ms_value);
    }
  }

  void reset() {
#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    last_time_cycles = k_cycle_get_64();
#else
    last_time_cycles = k_cycle_get_32();
#endif
  }
};
} // namespace OF

#endif // DELTAT_HPP

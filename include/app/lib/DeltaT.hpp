#ifndef DELTAT_HPP
#define DELTAT_HPP
#include <concepts>
#include <zephyr/kernel.h>


template<typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

template<Arithmetic T>
T getDeltaT(T *last_time) {
    T current_time = static_cast<T>(k_uptime_get());
    T delta = current_time - *last_time;
    if (delta <= static_cast<T>(0)) {
        delta = static_cast<T>(1);
    }

    *last_time = current_time;

    return delta;
}

template<Arithmetic T>
T getDeltaTCycles(T *last_time) {
    uint64_t current_cycles = k_cycle_get_64();
    T current_time = static_cast<T>(current_cycles);
    T delta = current_time - *last_time;
    *last_time = current_time;

    return delta;
}

#endif //DELTAT_HPP

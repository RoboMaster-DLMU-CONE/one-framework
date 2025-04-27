#ifndef PID_HPP
#define PID_HPP
#include <concepts>
#include <algorithm>
#include <numbers>
#include <app/lib/DeltaT.hpp>

template<bool... Bs>
concept AnyEnabled = (Bs || ...);

template<typename Algorithm>
concept PositionalAlgorithm = std::same_as<Algorithm, struct PositionalTag>;

template<typename Algorithm>
concept IncrementalAlgorithm = std::same_as<Algorithm, struct IncrementalTag>;

// 算法标签
struct PositionalTag {
};

struct IncrementalTag {
};

// 滤波器策略
template<Arithmetic T>
struct NoFilter {
    T operator()(T value) const { return value; }
};

template<Arithmetic T>
struct FirstOrderFilter {
    T operator()(T input, T prev) {
        return prev + alpha * (input - prev);
    }

    T alpha = 0.1;
};

// 主PID类
template<
    typename Algorithm,
    Arithmetic T,
    T Kp, T Ki, T Kd,
    typename OutputType = double,
    bool EnableDerivativeAhead = false,
    bool EnableDerivativeFilter = false,
    bool EnableOutputFilter = false,
    bool EnableOutputLimit = false,
    template<Arithmetic> class DerivativeFilter = NoFilter,
    template<Arithmetic> class OutputFilter = NoFilter
>
    requires (
        PositionalAlgorithm<Algorithm> || IncrementalAlgorithm<Algorithm>
    )
class PIDController {
    static_assert(AnyEnabled<EnableDerivativeAhead, EnableDerivativeFilter,
                      EnableOutputFilter, EnableOutputLimit> ||
                  !AnyEnabled<EnableDerivativeAhead, EnableDerivativeFilter,
                      EnableOutputFilter, EnableOutputLimit>,
                  "At least one optional feature must be enabled or all disabled");

    using ValueType = decltype(Kp + Ki + Kd);

    // 状态变量
    ValueType integral{};
    ValueType prev_error{};
    ValueType prev_ref{};

    // 滤波器状态
    DerivativeFilter<ValueType> derivativeFilter{};
    OutputFilter<ValueType> outputFilter{};

    // 限幅参数
    ValueType min_output{};
    ValueType max_output{};

    ValueType alpha = 0.1;

public:
    PIDController(ValueType min_out, ValueType max_out)
        : min_output(min_out), max_output(max_out) {
    }

    ValueType compute(ValueType ref, ValueType measure) {
        const ValueType error = ref - measure;
        dt = getDeltaT(&time_point);
        // 计算核心PID参数
        ValueType P = Kp * error;
        ValueType I = Ki * error * dt;

        // 微分计算（支持微分先行）
        ValueType D = [&] {
            if constexpr (EnableDerivativeAhead) {
                ValueType meas_deriv = -(measure - prev_measure) / dt;
                prev_measure = measure;
                return meas_deriv;
            } else {
                ValueType deriv = (error - prev_error) / dt;
                return deriv;
            }
        }();

        // 应用微分滤波
        if constexpr (EnableDerivativeFilter) {
            D = derivativeFilter(D, prev_derivative);
        }

        ValueType output = P + I + (Kd * D);

        // 应用输出滤波
        if constexpr (EnableOutputFilter) {
            output = outputFilter(output, prev_output);
        }

        // 应用输出限幅
        if constexpr (EnableOutputLimit) {
            output = std::clamp(output, min_output, max_output);
        }

        prev_ref = (ref);
        prev_derivative = D;
        prev_output = output;
        prev_error = error;

        return output;
    }

private:
    // 滤波器状态变量
    ValueType prev_measure{};
    ValueType prev_derivative{};
    ValueType prev_output{};
    ValueType dt{};
    ValueType time_point{};
};
#endif //PID_HPP

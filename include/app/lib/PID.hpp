#ifndef PID_HPP
#define PID_HPP
#include <concepts>
#include <algorithm>
#include <numbers>

// 概念定义
template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

template <bool... Bs>
concept AnyEnabled = (Bs || ...);

template <typename Algorithm>
concept PositionalAlgorithm = std::same_as<Algorithm, struct PositionalTag>;

template <typename Algorithm>
concept IncrementalAlgorithm = std::same_as<Algorithm, struct IncrementalTag>;

// 算法标签
struct PositionalTag
{
};

struct IncrementalTag
{
};

// 滤波器策略
template <Arithmetic T>
struct NoFilter
{
    T operator()(T value) const { return value; }
};

template <Arithmetic T>
struct FirstOrderFilter
{
    T operator()(T input, T prev)
    {
        return prev + alpha * (input - prev);
    }

    T alpha = 0.1;
};

// 主PID类
template <
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
class PIDController
{
    static_assert(AnyEnabled<EnableDerivativeAhead, EnableDerivativeFilter,
                             EnableOutputFilter, EnableOutputLimit> ||
                  !AnyEnabled<EnableDerivativeAhead, EnableDerivativeFilter,
                              EnableOutputFilter, EnableOutputLimit>,
                  "At least one optional feature must be enabled or all disabled");

    using ValueType = decltype(Kp + Ki + Kd);

    // 状态变量
    ValueType integral{};
    ValueType prev_error{};
    ValueType prev_setpoint{};

    // 滤波器状态
    DerivativeFilter<ValueType> derivativeFilter{};
    OutputFilter<ValueType> outputFilter{};

    // 限幅参数
    ValueType min_output{};
    ValueType max_output{};

    // 配置参数
    ValueType dt{};
    ValueType alpha = 0.1;

public:
    PIDController(ValueType dt, ValueType min_out, ValueType max_out)
        : min_output(min_out), max_output(max_out), dt(dt)
    {
    }

    template <Arithmetic Setpoint>
    ValueType compute(Setpoint input)
    {
        const ValueType error = static_cast<ValueType>(input) - prev_setpoint;

        // 计算核心PID参数
        ValueType P = Kp * error;
        ValueType I = integral += Ki * error * dt;

        // 微分计算（支持微分先行）
        ValueType derivative = [&]
        {
            if constexpr (EnableDerivativeAhead)
            {
                ValueType sp_deriv = (static_cast<ValueType>(input) - prev_setpoint) / dt;
                prev_setpoint = static_cast<ValueType>(input);
                return sp_deriv;
            }
            else
            {
                ValueType deriv = (error - prev_error) / dt;
                prev_error = error;
                return deriv;
            }
        }();

        // 应用微分滤波
        if constexpr (EnableDerivativeFilter)
        {
            derivative = derivativeFilter(derivative, prev_derivative);
            prev_derivative = derivative;
        }

        ValueType output = P + I + (Kd * derivative);

        // 应用输出滤波
        if constexpr (EnableOutputFilter)
        {
            output = outputFilter(output, prev_output);
            prev_output = output;
        }

        // 应用输出限幅
        if constexpr (EnableOutputLimit)
        {
            output = std::clamp(output, min_output, max_output);
        }

        return output;
    }

private:
    // 滤波器状态变量
    ValueType prev_derivative{};
    ValueType prev_output{};
};
#endif //PID_HPP

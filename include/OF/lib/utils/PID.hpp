#ifndef PID_HPP
#define PID_HPP
#include <algorithm>
#include <OF/lib/utils/DeltaT.hpp>
#include <numeric>
#include <optional>


// 算法
struct Positional
{
};

struct Incremental
{
};

// 特性标签
struct WithDeadband // 死区
{
};

struct WithIntegralLimit // 积分限幅
{
};

struct WithDerivativeOnMeasurement // 微分先行
{
};

struct WithDerivativeFilter // 微分项滤波
{
};

struct WithOutputFilter // 输出滤波
{
};

struct WithOutputLimit // 输出限幅
{
};

// 主PID类
template <
    typename Algorithm = Positional,
    Arithmetic ValueType = float,
    typename... Features
>
class PIDController
{
    // 特性检查
    static constexpr bool PositionalPID = std::is_same_v<Algorithm, Positional>;
    static constexpr bool HasDeadband = (std::is_same_v<Features, WithDeadband> || ...);
    static constexpr bool HasIntegralLimit = (std::is_same_v<Features, WithIntegralLimit> || ...);
    static constexpr bool HasDerivativeOnMeasurement = (std::is_same_v<Features, WithDerivativeOnMeasurement> || ...);
    static constexpr bool HasDerivativeFilter = (std::is_same_v<Features, WithDerivativeFilter> || ...);
    static constexpr bool HasOutputFilter = (std::is_same_v<Features, WithOutputFilter> || ...);
    static constexpr bool HasOutputLimit = (std::is_same_v<Features, WithOutputLimit> || ...);

    static constexpr ValueType D_RC = static_cast<ValueType>(CONFIG_PID_Derivative_LPF_RC_M1K) / static_cast<ValueType>(
        1000.0);
    static constexpr ValueType O_RC = static_cast<ValueType>(CONFIG_PID_Output_LPF_RC_M1K) / static_cast<ValueType>(
        1000.0);
    const ValueType MaxOutput;
    const ValueType Deadband;
    const ValueType IntegralLimit;
    const ValueType Kp;
    const ValueType Ki;
    const ValueType Kd;

    // 状态变量
    ValueType ITerm{};
    ValueType prev_error{};
    ValueType prev_prev_error{};
    ValueType prev_ref{};

public:
    explicit PIDController(ValueType Kp, ValueType Ki, ValueType Kd,
                           std::optional<ValueType> MaxOutput = std::nullopt,
                           std::optional<ValueType> Deadband = std::nullopt,
                           std::optional<ValueType> IntegralLimit = std::nullopt
    )
        : MaxOutput(MaxOutput.value_or(std::numeric_limits<ValueType>::max())),
          Deadband(Deadband.value_or(std::numeric_limits<ValueType>::max())),
          IntegralLimit(IntegralLimit.value_or(std::numeric_limits<ValueType>::max())),
          Kp(Kp), Ki(Ki), Kd(Kd),
          deltaT()
    {
    }

    ValueType compute(ValueType ref, ValueType measure)
    {
        const ValueType error = ref - measure;
        if constexpr (HasDeadband)
        {
            if (std::abs(error) <= Deadband)
            {
                return prev_output;
            }
        }
        ValueType dt = deltaT.getDeltaMS();
        ValueType P, I{}, D, output;
        if constexpr (PositionalPID)
        {
            P = Kp * error;
            ITerm = Ki * error * dt;
        }
        else
        {
            P = Kp * (error - prev_error);
            ITerm = Ki * error * dt;
        }

        D = [&]
        {
            if constexpr (HasDerivativeOnMeasurement)
            {
                ValueType meas_deriv = -(measure - prev_measure) / dt;
                prev_measure = measure;
                return meas_deriv;
            }
            else if constexpr (PositionalPID)
            {
                return (error - prev_error) / dt;
            }
            else
            {
                return (error - 2 * prev_error + prev_prev_error) / dt;
            }
        }();
        D *= Kd;

        // 应用微分滤波
        if constexpr (HasDerivativeFilter)
        {
            D = D * dt / (D_RC + dt) + prev_derivative * D_RC / (D_RC + dt);
        }
        // 应用积分限幅
        if constexpr (HasIntegralLimit)
        {
            float temp_Iout = I + ITerm;
            if (const float temp_Output = P + I + D; abs(temp_Output) > MaxOutput)
            {
                if (error * I > 0)
                {
                    ITerm = 0;
                }
            }
            if (temp_Iout > IntegralLimit)
            {
                ITerm = 0;
                I = IntegralLimit;
            }
            if (temp_Iout < -IntegralLimit)
            {
                ITerm = 0;
                I = -IntegralLimit;
            }
        }

        I += ITerm;
        output = P + I + D;


        // 应用输出滤波
        if constexpr (HasOutputFilter)
        {
            output = output * dt / (O_RC + dt) + prev_output * O_RC / (O_RC + dt);
        }

        // 应用输出限幅
        if constexpr (HasOutputLimit)
        {
            output = std::clamp(output, -MaxOutput, MaxOutput);
        }

        prev_ref = ref;
        prev_derivative = D;
        prev_output = output;
        prev_error = error;

        return output;
    }

    void reset()
    {
        ITerm = ValueType{};
        prev_error = ValueType{};
        prev_prev_error = ValueType{};
        prev_ref = ValueType{};
        prev_measure = ValueType{};
        prev_derivative = ValueType{};
        prev_output = ValueType{};

        deltaT.reset();
    }

private:
    // 滤波器状态变量
    ValueType prev_measure{};
    ValueType prev_derivative{};
    ValueType prev_output{};
    DeltaT<ValueType> deltaT;
};
#endif //PID_HPP

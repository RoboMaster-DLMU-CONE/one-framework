#include "OF/lib/utils/PID/pid_c_api.h"
#include "OF/lib/utils/PID/PID.hpp"
#include <memory>

using namespace OF;

class PIDInterface
{
public:
    virtual ~PIDInterface() = default;
    virtual float compute(float reference, float measurement) = 0;
    virtual void reset() = 0;
};

// 使用模板类实现类型擦除
template <typename... Args>
class PIDWrapper final : public PIDInterface
{
private:
    PIDController<Args...> controller;

public:
    template <typename... Params>
    explicit PIDWrapper(Params&&... params) :
        controller(std::forward<Params>(params)...)
    {
    }

    float compute(float reference, float measurement) override
    {
        return controller.compute(reference, measurement);
    }

    void reset() override
    {
        controller.reset();
    }
};

struct pid_controller_s
{
    std::unique_ptr<PIDInterface> controller;
};

pid_controller_t pid_create(const pid_type_t type, float kp, float ki, float kd,
                            const unsigned int features,
                            float max_output, float deadband, float integral_limit)
{
    const auto handle = new pid_controller_s();
    // 准备参数
    std::optional<float> max_output_opt = (features & PID_FEATURE_OUTPUT_LIMIT)
        ? std::optional(max_output)
        : std::nullopt;
    std::optional<float> deadband_opt = (features & PID_FEATURE_DEADBAND)
        ? std::optional(deadband)
        : std::nullopt;
    std::optional<float> integral_limit_opt = (features & PID_FEATURE_INTEGRAL_LIMIT)
        ? std::optional(integral_limit)
        : std::nullopt;

    // 位置式PID
    if (type == PID_POSITIONAL)
    {
        if (features == PID_FEATURE_NONE)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        // 单特性
        else if (features == PID_FEATURE_DEADBAND)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDeadband>>(
                kp, ki, kd, std::nullopt, deadband_opt, std::nullopt);
        }
        else if (features == PID_FEATURE_INTEGRAL_LIMIT)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithIntegralLimit>>(
                kp, ki, kd, std::nullopt, std::nullopt, integral_limit_opt);
        }
        else if (features == PID_FEATURE_OUTPUT_LIMIT)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithOutputLimit>>(
                kp, ki, kd, max_output_opt, std::nullopt, std::nullopt);
        }
        else if (features == PID_FEATURE_OUTPUT_FILTER)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithOutputFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        else if (features == PID_FEATURE_DERIVATIVE_FILTER)
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDerivativeFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        // 双特性组合
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDeadband, WithOutputLimit>>(
                kp, ki, kd, max_output_opt, deadband_opt, std::nullopt);
        }
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_INTEGRAL_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDeadband, WithIntegralLimit>>(
                kp, ki, kd, std::nullopt, deadband_opt, integral_limit_opt);
        }
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_DERIVATIVE_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDeadband, WithDerivativeFilter>>(
                kp, ki, kd, std::nullopt, deadband_opt, std::nullopt);
        }
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithDeadband, WithOutputFilter>>(
                kp, ki, kd, std::nullopt, deadband_opt, std::nullopt);
        }
        else if ((features & PID_FEATURE_OUTPUT_LIMIT) && (features & PID_FEATURE_INTEGRAL_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithOutputLimit, WithIntegralLimit>>(
                kp, ki, kd, max_output_opt, std::nullopt, integral_limit_opt);
        }
        else if ((features & PID_FEATURE_OUTPUT_LIMIT) && (features & PID_FEATURE_DERIVATIVE_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithOutputLimit, WithDerivativeFilter>>(
                kp, ki, kd, max_output_opt, std::nullopt, std::nullopt);
        }
        else if ((features & PID_FEATURE_OUTPUT_LIMIT) && (features & PID_FEATURE_OUTPUT_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithOutputLimit, WithOutputFilter>>(
                kp, ki, kd, max_output_opt, std::nullopt, std::nullopt);
        }
        else if ((features & PID_FEATURE_INTEGRAL_LIMIT) && (features & PID_FEATURE_DERIVATIVE_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<
                Positional, float, WithIntegralLimit, WithDerivativeFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, integral_limit_opt);
        }
        else if ((features & PID_FEATURE_INTEGRAL_LIMIT) && (features & PID_FEATURE_OUTPUT_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<Positional, float, WithIntegralLimit, WithOutputFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, integral_limit_opt);
        }
        else if ((features & PID_FEATURE_DERIVATIVE_FILTER) && (features & PID_FEATURE_OUTPUT_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<
                Positional, float, WithDerivativeFilter, WithOutputFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        // 三特性组合 (常见组合)
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_LIMIT) && (features &
            PID_FEATURE_INTEGRAL_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<
                Positional, float, WithDeadband, WithOutputLimit, WithIntegralLimit>>(
                kp, ki, kd, max_output_opt, deadband_opt, integral_limit_opt);
        }
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_LIMIT) && (features &
            PID_FEATURE_DERIVATIVE_FILTER))
        {
            handle->controller = std::make_unique<PIDWrapper<
                Positional, float, WithDeadband, WithOutputLimit, WithDerivativeFilter>>(
                kp, ki, kd, max_output_opt, deadband_opt, std::nullopt);
        }
        // 默认情况：使用所有特性
        else
        {
            handle->controller = std::make_unique<PIDWrapper<
                Positional, float, WithDeadband, WithOutputLimit, WithIntegralLimit, WithDerivativeFilter,
                WithOutputFilter>>(
                kp, ki, kd, max_output_opt, deadband_opt, integral_limit_opt);
        }
    }
    // 增量式PID
    else
    {
        if (features == PID_FEATURE_NONE)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        // 单特性
        else if (features == PID_FEATURE_DEADBAND)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithDeadband>>(
                kp, ki, kd, std::nullopt, deadband_opt, std::nullopt);
        }
        else if (features == PID_FEATURE_INTEGRAL_LIMIT)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithIntegralLimit>>(
                kp, ki, kd, std::nullopt, std::nullopt, integral_limit_opt);
        }
        else if (features == PID_FEATURE_OUTPUT_LIMIT)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithOutputLimit>>(
                kp, ki, kd, max_output_opt, std::nullopt, std::nullopt);
        }
        else if (features == PID_FEATURE_OUTPUT_FILTER)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithOutputFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        else if (features == PID_FEATURE_DERIVATIVE_FILTER)
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithDerivativeFilter>>(
                kp, ki, kd, std::nullopt, std::nullopt, std::nullopt);
        }
        // 双特性组合
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithDeadband, WithOutputLimit>>(
                kp, ki, kd, max_output_opt, deadband_opt, std::nullopt);
        }
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_INTEGRAL_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<Incremental, float, WithDeadband, WithIntegralLimit>>(
                kp, ki, kd, std::nullopt, deadband_opt, integral_limit_opt);
        }
        // 更多双特性组合...与位置式PID类似

        // 三特性组合 (常见组合)
        else if ((features & PID_FEATURE_DEADBAND) && (features & PID_FEATURE_OUTPUT_LIMIT) && (features &
            PID_FEATURE_INTEGRAL_LIMIT))
        {
            handle->controller = std::make_unique<PIDWrapper<
                Incremental, float, WithDeadband, WithOutputLimit, WithIntegralLimit>>(
                kp, ki, kd, max_output_opt, deadband_opt, integral_limit_opt);
        }
        // 默认情况：使用所有特性
        else
        {
            handle->controller = std::make_unique<PIDWrapper<
                Incremental, float, WithDeadband, WithOutputLimit, WithIntegralLimit, WithDerivativeFilter,
                WithOutputFilter>>(
                kp, ki, kd, max_output_opt, deadband_opt, integral_limit_opt);
        }
    }

    return handle;
}

// ReSharper disable once CppParameterMayBeConst
float pid_compute(pid_controller_t pid, const float reference, const float measurement)
{
    if (!pid || !pid->controller)
        return 0.0f;
    return pid->controller->compute(reference, measurement);
}

// ReSharper disable once CppParameterMayBeConst
void pid_reset(pid_controller_t pid)
{
    if (!pid || !pid->controller)
        return;
    pid->controller->reset();
}

// ReSharper disable once CppParameterMayBeConst
void pid_destroy(pid_controller_t pid)
{
    if (!pid)
        return;
    delete pid;
}

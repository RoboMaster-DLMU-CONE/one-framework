#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <app/lib/PID.hpp>
#include <cmath>

extern "C" {
// 测试基本位置式PID控制器

ZTEST(pid_controller, test_positional_basic)
{
    // 创建一个简单的位置式PID控制器
    PIDController pid(1.0f, 0.1f, 0.05f);

    float measurement = 0.0f;
    float output = pid.compute(10.0f, measurement);
    zassert_true(output > 0, "位置式PID应该输出正值");

    // 进行更多次迭代，观察是否最终接近目标值
    for (int i = 0; i < 50; i++)
    {
        measurement += output * 0.1f; // 简单系统响应
        output = pid.compute(10.0f, measurement);
        TC_PRINT("measure: %f, output: %f\n", static_cast<double>(measurement), static_cast<double>(output));
    }

    // 检查最终结果是否接近目标值
    zassert_true(std::abs(measurement - 10.0f) < 5.0f, "PID控制器应该使系统接近目标值");

    // 检查最终输出是否稳定
    const float last_output = output;
    measurement += output * 0.1f;
    output = pid.compute(10.0f, measurement);
    zassert_true(std::abs(output - last_output) < 0.5f, "PID控制器应该最终稳定");
}

// 测试微分先行特性
ZTEST(pid_controller, test_derivative_on_measurement)
{
    // 创建带微分先行的PID控制器
    PIDController<Positional, float,
                  WithDerivativeOnMeasurement> pid(1.0f, 0.0f, 1.0f);

    TC_PRINT("first output: %f\n", static_cast<double>(pid.compute(0.0f, 0.0f)));
    k_sleep(K_MSEC(1));

    // 设定点突变
    const float output = pid.compute(10.0f, 0.0f);
    TC_PRINT("second output: %f\n", static_cast<double>(output));


    // 使用常规PID进行比较
    PIDController regular_pid(1.0f, 0.0f, 1.0f);
    TC_PRINT("first output: %f\n", static_cast<double>(regular_pid.compute(0.0f, 0.0f)));
    k_sleep(K_MSEC(1));
    const float regular_output = regular_pid.compute(10.0f, 0.0f);
    TC_PRINT("second output: %f\n", static_cast<double>(regular_output));


    // 微分先行应该产生更小的输出跳变
    zassert_true(std::abs(output) < std::abs(regular_output),
                 "微分先行应该减少输出突变");
}

// 测试整数类型PID
ZTEST(pid_controller, test_integer_pid)
{
    // 创建使用整数类型的PID控制器
    PIDController pid(1, 0, 0);

    int output = pid.compute(100, 0);
    zassert_equal(output, 100, "整数类型PID计算错误");

    output = pid.compute(-50, 0);
    zassert_equal(output, -50, "整数类型PID计算错误");
}

// 测试带输出滤波的PID
ZTEST(pid_controller, test_output_filter)
{
    // 创建带输出滤波的PID控制器
    PIDController<Positional, float,
                  WithOutputFilter> pid(1.0f, 0.0f, 0.0f);

    // 突变测试
    const float output1 = pid.compute(10.0f, 0.0f);
    const float output2 = pid.compute(10.0f, 0.0f);

    // 滤波效果应该使第二次输出更接近设定值
    zassert_true(std::abs(output2 - 10.0f) < std::abs(output1 - 10.0f),
                 "滤波应该平滑输出");
}

// 测试基本增量式PID控制器
ZTEST(pid_controller, test_incremental_basic)
{
    // 创建一个简单的增量式PID控制器
    PIDController<Incremental, float> pid(1.0f, 0.1f, 0.05f);

    float measurement = 0.0f;
    float output = pid.compute(10.0f, measurement);
    zassert_true(output > 0, "增量式PID应该输出正值");

    // 进行更多次迭代，观察是否最终接近目标值
    for (int i = 0; i < 50; i++)
    {
        measurement += output * 0.1f; // 简单系统响应
        output = pid.compute(10.0f, measurement);
        TC_PRINT("Incremental PID: measure: %f, output: %f\n", static_cast<double>(measurement),
                 static_cast<double>(output));
    }

    // 检查最终结果是否接近目标值
    zassert_true(std::abs(measurement - 10.0f) < 5.0f, "增量式PID控制器应该使系统接近目标值");
}

// 比较位置式和增量式PID的响应
ZTEST(pid_controller, test_positional_vs_incremental)
{
    PIDController pos_pid(1.0f, 0.1f, 0.05f);
    PIDController<Incremental, float> inc_pid(1.0f, 0.1f, 0.05f);
    float pos_measure = 0.0f;
    float inc_measure = 0.0f;
    for (int i = 0; i < 20; i++)
    {
        const float pos_output = pos_pid.compute(10.0f, pos_measure);
        const float inc_output = inc_pid.compute(10.0f, inc_measure);
        pos_measure += pos_output * 0.1f;
        inc_measure += inc_output * 0.1f;
        TC_PRINT("step %d - positional: %f, incremental: %f\n", i, static_cast<double>(pos_measure),
                 static_cast<double>(inc_measure));
    }
    // 增量式通常有更平滑的输出变化
    zassert_true(true, "仅用于显示两种算法的比较结果");
}

// 测试增量式PID的输出限幅
ZTEST(pid_controller, test_incremental_with_output_limit)
{
    // 创建带输出限幅的增量式PID控制器
    PIDController<Incremental, float, WithOutputLimit> pid(10.0f, 0.0f, 0.0f, 5.0f);
    // 设置较大的偏差，应该触发限幅
    const float output = pid.compute(10.0f, 0.0f);

    // 输出不应超过限幅
    zassert_true(output <= 5.0f, "增量式PID应该遵守输出限幅");
    zassert_equal(output, 5.0f, "对于大偏差，输出应该达到限幅上限");
}

ZTEST_SUITE(pid_controller, NULL, NULL, NULL, NULL, NULL);
}

#include <zephyr/ztest.h>
#include <app/lib/PID.hpp>
#include <cmath>
#include <limits>

// 算法标签实例
static constexpr PositionalTag posAlgo{};
static constexpr IncrementalTag incAlgo{};

extern "C" {

// 测试基本位置式PID控制器
ZTEST(pid_controller, test_positional_basic)
{
    // 创建一个简单的位置式PID控制器
    PIDController<PositionalTag, float, 1.0f, 0.1f, 0.05f> pid(0.1f, -100.0f, 100.0f);

    // 初始值为0，设定值为10，应该产生正向输出
    float output = pid.compute(10.0f);
    zassert_true(output > 0, "位置式PID应该输出正值");

    // 连续几个周期，误差应该减小
    float prev_output = output;
    for (int i = 0; i < 5; i++) {
        output = pid.compute(10.0f);
        zassert_true(std::abs(output - 10.0f) < std::abs(prev_output - 10.0f),
                     "PID算法应当收敛");
        prev_output = output;
    }
}

// 测试PID控制器的输出限幅功能
ZTEST(pid_controller, test_output_limit)
{
    // 创建带输出限幅的PID控制器
    PIDController<PositionalTag, float, 10.0f, 0.0f, 0.0f,
                 double, false, false, false, true> pid(0.1f, -5.0f, 5.0f);

    // 大幅度跳变应触发限幅
    float output = pid.compute(100.0f);
    zassert_equal(output, 5.0f, "输出应该被限制在最大值");

    // 负向大幅度跳变
    output = pid.compute(-100.0f);
    zassert_equal(output, -5.0f, "输出应该被限制在最小值");
}

// 测试微分先行特性
ZTEST(pid_controller, test_derivative_on_measurement)
{
    // 创建带微分先行的PID控制器
    PIDController<PositionalTag, float, 1.0f, 0.0f, 1.0f,
                 double, true, false, false, false> pid(0.1f, -100.0f, 100.0f);

    pid.compute(0.0f);

    // 设定点突变
    const float output2 = pid.compute(10.0f);

    // 使用常规PID进行比较
    PIDController<PositionalTag, float, 1.0f, 0.0f, 1.0f> regular_pid(0.1f, -100.0f, 100.0f);
    regular_pid.compute(0.0f);
    const float regular_output = regular_pid.compute(10.0f);

    // 微分先行应该产生更小的输出跳变
    zassert_true(std::abs(output2) < std::abs(regular_output),
                "微分先行应该减少输出突变");
}

// 测试整数类型PID
ZTEST(pid_controller, test_integer_pid)
{
    // 创建使用整数类型的PID控制器
    PIDController<PositionalTag, int, 1, 0, 0> pid(1, -1000, 1000);

    int output = pid.compute(100);
    zassert_equal(output, 100, "整数类型PID计算错误");

    output = pid.compute(-50);
    zassert_equal(output, -50, "整数类型PID计算错误");
}

// 测试带输出滤波的PID
ZTEST(pid_controller, test_output_filter)
{
    // 创建带输出滤波的PID控制器
    PIDController<PositionalTag, float, 1.0f, 0.0f, 0.0f,
                 double, false, false, true, false,
                 NoFilter, FirstOrderFilter> pid(0.1f, -100.0f, 100.0f);

    // 突变测试
    const float output1 = pid.compute(10.0f);
    const float output2 = pid.compute(10.0f);

    // 滤波效果应该使第二次输出更接近设定值
    zassert_true(std::abs(output2 - 10.0f) < std::abs(output1 - 10.0f),
                "滤波应该平滑输出");
}
    ZTEST_SUITE(pid_controller, NULL, NULL, NULL, NULL, NULL);

}


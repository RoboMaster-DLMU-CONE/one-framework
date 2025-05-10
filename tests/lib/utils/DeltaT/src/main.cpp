#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <OF/lib/utils/DeltaT.hpp>
#include <vector>
extern "C" {

// 测试基本计时功能
ZTEST(delta_t, test_basic_timing)
{
    DeltaT<> timer; // 默认使用double类型
    
    // 第一次调用应该返回较小的值
    const double delta1 = timer.getDeltaMS();
    zassert_true(delta1 >= 0.0, "Delta时间不应为负值");
    
    // 等待一段已知时间
    k_sleep(K_MSEC(100));
    
    // 再次获取delta，应该接近100ms
    const double delta2 = timer.getDeltaMS();
    zassert_true(delta2 >= 100.0, "Delta时间应接大于100ms");
}

// 测试reset功能
ZTEST(delta_t, test_reset)
{
    DeltaT<> timer;
    
    k_sleep(K_MSEC(100));
    const double delta1 = timer.getDeltaMS();
    zassert_true(delta1 >= 100.0, "休眠后Delta时间应大于100ms");
    
    // 重置计时器
    timer.reset();
    
    // 重置后应该返回较小的值
    const double delta2 = timer.getDeltaMS();
    zassert_true(delta2 < 2.0, "重置后Delta时间应较小");
}

// 测试使用不同数值类型
ZTEST(delta_t, test_different_types)
{
    // 测试float类型
    {
        DeltaT<float> timer;
        k_sleep(K_MSEC(100));
        const float delta = timer.getDeltaMS();
        zassert_true(delta >= 100.0f, "Float类型Delta应接近100ms");
    }
    
    // 测试int类型
    {
        DeltaT<int> timer;
        k_sleep(K_MSEC(100));
        const int delta = timer.getDeltaMS();
        zassert_true(delta >= 100, "Int类型Delta应接近100ms");
    }
}

// 测试连续调用的累计时间
ZTEST(delta_t, test_consecutive_calls)
{
    DeltaT<> timer;
    double total = 0.0;
    
    // 连续几次调用
    for (int i = 0; i < 4; i++) {
        k_sleep(K_MSEC(25));
        total += timer.getDeltaMS();
    }

    zassert_true(total >= 100.0, "四次25ms延迟应累计至少100ms");
}

// 测试非零保证
ZTEST(delta_t, test_nonzero_guarantee)
{
    DeltaT<> timer;
    DeltaT<uint32_t> timer1;
    
    // 连续多次快速调用，验证返回值始终大于0
    std::vector<double> t_vector;
    std::vector<uint32_t> t_vector1;
    for (int i = 0; i < 10; i++) {
        t_vector.push_back(timer.getDeltaMS());
        t_vector1.push_back(timer1.getDeltaMS());
    }
    for (int i = 0; i < 10; i++)
    {
        const auto time = t_vector[i];
        const auto time1 = t_vector1[i];
        TC_PRINT("%f ms %d ms\n", time, time1);
        zassert_true(time != 0 && time1 != 0, "DeltaT不能为0");
    }
}

ZTEST_SUITE(delta_t, NULL, NULL, NULL, NULL, NULL);

}
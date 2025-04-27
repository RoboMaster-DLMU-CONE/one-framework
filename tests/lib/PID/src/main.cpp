#include <zephyr/ztest.h>
#include <app/lib/PID.hpp>
#include <cmath>
#include <limits>

extern "C" {
    ZTEST(basic_test, test_simple)
    {
        zassert_true(1 == 1, "基本断言失败");
    }

    ZTEST_SUITE(basic_test, NULL, NULL, NULL, NULL, NULL);
}
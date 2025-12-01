// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/IMUCenter/IMUCenter.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>


LOG_MODULE_REGISTER(imu_test, CONFIG_LOG_DEFAULT_LEVEL);

using OF::IMUCenter;

int main()
{
    LOG_INF("main");
    while (true)
    {
        auto data = IMUCenter::getInstance().getData();
        auto& [x, y, z] = data.accel;
        LOG_INF("accel: %f, %f, %f;", x, y, z);
        auto& [a, b, c] = data.gyro;
        LOG_INF("gyro: %f, %f, %f;", a, b, c);
        k_sleep(K_MSEC(100));
    }
}

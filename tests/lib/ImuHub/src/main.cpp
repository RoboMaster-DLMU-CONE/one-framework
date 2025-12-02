// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/ImuHub/ImuHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "OF/lib/HubManager/HubManager.hpp"


LOG_MODULE_REGISTER(imu_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

int main()
{
    LOG_INF("main");

    HubManager::Builder().bind<ImuHub>(
    {
        DEVICE_DT_GET(DT_NODELABEL(bmi088_accel)),
        DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro))
    });
    HubManager::startAll();

    while (true)
    {
        auto data = ImuHub::getInstance().getData();
        auto& [x, y, z] = data.accel;
        LOG_INF("accel: %f, %f, %f;", x, y, z);
        auto& [a, b, c] = data.gyro;
        LOG_INF("gyro: %f, %f, %f;", a, b, c);
        k_sleep(K_MSEC(100));
    }
}

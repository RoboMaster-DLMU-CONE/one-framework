// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/ImuHub/ImuHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>

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
        LOG_INF("----");
        auto& [ax, ay, az] = data.accel;
        LOG_INF("accel: %f, %f, %f;", ax, ay, az);
        auto& [a, b, c] = data.gyro;
        LOG_INF("gyro: %f, %f, %f;", a, b, c);
        auto& [w, qx, qy, qz] = data.quat;
        LOG_INF("quat: %f, %f, %f, %f;", w, qx, qy, qz);
        auto& [p, r, y] = data.euler_angle;
        LOG_INF("euler: %f, %f, %f;", p, r, y);
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        k_sleep(K_MSEC(100));
    }
}

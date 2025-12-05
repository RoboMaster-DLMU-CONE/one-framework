// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/HubManager/HubRegistry.hpp>
#include <OF/lib/ImuHub/ImuHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>

LOG_MODULE_REGISTER(imu_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr ImuHubConfig imu_hub_config{
    .accel_dev = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel)),
    .gyro_dev = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro))
};

int main()
{
    LOG_INF("main");

    HubRegistry::startAll();

    const auto* imu_hub = getHub<ImuHub>();

    while (true)
    {
        auto [quat, euler_angle, gyro, accel] = imu_hub->getData();
        LOG_INF("----");
        auto& [ax, ay, az] = accel;
        LOG_INF("accel: %f, %f, %f;", ax, ay, az);
        auto& [a, b, c] = gyro;
        LOG_INF("gyro: %f, %f, %f;", a, b, c);
        auto& [w, qx, qy, qz] = quat;
        LOG_INF("quat: %f, %f, %f, %f;", w, qx, qy, qz);
        auto& [p, r, y] = euler_angle;
        LOG_INF("euler: %f, %f, %f;", p, r, y);
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        k_sleep(K_MSEC(500));
    }
}

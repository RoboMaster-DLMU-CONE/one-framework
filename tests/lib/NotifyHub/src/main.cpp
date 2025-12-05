// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <OF/lib/HubManager/HubRegistry.hpp>


LOG_MODULE_REGISTER(notify_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr NotifyHubConfig notify_hub_config{
    .status_leds_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds)),
    .pwm_buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer))
};


int main()
{
    LOG_INF("main");
    HubRegistry::startAll();

    while (true)
    {
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        k_sleep(K_MSEC(500));
    }
}

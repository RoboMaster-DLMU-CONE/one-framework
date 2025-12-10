// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/NotifyHub/Notify.hpp>
#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <OF/lib/HubManager/HubRegistry.hpp>

#include "OF/drivers/output/buzzer.h"


LOG_MODULE_REGISTER(notify_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr NotifyHubConfig notify_hub_config{
    .status_leds_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds)),
    .pwm_buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer))
};

using namespace ems::literals;
constexpr auto melody =
    R"((200)
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,,
        2s`,,1s`,7,,1s`,2s`-,3`-2s`,1s`,,7-1s`-
        2s`,2s`,1s`,3`,2s`,1s`,1s`,1s`,7,3`,2s`,1s`,
        1s`,,7-1s`-2s`,,,,,,7,4s`,7`,
        6s`,,7`,6s`,,7`,6s`-5s`-4s`,,4s`,1s`,3`,
        3`,2s`,2s`,2s`,,,3`,2s`,1s`,2s`,,4s`,
        7,,,0
)"_ems;

int main()
{
    LOG_INF("main");
    HubRegistry::startAll();

    const device* buzzer = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer));
    if (!device_is_ready(buzzer))
    {
        LOG_ERR("PWM蜂鸣器设备未就绪");
        return -1;
    }

    for (const auto& [ratio, dur] : melody)
    {
        pwm_buzzer_play_note(buzzer, ratio, 1);
        k_sleep(K_MSEC(dur));
        pwm_buzzer_stop(buzzer);
        k_sleep(K_MSEC(10));
    }
    pwm_buzzer_stop(buzzer);


    while (true)
    {
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        k_sleep(K_MSEC(500));
    }
}

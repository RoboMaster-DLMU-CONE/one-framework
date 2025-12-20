// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <autoconf.h>
#include <OF/lib/NotifyHub/Notify.hpp>
#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <OF/lib/HubManager/HubRegistry.hpp>
#include <zephyr/drivers/led.h>

#include "OF/drivers/output/buzzer.h"


LOG_MODULE_REGISTER(notify_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr NotifyHubConfig notify_hub_config{
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
    // HubRegistry::startAll();

    const device* led_dev = DEVICE_DT_GET(DT_NODELABEL(pwmleds));

    if (!device_is_ready(led_dev))
    {
        LOG_ERR("PWM_LED not ready");
        return 1;
    }


    while (true)
    {
        // uint32_t load = cpu_load_get(false);
        // LOG_INF("cpu: %u.%u%%", load / 10, load % 10);

        for (int ch = 0; ch < 3; ++ch)
        {
            LOG_INF("ch: %d", ch);
            for (int i = 0; i < 100; ++i)
            {
                const int ret = led_set_brightness(led_dev, ch, i);
                if (ret < 0)
                {
                    LOG_ERR("ch %d failed. %d", ch, ret);
                    goto next_ch;
                }
                k_sleep(K_MSEC(10));
            }
            k_sleep(K_MSEC(200));

            for (int i = 100; i > 0; --i)
            {
                led_set_brightness(led_dev, ch, i);
                k_sleep(K_MSEC(10));
            }
            k_sleep(K_MSEC(500));
        next_ch:;
        }
    }
}

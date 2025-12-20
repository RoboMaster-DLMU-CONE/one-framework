// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <autoconf.h>
#include <OF/lib/NotifyHub/Notify.hpp>
#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <OF/lib/HubManager/HubRegistry.hpp>
#include <OF/drivers/output/led_pixel.h>

#include "OF/drivers/output/buzzer.h"


LOG_MODULE_REGISTER(notify_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr NotifyHubConfig notify_hub_config{
    .led_pixel_dev = DEVICE_DT_GET(DT_NODELABEL(pixel_led)),
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

    const device* led_dev = DEVICE_DT_GET(DT_NODELABEL(pixel_led));

    if (!device_is_ready(led_dev))
    {
        LOG_ERR("LED not ready");
        return 1;
    }

    constexpr led_color c = COLOR_HEX("#d81159");
    int state = 0;


    while (true)
    {
        // uint32_t load = cpu_load_get(false);
        // LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        if (state++ == 0)
        {
            int ret = led_pixel_set(led_dev, c);
            if (ret < 0)
                LOG_ERR("failed to set pixel: %d", ret);
        }
        else
        {
            int ret = led_pixel_off(led_dev);
            if (ret < 0)
                LOG_ERR("failed to close led: %d", ret);
        }

        if (state == 2) state = 0;

        k_sleep(K_MSEC(500));
    }
}

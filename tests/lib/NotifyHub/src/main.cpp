// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/NotifyHub/Notify.hpp>
#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <OF/lib/HubManager/HubRegistry.hpp>
#include <OF/lib/algo/Mecanum.hpp>

#include "OF/drivers/output/buzzer.h"


LOG_MODULE_REGISTER(notify_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;

constexpr NotifyHubConfig notify_hub_config{
    .led_pixel_dev = DEVICE_DT_GET(DT_NODELABEL(pixel_led)),
    .pwm_buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer))
};

using namespace ems::literals;
constexpr auto melody =
    R"((200)1,2,3,4,5,
)"_ems;

int main()
{
    LOG_INF("main");
    HubRegistry::startAll();
    constexpr led_color c1 = COLOR_HEX("#d81159");
    constexpr led_color c2 = COLOR_HEX("#118ab2");
    NotifyHub::setBuzzerStatus("buzzer1", {std::span(melody), 10, true});

    NotifyHub::setLEDStatus("test1", {c1, LEDMode::Blink, 3, 300});
    k_sleep(K_MSEC(2000));
    NotifyHub::setLEDStatus("test1", {c1, LEDMode::Blink, 3, 100});
    k_sleep(K_MSEC(2000));
    NotifyHub::setLEDStatus("test2", {c2, LEDMode::Breathing, 1, 300});
    k_sleep(K_MSEC(2000));
    NotifyHub::removeLEDStatus("test1");

    while (true)
    {
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);

        k_sleep(K_MSEC(500));
    }
}

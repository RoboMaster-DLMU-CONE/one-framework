// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/HubManager/HubRegistry.hpp>
#include <OF/lib/ControllerHub/ControllerHub.hpp>

#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>

LOG_MODULE_REGISTER(cc_test, CONFIG_LOG_DEFAULT_LEVEL);


using namespace OF;
using enum ControllerHub::Channel;

constexpr ControllerHubConfig controller_hub_config{
    .input_device = DEVICE_DT_GET(DT_NODELABEL(usart3)),
};

int main()
{
    HubRegistry::startAll();

    while (true)
    {
        k_sleep(K_MSEC(200));
        auto state = ControllerHub::getData();
        if (!state)
        {
            LOG_INF("The controller has disconnected.");
            continue;
        }
        auto value = state.value();
        const auto leftX = value.percent(LEFT_X);
        const auto leftY = value.percent(LEFT_Y);
        const auto rightX = value[RIGHT_X];
        const auto rightY = value[RIGHT_Y];
        const auto swL = value[SW_L];
        const auto swR = value[SW_R];
        const auto wheel = value[WHEEL];
        LOG_INF("%f, %f, %d, %d, %d, %d, %d", leftX, leftY, rightX, rightY, swL, swR, wheel);
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
    }
}
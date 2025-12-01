// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/ControllerHub/ControllerHub.hpp>

#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(cc_test, CONFIG_LOG_DEFAULT_LEVEL);

using enum OF::ControllerHub::Channel;

int main()
{
    auto& inst = OF::ControllerHub::getInstance();
    while (true)
    {
        auto state = inst.getState();


        const auto leftX = state[LEFT_X];
        const auto leftY = state[LEFT_Y];
        const auto rightX = state[RIGHT_X];
        const auto rightY = state[RIGHT_Y];
        const auto swL = state[SW_L];
        const auto swR = state[SW_R];
        const auto wheel = state[WHEEL];
        LOG_INF("%d, %d, %d, %d, %d, %d, %d", leftX, leftY, rightX, rightY, swL, swR, wheel);
        k_sleep(K_MSEC(500));
    }
}

// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/ControllerCenter/ControllerCenter.hpp>

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(cc_test, CONFIG_LOG_DEFAULT_LEVEL);

using enum OF::ControllerCenter::Channel;
int main()
{
    auto& inst = OF::ControllerCenter::getInstance();
    while (true)
    {
        const auto leftX = inst[LEFT_X];
        const auto leftY = inst[LEFT_Y];
        const auto rightX = inst[RIGHT_X];
        const auto rightY = inst[RIGHT_Y];
        const auto swL = inst[SW_L];
        const auto swR = inst[SW_R];
        LOG_INF("%ld, %ld, %ld, %ld, %ld, %ld", leftX, leftY, rightX, rightY, swL, swR);
        k_sleep(K_MSEC(500));
    }
}

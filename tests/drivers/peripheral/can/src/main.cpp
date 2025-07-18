// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <OneMotor/Can/CanDriver.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

using OneMotor::Can::CanDriver;
using OneMotor::Can::CanFrame;

int main()
{
    return 0;
}

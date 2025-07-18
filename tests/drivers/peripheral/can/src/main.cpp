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
    const device* can_dev1 = DEVICE_DT_GET(DT_CHOSEN(can1));
    CanDriver can1(can_dev1);

    (void)can1.open().map_error([](const OneMotor::Error& e)
    {
        LOG_ERR("%s", e.message.c_str());
    });

    return 0;
}

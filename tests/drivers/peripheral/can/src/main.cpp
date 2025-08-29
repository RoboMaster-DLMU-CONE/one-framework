// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <OneMotor/Can/CanDriver.hpp>
#include <OF/lib/Network/VCANInterface.hpp>
#include <OF/lib/Network/Daemon.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

using OneMotor::Can::CanDriver;
using OneMotor::Can::CanFrame;
using namespace OF::Network;

int main()
{
    // Initialize network daemon for VCAN interface management
    Daemon daemon;
    if (!daemon.initialize()) {
        LOG_ERR("Failed to initialize network daemon");
        return -1;
    }

    // Create VCAN interface for testing
    VCANInterface vcan_if("vcan_test", daemon);
    
    if (!vcan_if.is_up()) {
        LOG_INF("Bringing up VCAN interface");
        vcan_if.bring_up();
    }

    LOG_INF("VCAN interface %s is %s", 
            vcan_if.getName().c_str(), 
            vcan_if.is_up() ? "up" : "down");

    // Original CAN driver test (if device tree node exists)
    const device* can_dev1 = DEVICE_DT_GET(DT_CHOSEN(can1));
    if (device_is_ready(can_dev1)) {
        CanDriver can1(can_dev1);

        (void)can1.open().map_error([](const OneMotor::Error& e)
        {
            LOG_ERR("%s", e.message.c_str());
        });
    } else {
        LOG_INF("CAN device not available, using VCAN interface for testing");
    }

    daemon.shutdown();
    return 0;
}

// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Network/CANInterface.hpp>
#include <OF/lib/Network/Daemon.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_interface, CONFIG_LOG_DEFAULT_LEVEL);

namespace OF::Network
{
    CANInterface::CANInterface(std::string_view name, Daemon& daemon)
        : Interface(name), daemon_(daemon)
    {
        LOG_DBG("Creating CAN interface: %.*s", 
                static_cast<int>(name.length()), name.data());

        // Check if interface exists
        if (!daemon_.interfaceExists(name)) {
            LOG_ERR("CAN interface not found: %.*s", 
                    static_cast<int>(name.length()), name.data());
            throw CANInterfaceNotFoundException(name);
        }

        // Verify it's actually a CAN interface
        std::string interfaceType = daemon_.parseInterfaceType(name);
        if (interfaceType != "can") {
            LOG_ERR("Interface %.*s is not a CAN interface (type: %s)", 
                    static_cast<int>(name.length()), name.data(), interfaceType.c_str());
            throw CANInterfaceNotFoundException(name);
        }

        LOG_INF("CAN interface created successfully: %.*s", 
                static_cast<int>(name.length()), name.data());
    }

    bool CANInterface::is_up() const
    {
        LOG_DBG("Checking if CAN interface is up: %s", name_.c_str());
        return daemon_.getNetlink().is_up(name_);
    }

    bool CANInterface::bring_up()
    {
        LOG_INF("Bringing up CAN interface: %s", name_.c_str());
        return daemon_.bringInterfaceUp(name_);
    }

    bool CANInterface::bring_down()
    {
        LOG_INF("Bringing down CAN interface: %s", name_.c_str());
        return daemon_.bringInterfaceDown(name_);
    }

} // namespace OF::Network
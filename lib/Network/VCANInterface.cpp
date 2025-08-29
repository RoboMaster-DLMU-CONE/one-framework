// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Network/VCANInterface.hpp>
#include <OF/lib/Network/Daemon.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vcan_interface, CONFIG_LOG_DEFAULT_LEVEL);

namespace OF::Network
{
    VCANInterface::VCANInterface(std::string_view name, Daemon& daemon)
        : Interface(name), daemon_(daemon), createdByThisInstance_(false)
    {
        LOG_DBG("Creating VCAN interface: %.*s", 
                static_cast<int>(name.length()), name.data());

        // Check if interface already exists
        if (daemon_.interfaceExists(name)) {
            LOG_INF("VCAN interface already exists: %.*s", 
                    static_cast<int>(name.length()), name.data());
            
            // Verify it's actually a VCAN interface
            std::string interfaceType = daemon_.parseInterfaceType(name);
            if (interfaceType == "vcan") {
                LOG_INF("Using existing VCAN interface: %.*s", 
                        static_cast<int>(name.length()), name.data());
                createdByThisInstance_ = false;
            } else {
                LOG_ERR("Interface %.*s exists but is not a VCAN interface (type: %s)", 
                        static_cast<int>(name.length()), name.data(), interfaceType.c_str());
                // We could throw an exception here, but for VCAN we'll be more permissive
                // and try to create anyway with a warning
                LOG_WRN("Attempting to create VCAN interface despite existing non-VCAN interface");
            }
        }

        // Create the VCAN interface if it doesn't exist or if we need to override
        if (!daemon_.interfaceExists(name) || createdByThisInstance_ == false) {
            if (daemon_.createVCANInterface(name)) {
                createdByThisInstance_ = true;
                LOG_INF("VCAN interface created successfully: %.*s", 
                        static_cast<int>(name.length()), name.data());
            } else {
                LOG_ERR("Failed to create VCAN interface: %.*s", 
                        static_cast<int>(name.length()), name.data());
                // For VCAN, we don't throw exception as it's for testing/development
                createdByThisInstance_ = false;
            }
        }
    }

    VCANInterface::~VCANInterface()
    {
        if (createdByThisInstance_) {
            LOG_INF("Cleaning up VCAN interface: %s", name_.c_str());
            if (!daemon_.deleteInterface(name_)) {
                LOG_WRN("Failed to delete VCAN interface during cleanup: %s", name_.c_str());
            }
        }
    }

    bool VCANInterface::is_up() const
    {
        LOG_DBG("Checking if VCAN interface is up: %s", name_.c_str());
        return daemon_.getNetlink().is_up(name_);
    }

    bool VCANInterface::bring_up()
    {
        LOG_INF("Bringing up VCAN interface: %s", name_.c_str());
        return daemon_.bringInterfaceUp(name_);
    }

    bool VCANInterface::bring_down()
    {
        LOG_INF("Bringing down VCAN interface: %s", name_.c_str());
        return daemon_.bringInterfaceDown(name_);
    }

} // namespace OF::Network
// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Network/Netlink.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(netlink, CONFIG_LOG_DEFAULT_LEVEL);

// For embedded systems, we'll simulate netlink functionality
// since full netlink support may not be available in Zephyr
#include <cstring>

namespace OF::Network
{
    Netlink::Netlink() : socketFd_(-1), initialized_(false)
    {
    }

    Netlink::~Netlink()
    {
        close();
    }

    bool Netlink::initialize()
    {
        if (initialized_) {
            return true;
        }

        // In a real implementation, this would create a netlink socket
        // For simulation purposes in embedded environment, we'll mock this
        LOG_DBG("Initializing Netlink socket (simulated)");
        
        socketFd_ = 1; // Mock socket fd
        initialized_ = true;
        
        LOG_INF("Netlink socket initialized successfully");
        return true;
    }

    void Netlink::close()
    {
        if (initialized_ && socketFd_ >= 0) {
            // In real implementation: ::close(socketFd_);
            LOG_DBG("Closing Netlink socket (simulated)");
            socketFd_ = -1;
            initialized_ = false;
        }
    }

    bool Netlink::is_up(std::string_view interfaceName) const
    {
        if (!initialized_) {
            LOG_WRN("Netlink not initialized");
            return false;
        }

        return queryInterfaceStatus(interfaceName);
    }

    bool Netlink::queryInterfaceStatus(std::string_view interfaceName) const
    {
        // Simulate interface status query
        // In real implementation, this would send RTM_GETLINK netlink message
        LOG_DBG("Querying interface status for: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Mock logic: assume vcan interfaces are always up, others need to be checked
        if (interfaceName.starts_with("vcan")) {
            LOG_DBG("VCAN interface %.*s is up", 
                    static_cast<int>(interfaceName.length()), interfaceName.data());
            return true;
        }

        // For CAN interfaces, simulate based on name pattern
        if (interfaceName.starts_with("can")) {
            // Simulate some CAN interfaces being down
            if (interfaceName == "can0") {
                LOG_DBG("CAN interface %.*s is up", 
                        static_cast<int>(interfaceName.length()), interfaceName.data());
                return true;
            } else {
                LOG_DBG("CAN interface %.*s is down", 
                        static_cast<int>(interfaceName.length()), interfaceName.data());
                return false;
            }
        }

        // Default to down for unknown interfaces
        LOG_DBG("Unknown interface %.*s assumed down", 
                static_cast<int>(interfaceName.length()), interfaceName.data());
        return false;
    }

} // namespace OF::Network
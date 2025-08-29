// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/Network/Daemon.hpp>
#include <OF/lib/Network/Netlink.hpp>
#include <zephyr/logging/log.h>
#include <algorithm>

LOG_MODULE_REGISTER(daemon, CONFIG_LOG_DEFAULT_LEVEL);

namespace OF::Network
{
    Daemon::Daemon() : initialized_(false)
    {
        netlink_ = std::make_unique<Netlink>();
    }

    Daemon::~Daemon()
    {
        shutdown();
    }

    bool Daemon::initialize()
    {
        if (initialized_) {
            return true;
        }

        LOG_INF("Initializing Network Daemon");

        if (!netlink_->initialize()) {
            LOG_ERR("Failed to initialize Netlink");
            return false;
        }

        initialized_ = true;
        LOG_INF("Network Daemon initialized successfully");
        return true;
    }

    void Daemon::shutdown()
    {
        if (initialized_) {
            LOG_INF("Shutting down Network Daemon");
            netlink_->close();
            interfaceTypes_.clear();
            initialized_ = false;
        }
    }

    bool Daemon::createVCANInterface(std::string_view interfaceName)
    {
        if (!initialized_) {
            LOG_ERR("Daemon not initialized");
            return false;
        }

        if (!validateInterfaceName(interfaceName)) {
            LOG_ERR("Invalid interface name: %.*s", 
                    static_cast<int>(interfaceName.length()), interfaceName.data());
            return false;
        }

        LOG_INF("Creating VCAN interface: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Simulate VCAN interface creation
        // In real implementation: ip link add dev <name> type vcan
        std::string command = "ip link add dev ";
        command += interfaceName;
        command += " type vcan";

        if (!executeCommand(command)) {
            LOG_ERR("Failed to create VCAN interface: %.*s", 
                    static_cast<int>(interfaceName.length()), interfaceName.data());
            return false;
        }

        // Store interface type
        interfaceTypes_[std::string(interfaceName)] = "vcan";

        // Bring interface up
        if (!bringInterfaceUp(interfaceName)) {
            LOG_WRN("Created VCAN interface but failed to bring it up: %.*s", 
                    static_cast<int>(interfaceName.length()), interfaceName.data());
        }

        LOG_INF("VCAN interface created successfully: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());
        return true;
    }

    bool Daemon::deleteInterface(std::string_view interfaceName)
    {
        if (!initialized_) {
            LOG_ERR("Daemon not initialized");
            return false;
        }

        LOG_INF("Deleting interface: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Simulate interface deletion
        // In real implementation: ip link del dev <name>
        std::string command = "ip link del dev ";
        command += interfaceName;

        if (!executeCommand(command)) {
            LOG_ERR("Failed to delete interface: %.*s", 
                    static_cast<int>(interfaceName.length()), interfaceName.data());
            return false;
        }

        // Remove from interface types map
        interfaceTypes_.erase(std::string(interfaceName));

        LOG_INF("Interface deleted successfully: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());
        return true;
    }

    bool Daemon::interfaceExists(std::string_view interfaceName) const
    {
        if (!initialized_) {
            return false;
        }

        // Simulate interface existence check
        // In real implementation: check /sys/class/net/<name> or use netlink
        LOG_DBG("Checking if interface exists: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Mock logic: assume vcan interfaces exist if created, standard CAN interfaces exist
        if (interfaceTypes_.find(std::string(interfaceName)) != interfaceTypes_.end()) {
            return true;
        }

        // Check for standard CAN interfaces
        if (interfaceName == "can0" || interfaceName == "can1") {
            return true;
        }

        return false;
    }

    bool Daemon::bringInterfaceUp(std::string_view interfaceName)
    {
        if (!initialized_) {
            LOG_ERR("Daemon not initialized");
            return false;
        }

        LOG_DBG("Bringing interface up: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Simulate bringing interface up
        // In real implementation: ip link set dev <name> up
        std::string command = "ip link set dev ";
        command += interfaceName;
        command += " up";

        return executeCommand(command);
    }

    bool Daemon::bringInterfaceDown(std::string_view interfaceName)
    {
        if (!initialized_) {
            LOG_ERR("Daemon not initialized");
            return false;
        }

        LOG_DBG("Bringing interface down: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data());

        // Simulate bringing interface down
        // In real implementation: ip link set dev <name> down
        std::string command = "ip link set dev ";
        command += interfaceName;
        command += " down";

        return executeCommand(command);
    }

    std::string Daemon::parseInterfaceType(std::string_view interfaceName) const
    {
        // Check if we have stored type information
        auto it = interfaceTypes_.find(std::string(interfaceName));
        if (it != interfaceTypes_.end()) {
            return it->second;
        }

        // Parse type from name pattern
        if (interfaceName.starts_with("vcan")) {
            return "vcan";
        } else if (interfaceName.starts_with("can")) {
            return "can";
        }

        return "unknown";
    }

    bool Daemon::parseInterfaceConfig(std::string_view interfaceName, std::string_view configParams)
    {
        LOG_DBG("Parsing interface config for %.*s: %.*s", 
                static_cast<int>(interfaceName.length()), interfaceName.data(),
                static_cast<int>(configParams.length()), configParams.data());

        // Simple config parsing simulation
        // In real implementation, this would parse parameters like bitrate, etc.
        
        // For now, just store that we've seen this configuration
        std::string key = std::string(interfaceName) + "_config";
        interfaceTypes_[key] = std::string(configParams);

        return true;
    }

    bool Daemon::executeCommand(std::string_view command) const
    {
        // Simulate command execution
        // In real implementation: use system() or proper process execution
        LOG_DBG("Executing command (simulated): %.*s", 
                static_cast<int>(command.length()), command.data());

        // Mock success for most commands
        return true;
    }

    bool Daemon::validateInterfaceName(std::string_view interfaceName) const
    {
        if (interfaceName.empty() || interfaceName.length() > 15) {
            return false;
        }

        // Check for valid characters (alphanumeric and underscore)
        return std::all_of(interfaceName.begin(), interfaceName.end(), 
                          [](char c) { return std::isalnum(c) || c == '_'; });
    }

} // namespace OF::Network
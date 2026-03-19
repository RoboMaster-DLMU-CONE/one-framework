// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/VtHub/VtHub.hpp>

#include <RPL/Packets/VT03RemotePacket.hpp>
#include <RPL/Packets/RoboMaster/CustomControllerData.hpp>
#include <RPL/Packets/RoboMaster/RemoteControl.hpp>
#include <RPL/Packets/RoboMaster/CustomRobotData.hpp>
#include <RPL/Packets/RoboMaster/RobotCustomData.hpp>
#include <RPL/Packets/RoboMaster/VtmSetChannel.hpp>
#include <RPL/Packets/RoboMaster/VtmQueryChannel.hpp>

#include <RPL/Parser.hpp>
#include <RPL/Deserializer.hpp>

#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <zephyr/device.h>

#include <OF/utils/Remap.hpp>

LOG_MODULE_REGISTER(VtHub, CONFIG_VT_HUB_LOG_LEVEL);

namespace OF {

    float vt_stick_percent(uint64_t stick)
    {
        return remap<364.0f,1684.0f,-1.0f, 1.0f>(stick);
    }

struct VtHubDataInternal {
    const device* uart_dev;
    RPL::Deserializer<
        VT03RemotePacket,
        CustomControllerData,
        RemoteControl,
        CustomRobotData,
        RobotCustomData,
        VtmSetChannel,
        VtmQueryChannel
    > deserializer;
    RPL::Parser<
        VT03RemotePacket,
        CustomControllerData,
        RemoteControl,
        CustomRobotData,
        RobotCustomData,
        VtmSetChannel,
        VtmQueryChannel
    > parser;
    uint32_t last_rx_time;
    
    VtHubDataInternal() : parser(deserializer) {
        last_rx_time = 0;
        uart_dev = nullptr;
    }
};

static VtHubDataInternal s_data;

bool VtHub::is_connected() {
    // Basic timeout check (e.g. 500ms)
    // If last_rx_time is 0, never connected
    if (s_data.last_rx_time == 0) return false;
    return k_uptime_get_32() - s_data.last_rx_time < 500;
}

// ISR Callback
static void uart_rx_callback(const device* dev, void* user_data) {
    ARG_UNUSED(user_data);

    if (!uart_irq_update(dev)) {
        return;
    }

    if (uart_irq_rx_ready(dev)) {
        // Zero-copy read directly into parser buffer
        auto span = s_data.parser.get_write_buffer();
        if (!span.empty()) {
            if (const int len = uart_fifo_read(dev, span.data(), span.size()); len > 0) {
                // Update timestamp
                s_data.last_rx_time = k_uptime_get_32();
                
                // Advance write index - this triggers parsing and deserialization
                (void)s_data.parser.advance_write_index(len);
            }
        } else {
            // Drain FIFO to avoid stuck IRQ
             uint8_t discard;
             while (uart_fifo_read(dev, &discard, 1) > 0);
        }
    }
}

#define DT_DRV_COMPAT one_framework_vt_hub

static int vthub_sys_init(void)
{
    
    // Find the node with compatible "one-framework,vt-hub"
    #if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
        LOG_WRN("No VtHub instance defined in Device Tree");
        return 0;
    #endif

    const struct device *uart_dev = DEVICE_DT_GET(DT_INST_PROP(0, input_device));
    
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }
    
    s_data.uart_dev = uart_dev;

    // Configure UART
    uart_config cfg{};
    uart_config_get(uart_dev, &cfg);
    cfg.baudrate = 921600;
    cfg.data_bits = UART_CFG_DATA_BITS_8;
    cfg.stop_bits = UART_CFG_STOP_BITS_1;
    cfg.parity = UART_CFG_PARITY_NONE;
    cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

    if (uart_configure(uart_dev, &cfg) < 0) {
        LOG_ERR("Failed to configure UART");
        return -EIO;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_rx_callback, nullptr);
    uart_irq_rx_enable(uart_dev);

    LOG_INF("VtHub initialized");
    return 0;
}

SYS_INIT(vthub_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

// Implementation of get<T>
template <typename T>
tl::expected<T, VtHubError> VtHub::get() {
    if (!is_connected()) {
        return tl::unexpected(VtHubError{VtHubError::Code::DISCONNECTED, "VtHub Disconnected"});
    }
    // Deserializer.get<T>() returns a copy of the packet
    T packet = s_data.deserializer.template get<T>();
    
    return packet;
}

// Explicit instantiations
template tl::expected<VT03RemotePacket, VtHubError> VtHub::get();
template tl::expected<CustomControllerData, VtHubError> VtHub::get();
template tl::expected<RemoteControl, VtHubError> VtHub::get();
template tl::expected<CustomRobotData, VtHubError> VtHub::get();
template tl::expected<RobotCustomData, VtHubError> VtHub::get();
template tl::expected<VtmSetChannel, VtHubError> VtHub::get();
template tl::expected<VtmQueryChannel, VtHubError> VtHub::get();

} // namespace OF
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

namespace OF
{
    float vt_stick_percent(uint64_t stick)
    {
        return remap<364.0f, 1684.0f, -1.0f, 1.0f>(stick);
    }

    struct VtHubDataInternal
    {
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

        // Double buffering for async UART (only used if async API available)
        static constexpr size_t RX_BUF_SIZE = 256;
        uint8_t rx_buf[2][RX_BUF_SIZE];
        uint8_t current_buf;
        bool using_async;

        VtHubDataInternal() : parser(deserializer)
        {
            last_rx_time = 0;
            uart_dev = nullptr;
            current_buf = 0;
            using_async = false;
        }
    };

    static VtHubDataInternal s_data;

    bool VtHub::is_connected()
    {
        if (s_data.last_rx_time == 0) return false;
        return k_uptime_get_32() - s_data.last_rx_time < 500;
    }

    // ========== Async API Implementation ==========
#if defined(CONFIG_UART_ASYNC_API)
    static void uart_async_callback(const device* dev, uart_event* evt, void* user_data)
    {
        ARG_UNUSED(user_data);

        switch (evt->type)
        {
            case UART_RX_RDY:
            {
                const size_t len = evt->data.rx.len;
                const uint8_t* data = &evt->data.rx.buf[evt->data.rx.offset];

                s_data.last_rx_time = k_uptime_get_32();

                auto span = s_data.parser.get_write_buffer();
                if (!span.empty())
                {
                    size_t copy_len = len < span.size() ? len : span.size();
                    memcpy(span.data(), data, copy_len);
                    (void)s_data.parser.advance_write_index(copy_len);
                }
                break;
            }

            case UART_RX_BUF_REQUEST:
                s_data.current_buf = 1 - s_data.current_buf;
                uart_rx_buf_rsp(dev, s_data.rx_buf[s_data.current_buf], VtHubDataInternal::RX_BUF_SIZE);
                break;

            case UART_RX_DISABLED:
                LOG_WRN("UART RX disabled, re-enabling");
                uart_rx_enable(dev, s_data.rx_buf[s_data.current_buf], VtHubDataInternal::RX_BUF_SIZE, 10);
                break;

            default:
                break;
        }
    }

    static bool try_init_async(const device* uart_dev)
    {
        if (uart_callback_set(uart_dev, uart_async_callback, nullptr) < 0)
        {
            return false;
        }

        if (uart_rx_enable(uart_dev, s_data.rx_buf[0], VtHubDataInternal::RX_BUF_SIZE, 10) < 0)
        {
            return false;
        }

        s_data.using_async = true;
        LOG_INF("VtHub using async API with DMA");
        return true;
    }
#else
    static bool try_init_async(const device* uart_dev)
    {
        ARG_UNUSED(uart_dev);
        return false;
    }
#endif

    // ========== Interrupt API Implementation (Fallback) ==========
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
    static void uart_irq_callback(const device* dev, void* user_data)
    {
        ARG_UNUSED(user_data);

        if (!uart_irq_update(dev))
        {
            return;
        }

        if (uart_irq_rx_ready(dev))
        {
            // Use a local buffer to batch-read from FIFO
            uint8_t buffer[64];
            int len = uart_fifo_read(dev, buffer, sizeof(buffer));
            if (len > 0)
            {
                s_data.last_rx_time = k_uptime_get_32();
                (void)s_data.parser.push_data(buffer, len);
            }
        }
    }

    static bool try_init_irq(const device* uart_dev)
    {
        uart_irq_callback_user_data_set(uart_dev, uart_irq_callback, nullptr);
        uart_irq_rx_enable(uart_dev);
        LOG_INF("VtHub using interrupt API (fallback)");
        return true;
    }
#else
    static bool try_init_irq(const device* uart_dev)
    {
        ARG_UNUSED(uart_dev);
        return false;
    }
#endif

#define DT_DRV_COMPAT one_framework_vt_hub

    static int vthub_sys_init(void)
    {
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
        LOG_WRN("No VtHub instance defined in Device Tree");
        return 0;
#endif

        const struct device* uart_dev = DEVICE_DT_GET(DT_INST_PROP(0, input_device));

        if (!device_is_ready(uart_dev))
        {
            LOG_ERR("UART device not ready");
            return -ENODEV;
        }

        s_data.uart_dev = uart_dev;

        // Configure UART for high-speed VT link
        uart_config cfg{};
        uart_config_get(uart_dev, &cfg);
        cfg.baudrate = 921600;
        cfg.data_bits = UART_CFG_DATA_BITS_8;
        cfg.stop_bits = UART_CFG_STOP_BITS_1;
        cfg.parity = UART_CFG_PARITY_NONE;
        cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

        if (uart_configure(uart_dev, &cfg) < 0)
        {
            LOG_ERR("Failed to configure UART");
            return -EIO;
        }

        // Try async API first, fallback to interrupt API
        if (!try_init_async(uart_dev))
        {
            if (!try_init_irq(uart_dev))
            {
                LOG_ERR("Failed to initialize VtHub: no UART API available");
                return -ENOTSUP;
            }
        }

        LOG_INF("VtHub initialized");
        return 0;
    }

    SYS_INIT(vthub_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

    // Implementation of get<T>
    template <typename T>
    tl::expected<T, VtHubError> VtHub::get()
    {
        if (!is_connected())
        {
            return tl::unexpected(VtHubError{VtHubError::Code::DISCONNECTED, "VtHub Disconnected"});
        }
        return s_data.deserializer.template get<T>();
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

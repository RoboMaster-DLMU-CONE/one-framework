// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/lib/ControllerHub/ControllerHub.hpp>
#include "zephyr/logging/log.h"
#include <OF/utils/CCM.h>

LOG_MODULE_REGISTER(ControllerHub, CONFIG_CONTROLLER_HUB_LOG_LEVEL);

namespace OF
{
    // 定义全局变量
    const device* g_input_dev = nullptr;
    OF_CCM_ATTR ControllerHub hub;

    // 全局控制器数据缓冲区
    OF_CCM_ATTR SeqlockBuf<ControllerHubData> g_controller_buf;

    namespace
    {
        struct ControllerHubRegistrar
        {
            ControllerHubRegistrar()
            {
                registerHub<ControllerHub>(&hub);
            }
        }
            __used controller_hub_registrar;
    }

    // 实现静态成员变量
    ControllerHub::ControllerHubDataInternal ControllerHub::s_data{};
    const device* ControllerHub::s_uart_dev = nullptr;

    // DBUS UART配置 - 100kbps波特率 奇偶校验
    constexpr uart_config uart_cfg_dbus = {
        .baudrate = 100000,
        .parity = UART_CFG_PARITY_EVEN,
        .stop_bits = UART_CFG_STOP_BITS_2,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
    };

    // 定义通道映射表 - 存储DBUS通道号
    const uint32_t ControllerHub::input_channels_full[DBUS_CHANNEL_COUNT] = {
        0, /* right_stick_x */
        1, /* right_stick_y */
        2, /* left_stick_x */
        3, /* left_stick_y */
        4, /* switch_right */
        5, /* switch_left */
        6, /* mouse_x */
        7, /* mouse_y */
        8, /* mouse_z */
        9, /* mouse_left */
        10, /* mouse_right */
        11, /* wheel */
        /* 键盘 16 键 */
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
    };

    tl::expected<ControllerHub::State, ControllerHubError> ControllerHub::getData()
    {
        if (s_data.in_sync)
        {
            return g_controller_buf.read();
        }
        return tl::make_unexpected(ControllerHubError{
            ControllerHubError::Code::DISCONNECTED, "Controller is disconnected"
        });
    }

    void ControllerHub::configure(const ControllerHubConfig& config)
    {
        if (config.input_device)
        {
            g_input_dev = config.input_device;
        }
    }

    void ControllerHub::setup()
    {
        // 获取UART设备
        s_uart_dev = g_input_dev;

        if (!s_uart_dev || !device_is_ready(s_uart_dev))
        {
            LOG_ERR("Invalid or not ready UART device");
            return;
        }

        // 确保之前的RX/TX已停止
        uart_rx_disable(s_uart_dev);
        uart_tx_abort(s_uart_dev);

        LOG_DBG("Initializing DBUS driver in ControllerHub");

        // 初始化内部数据结构
        for (int i = 0; i < static_cast<int>(DBUS_CHANNEL_COUNT); i++)
        {
            s_data.last_reported_value[i] = 0;
            s_data.channel_mapping[i] = -1;
        }

        s_data.xfer_bytes = 0;
        s_data.in_sync = false;
        s_data.last_rx_time = 0;
        s_data.using_async = false; /* will be set to true if async setup succeeds */

        // 设置通道映射
        for (int8_t i = 0; i < static_cast<int>(DBUS_CHANNEL_COUNT); i++)
        {
            s_data.channel_mapping[input_channels_full[i]] = i;
        }

        // 配置UART
        int ret = uart_configure(s_uart_dev, &uart_cfg_dbus);
        if (ret < 0)
        {
            LOG_ERR("Failed to configure UART port: %d", ret);
            return;
        }

        // 尝试异步UART API
        ret = uart_callback_set(s_uart_dev, [](const device*, uart_event* evt, void* user_data)
        {
            dbus_uart_event_handler(evt);
        }, nullptr);

        if (ret == 0)
        {
            s_data.using_async = true;
            k_sem_init(&s_data.report_lock, 0, 1);

            ret = dbus_enable_rx();
            if (ret < 0)
            {
                LOG_ERR("Can't use UART Async rx: %d", ret);
                return;
            }

            LOG_DBG("Using UART async API");
        }
        else
        {
            // 异步API不可用 -> 尝试IRQ API
            if (ret == -ENOTSUP || ret == -ENOSYS)
            {
                LOG_DBG("UART async API unavailable (%d), falling back to IRQ API", ret);

                ret = uart_irq_callback_user_data_set(s_uart_dev, [](const device* uart_dev, void* user_data)
                {
                    dbus_uart_isr_handler();
                }, nullptr);

                if (ret < 0)
                {
                    if (ret == -ENOTSUP)
                    {
                        LOG_ERR("Interrupt-driven UART API support not enabled");
                    }
                    else if (ret == -ENOSYS)
                    {
                        LOG_ERR("UART device does not support interrupt-driven API");
                    }
                    else
                    {
                        LOG_ERR("Error setting UART callback: %d", ret);
                    }
                    return;
                }

                s_data.using_async = false;
                k_sem_init(&s_data.report_lock, 0, 1);

                uart_irq_rx_disable(s_uart_dev);
                uart_irq_rx_enable(s_uart_dev);

                LOG_DBG("Using UART IRQ API (fallback)");
            }
        }

        // 启动数据处理线程
        k_thread_create(&s_data.thread, s_data.thread_stack,
                        K_THREAD_STACK_SIZEOF(s_data.thread_stack),
                        reinterpret_cast<k_thread_entry_t>(input_dbus_input_report_thread),
                        nullptr, nullptr, nullptr,
                        K_PRIO_COOP(7), 0, K_NO_WAIT);

        k_thread_name_set(&s_data.thread, "dbus_proc");
    }

    // 实现内部方法
    int ControllerHub::dbus_enable_rx()
    {
        if (s_data.using_async)
        {
            s_data.next_async_buf = 1;
            return uart_rx_enable(s_uart_dev, s_data.async_rx_buf[0], DBUS_FRAME_LEN, SYS_FOREVER_US);
        }
        uart_irq_rx_enable(s_uart_dev);
        return 0;
    }

    void ControllerHub::dbus_restart_rx()
    {
        int ret = 0;

        if (s_data.using_async)
        {
            ret = uart_rx_disable(s_uart_dev);

            if (ret < 0 && ret != -ENOTSUP)
            {
                LOG_ERR("Failed to disable UART RX: %d", ret);
            }

            ret = dbus_enable_rx();
            if (ret < 0)
            {
                LOG_ERR("Failed to enable UART async receive: %d", ret);
            }
        }
        else
        {
            /* IRQ mode */
            uart_irq_rx_disable(s_uart_dev);
            uart_irq_rx_enable(s_uart_dev);
        }
    }

    void ControllerHub::dbus_append_rx_bytes(const uint8_t* buf, size_t len)
    {
        const uint32_t now = k_uptime_get_32();
        if (s_data.in_sync && s_data.last_rx_time != 0 &&
            now - s_data.last_rx_time > DBUS_INTERFRAME_SPACING_MS)
        {
            s_data.in_sync = false;
            s_data.xfer_bytes = 0;
        }

        size_t offset = 0;

        while (offset < len)
        {
            size_t chunk = len - offset;
            size_t remaining = DBUS_FRAME_LEN - s_data.xfer_bytes;

            if (chunk > remaining)
            {
                chunk = remaining;
            }

            if (s_data.xfer_bytes == 0)
            {
                s_data.last_rx_time = now;
            }

            memcpy(&s_data.rd_data[s_data.xfer_bytes], buf + offset, chunk);
            s_data.xfer_bytes += chunk;
            offset += chunk;

            if (s_data.xfer_bytes != DBUS_FRAME_LEN)
            {
                continue;
            }

            s_data.xfer_bytes = 0;
            if (!s_data.in_sync)
            {
                s_data.in_sync = true;
            }

            memcpy(s_data.dbus_frame, s_data.rd_data, DBUS_FRAME_LEN);
            s_data.last_rx_time = now;
            k_sem_give(&s_data.report_lock);
        }
    }

    void ControllerHub::dbus_supply_rx_buffer()
    {
        const int ret = uart_rx_buf_rsp(s_uart_dev, s_data.async_rx_buf[s_data.next_async_buf], DBUS_FRAME_LEN);
        if (ret == 0)
        {
            s_data.next_async_buf ^= 1;
        }
        else
        {
            LOG_ERR("Failed to provide UART RX buffer: %d", ret);
        }
    }

    bool ControllerHub::dbus_frame_valid(const uint8_t* dbus_buf)
    {
        constexpr int16_t STICK_MIN = 364;
        constexpr int16_t STICK_MAX = 1684;

        const auto ch0 =
            static_cast<int16_t>((static_cast<uint16_t>(dbus_buf[0]) | (static_cast<uint16_t>(dbus_buf[1]) << 8)) &
                0x07FF);
        const auto ch1 =
            static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[1]) >> 3) | (static_cast<uint16_t>(dbus_buf[2]) <<
                5)) & 0x07FF));
        const auto ch2 =
            static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[2]) >> 6) | (static_cast<uint16_t>(dbus_buf[3]) << 2)
                |
                (static_cast<uint16_t>(dbus_buf[4]) << 10)) & 0x07FF));
        const auto ch3 =
                static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[4]) >> 1) |
                                      (static_cast<uint16_t>(dbus_buf[5]) << 7)) &
                    0x07FF));


        const uint8_t sw_l = (dbus_buf[5] >> 4) & 0x03;
        const uint8_t sw_r = (dbus_buf[5] >> 6) & 0x03;

        const auto stick_in_range = [](int16_t v)
        {
            return v >= STICK_MIN && v <= STICK_MAX;
        };

        if (!stick_in_range(ch0) || !stick_in_range(ch1) || !stick_in_range(ch2) || !stick_in_range(ch3))
        {
            return false;
        }

        if (sw_l == 0 || sw_l > 3 || sw_r == 0 || sw_r > 3)
        {
            return false;
        }

        return true;
    }

    void ControllerHub::dbus_uart_event_handler(uart_event* evt)
    {
        switch (evt->type)
        {
        case UART_RX_RDY:
            dbus_append_rx_bytes(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
            break;
        case UART_RX_BUF_REQUEST:
            dbus_supply_rx_buffer();
            break;
        case UART_RX_STOPPED:
            LOG_WRN("UART RX stopped (%d)", evt->data.rx_stop.reason);
            s_data.in_sync = false;
            s_data.xfer_bytes = 0;
            dbus_restart_rx();
            break;
        default:
            break;
        }
    }

    /* IRQ fallback handler (interrupt-driven UART API) */
    void ControllerHub::dbus_uart_isr_handler()
    {
        uint8_t* rd_data = s_data.rd_data;

        if (s_uart_dev == nullptr)
        {
            LOG_DBG("UART device is NULL");
            return;
        }

        if (!uart_irq_update(s_uart_dev))
        {
            LOG_DBG("uart_irq_update() failed, cannot handle interrupt");
            return;
        }

        while (uart_irq_rx_ready(s_uart_dev) && s_data.xfer_bytes < DBUS_FRAME_LEN)
        {
            if (s_data.in_sync)
            {
                if (s_data.xfer_bytes == 0)
                {
                    s_data.last_rx_time = k_uptime_get_32();
                }
                s_data.xfer_bytes += uart_fifo_read(s_uart_dev, &rd_data[s_data.xfer_bytes],
                                                    DBUS_FRAME_LEN - s_data.xfer_bytes);
            }
            else
            {
                /* DBUS没有特定的帧头/帧尾，使用帧长度来同步 */
                s_data.xfer_bytes += uart_fifo_read(s_uart_dev, &rd_data[s_data.xfer_bytes],
                                                    DBUS_FRAME_LEN - s_data.xfer_bytes);

                if (s_data.xfer_bytes == DBUS_FRAME_LEN)
                {
                    s_data.in_sync = true;
                    k_sem_give(&s_data.report_lock);
                }
            }
        }

        if (s_data.in_sync && (k_uptime_get_32() - s_data.last_rx_time > DBUS_INTERFRAME_SPACING_MS))
        {
            s_data.in_sync = false;
            s_data.xfer_bytes = 0;
            uart_irq_rx_disable(s_uart_dev);
            uart_irq_rx_enable(s_uart_dev);
            k_sem_give(&s_data.report_lock);
        }
        else if (s_data.in_sync && s_data.xfer_bytes == DBUS_FRAME_LEN)
        {
            s_data.xfer_bytes = 0;
            memcpy(s_data.dbus_frame, rd_data, DBUS_FRAME_LEN);
            k_sem_give(&s_data.report_lock);
        }
    }

    void ControllerHub::input_dbus_input_report_thread()
    {
        uint16_t last_keyboard = 0;
        bool prev_connected = false;

        while (true)
        {
            int ret = k_sem_take(&s_data.report_lock, K_MSEC(DBUS_INTERFRAME_SPACING_MS));
            if (ret == -EBUSY || ret == -EAGAIN)
            {
                // 没有接收到数据，检查是否超时
                if (s_data.in_sync)
                {
                    // 之前是同步状态，现在没有数据，可能已断开
                    s_data.in_sync = false;
                    s_data.xfer_bytes = 0;
                    dbus_restart_rx();
                    LOG_DBG("DBUS receiver connection lost due to timeout");
                }
                continue;
            }
            if (ret < 0)
            {
                /* 与UART接收器失去同步 */
                const unsigned int key = irq_lock();

                s_data.in_sync = false;
                s_data.xfer_bytes = 0;
                irq_unlock(key);

                dbus_restart_rx();

                LOG_DBG("DBUS receiver connection lost");

                /* 报告连接丢失 */
                continue;
            }
            // 如果成功获取信号量，说明收到了数据，检查同步状态
            else if (!s_data.in_sync)
            {
                // 之前没有同步，现在收到数据，说明已连接
                LOG_DBG("DBUS receiver connected");
                s_data.in_sync = true;
            }

            // 检查连接状态是否有变化
            if (!prev_connected && s_data.in_sync)
            {
                LOG_DBG("DBUS controller connected");
            }
            else if (prev_connected && !s_data.in_sync)
            {
                // 连接断开的情况
                LOG_DBG("DBUS controller disconnected");
            }

            prev_connected = s_data.in_sync;

            // 解析DBUS数据并更新全局控制器数据
            const uint8_t* dbus_buf = s_data.dbus_frame;

            if (!dbus_frame_valid(dbus_buf))
            {
                s_data.in_sync = false;
                s_data.xfer_bytes = 0;
                dbus_restart_rx();
                LOG_DBG("Invalid DBUS frame, resync");
                continue;
            }

            ControllerHubData state;
            // 报告通道1-4（遥控器摇杆）
            state[Channel::RIGHT_X] =
                static_cast<int16_t>((static_cast<uint16_t>(dbus_buf[0]) | (static_cast<uint16_t>(dbus_buf[1]) << 8)) &
                    0x07FF);
            state[Channel::RIGHT_Y] =
                static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[1]) >> 3) | (static_cast<uint16_t>(dbus_buf[2])
                    << 5)) & 0x07FF));
            state[Channel::LEFT_X] =
                static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[2]) >> 6) | (static_cast<uint16_t>(dbus_buf[3])
                    << 2) | (
                    static_cast<uint16_t>(dbus_buf[4]) << 10)) & 0x07FF));
            state[Channel::LEFT_Y] =
                static_cast<int16_t>((((static_cast<uint16_t>(dbus_buf[4]) >> 1) | (static_cast<uint16_t>(dbus_buf[5])
                    << 7)) & 0x07FF));

            // 报告开关位置 - 通道5-6
            state[Channel::SW_L] =
                static_cast<int16_t>((dbus_buf[5] >> 4) & 0x0003);
            state[Channel::SW_R] =
                static_cast<int16_t>((dbus_buf[5] >> 6) & 0x0003);

            // 鼠标数据 - 通道7-10
            state[Channel::MOUSE_X] =
                static_cast<int16_t>(dbus_buf[6] | dbus_buf[7] << 8); /* X轴 */
            state[Channel::MOUSE_Y] =
                static_cast<int16_t>(dbus_buf[8] | dbus_buf[9] << 8); /* Y轴 */
            state[Channel::MOUSE_Z] =
                static_cast<int16_t>(dbus_buf[10] | dbus_buf[11] << 8); /* Z轴 */
            state[Channel::MOUSE_LEFT] =
                static_cast<int16_t>((dbus_buf[12] & 0x01) ? 1 : 0); /* 左键 */
            state[Channel::MOUSE_RIGHT] =
                static_cast<int16_t>((dbus_buf[12] & 0x02) ? 1 : 0); /* 右键 */

            // 滚轮数据 - 通道11
            state[Channel::WHEEL] =
                static_cast<int16_t>((static_cast<uint16_t>(dbus_buf[16] | static_cast<uint16_t>(dbus_buf[17] << 8))
                    & 0x07FF));

            // 键盘数据 - 通道12-27
            const uint16_t keyboard = dbus_buf[14] | (dbus_buf[15] << 8);
            if (keyboard != last_keyboard)
            {
                state[Channel::KEY_W] =
                    static_cast<int16_t>((keyboard & (1u << 0)) ? 1 : 0);
                state[Channel::KEY_S] =
                    static_cast<int16_t>((keyboard & (1u << 1)) ? 1 : 0);
                state[Channel::KEY_D] =
                    static_cast<int16_t>((keyboard & (1u << 2)) ? 1 : 0);
                state[Channel::KEY_A] =
                    static_cast<int16_t>((keyboard & (1u << 3)) ? 1 : 0);
                state[Channel::KEY_SHIFT] =
                    static_cast<int16_t>((keyboard & (1u << 4)) ? 1 : 0);
                state[Channel::KEY_CTRL] =
                    static_cast<int16_t>((keyboard & (1u << 5)) ? 1 : 0);
                state[Channel::KEY_Q] =
                    static_cast<int16_t>((keyboard & (1u << 6)) ? 1 : 0);
                state[Channel::KEY_E] =
                    static_cast<int16_t>((keyboard & (1u << 7)) ? 1 : 0);
                state[Channel::KEY_R] =
                    static_cast<int16_t>((keyboard & (1u << 8)) ? 1 : 0);
                state[Channel::KEY_F] =
                    static_cast<int16_t>((keyboard & (1u << 9)) ? 1 : 0);
                state[Channel::KEY_G] =
                    static_cast<int16_t>((keyboard & (1u << 10)) ? 1 : 0);
                state[Channel::KEY_Z] =
                    static_cast<int16_t>((keyboard & (1u << 11)) ? 1 : 0);
                state[Channel::KEY_X] =
                    static_cast<int16_t>((keyboard & (1u << 12)) ? 1 : 0);
                state[Channel::KEY_C] =
                    static_cast<int16_t>((keyboard & (1u << 13)) ? 1 : 0);
                state[Channel::KEY_V] =
                    static_cast<int16_t>((keyboard & (1u << 14)) ? 1 : 0);
                state[Channel::KEY_B] =
                    static_cast<int16_t>((keyboard & (1u << 15)) ? 1 : 0);

                last_keyboard = keyboard;
            }

            g_controller_buf.write(state);
        }
    }
}

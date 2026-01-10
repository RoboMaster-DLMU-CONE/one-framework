// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONTROLLERHUB_HPP
#define CONTROLLERHUB_HPP

#include <array>

#include <OF/lib/HubManager/HubBase.hpp>

#include <OF/utils/Remap.hpp>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <tl/expected.hpp>

namespace OF
{
    struct ControllerHubConfig
    {
        const device* input_device;
    };

    // ControllerHub 错误类型定义
    struct ControllerHubError
    {
        enum class Code
        {
            DISCONNECTED, // 遥控器断线
        } code;

        const char* message;
    };

    // Forward declare State class for ControllerHub
    class ControllerHubData
    {
    public:
        enum class Channel: uint16_t
        {
            LEFT_X,
            LEFT_Y,
            RIGHT_X,
            RIGHT_Y,
            WHEEL,
            SW_L,
            SW_R,
            MOUSE_X,
            MOUSE_Y,
            MOUSE_Z,
            MOUSE_LEFT,
            MOUSE_RIGHT,
            KEY_W,
            KEY_S,
            KEY_D,
            KEY_A,
            KEY_SHIFT,
            KEY_CTRL,
            KEY_Q,
            KEY_E,
            KEY_R,
            KEY_F,
            KEY_G,
            KEY_Z,
            KEY_X,
            KEY_C,
            KEY_V,
            KEY_B,
        };

        int16_t& operator[](Channel ch)
        {
            return m_arr[static_cast<size_t>(ch)];
        }

        int16_t operator[](Channel ch) const
        {
            return m_arr[static_cast<size_t>(ch)];
        }

        [[nodiscard]] float percent(const Channel ch) const
        {
            const float value = m_arr[static_cast<size_t>(ch)];
            return remap<364.0f, 1684.0f, -1.0f, 1.0f>(value);
        }

    private:
        std::array<int16_t, 32> m_arr{}; // 扩展数组大小以容纳新通道
    };

    class ControllerHub : public HubBase<ControllerHub>
    {
    public:
        ControllerHub() = default;
        static constexpr auto name = "ControllerHub";

        using Channel = ControllerHubData::Channel;
        using State = ControllerHubData;

        static tl::expected<State, ControllerHubError> getData();

        void setup();
        void configure(const ControllerHubConfig& config);

        ControllerHub(ControllerHub&) = delete;
        ControllerHub& operator=(const ControllerHub&) = delete;

    private:
        // DBUS相关数据结构
        static constexpr size_t DBUS_FRAME_LEN = 18;
        static constexpr size_t DBUS_CHANNEL_COUNT = 28; // 通道总数：5个摇杆+2个开关+4个鼠标+1个滚轮+16个键盘
        static constexpr uint32_t DBUS_INTERFRAME_SPACING_MS = 20;

        // 通道映射表 - 直接存储DBUS通道号
        static const uint32_t input_channels_full[DBUS_CHANNEL_COUNT];

        // 内部数据结构
        struct ControllerHubDataInternal
        {
            k_thread thread;
            k_sem report_lock;

            uint16_t xfer_bytes;
            uint8_t rd_data[DBUS_FRAME_LEN];
            uint8_t dbus_frame[DBUS_FRAME_LEN];
            bool in_sync;
            uint32_t last_rx_time;

            /* async mode buffers */
            uint8_t async_rx_buf[2][DBUS_FRAME_LEN];
            uint8_t next_async_buf;

            uint16_t last_reported_value[DBUS_CHANNEL_COUNT];
            int8_t channel_mapping[DBUS_CHANNEL_COUNT];

            bool using_async; /* true if async API is in use, false -> IRQ fallback */

            K_KERNEL_STACK_MEMBER(thread_stack, 1024);
        };

        static ControllerHubDataInternal s_data;
        static const device* s_uart_dev;

        // 内部方法
        static int dbus_enable_rx();
        static void dbus_restart_rx();
        static void dbus_append_rx_bytes(const uint8_t* buf, size_t len);
        static void dbus_supply_rx_buffer();
        static void dbus_uart_event_handler(struct uart_event* evt);
        static void dbus_uart_isr_handler();
        static void input_dbus_input_report_thread();
    };
} // namespace OF
static tl::expected<OF::ControllerHubData, OF::ControllerHubError> getControllerData()
{
    return OF::ControllerHub::getData();
}
#endif // CONTROLLERHUB_HPP

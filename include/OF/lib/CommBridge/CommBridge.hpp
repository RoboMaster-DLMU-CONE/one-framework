// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef OF_LIB_COMMBRIDGE_MANAGER_HPP
#define OF_LIB_COMMBRIDGE_MANAGER_HPP

#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <memory>

// 空包类型，用于表示不需要发送或接收任何包的情况
struct EmptyPacket
{
};


template <>
struct RPL::Meta::PacketTraits<EmptyPacket> : PacketTraitsBase<PacketTraits<EmptyPacket>>
{
    static constexpr uint16_t cmd = 0xF0FF;
    static constexpr size_t size = sizeof(EmptyPacket);
};

namespace OF
{
    template <typename TxPackets, typename RxPackets>
    class CommBridge;

    template <typename... TxPackets, typename... RxPackets>
    class CommBridge<std::tuple<TxPackets...>, std::tuple<RxPackets...>>
    {
    public:
        /**
         * @brief 工厂函数，创建 CommBridge 对象
         */
        static std::unique_ptr<CommBridge> create(const device* uart_dev)
        {
            return std::unique_ptr<CommBridge>(new CommBridge(uart_dev));
        }

        ~CommBridge()
        {
            stop_receive();
        }

        CommBridge(const CommBridge&) = delete;
        CommBridge& operator=(const CommBridge&) = delete;
        CommBridge(CommBridge&&) = delete;
        CommBridge& operator=(CommBridge&&) = delete;

        /**
         * @brief 启动中断驱动接收
         */
        void start_receive()
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            if (m_RxEnabled)
            {
                LOG_WRN("Receive already enabled");
                return;
            }

            uart_irq_callback_user_data_set(m_UartDev, uart_rx_callback, this);
            uart_irq_rx_enable(m_UartDev);
            m_RxEnabled = true;

            LOG_INF("UART RX interrupt enabled");
        }

        /**
         * @brief 停止接收
         */
        void stop_receive()
        {
            if (!m_RxEnabled)
            {
                return;
            }

            uart_irq_rx_disable(m_UartDev);
            m_RxEnabled = false;
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            LOG_INF("UART RX interrupt disabled");
        }

        /**
         * @brief 发送数据包
         */
        template <typename... Packets>
            requires(sizeof...(Packets) == 0 || (RPL::Serializable<Packets, TxPackets...> && ...))
        void send(const Packets&... packets)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            // 如果没有发送包，则直接返回
            if constexpr (sizeof...(Packets) == 0)
            {
                return;
            }

            k_sem_take(&m_TxDoneSem, K_FOREVER);

            auto res = m_Serializer.serialize(m_TxBuffer, TxBufferSize, packets...);
            if (!res)
            {
                LOG_ERR("Serialize failed");
                k_sem_give(&m_TxDoneSem);
                return;
            }

            m_TxSize = res.value();

            for (size_t i = 0; i < m_TxSize; ++i)
            {
                uart_poll_out(m_UartDev, m_TxBuffer[i]);
            }

            k_sem_give(&m_TxDoneSem);
        }

        /**
         * @brief 获取接收到的数据包
         */
        template <typename T>
            requires(sizeof...(RxPackets) == 0 ? false : RPL::Deserializable<T, RxPackets...>)
        T get() noexcept
        {
            return m_Deserializer.template get<T>();
        }

    private:
        explicit CommBridge(const device* uart_dev) :
            m_UartDev(uart_dev),
            m_Deserializer(),
            m_Parser(m_Deserializer)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            LOG_INF("Creating CommBridge");
            if (!device_is_ready(m_UartDev))
            {
                LOG_ERR("UART not ready");
                k_oops();
            }

            k_sem_init(&m_TxDoneSem, 1, 1);

            LOG_INF("CommBridge initialized, UART device: %s", m_UartDev->name);
        }

        // 如果 TxPackets 为空，则 TxBufferSize 为 0，否则计算实际大小
        static constexpr size_t calculateTxBufferSize()
        {
            if constexpr (sizeof...(TxPackets) == 0)
            {
                return 0; // 没有发送包时，不需要发送缓冲区
            }
            else
            {
                return (RPL::Serializer<TxPackets...>::template frame_size<TxPackets>() + ...);
            }
        }

        static constexpr size_t TxBufferSize = calculateTxBufferSize();
        static constexpr size_t RxBufferSize = CONFIG_COMM_BRIDGE_MAX_RX_SIZE;

        const device* m_UartDev;

        RPL::Serializer<TxPackets...> m_Serializer{};
        uint8_t m_TxBuffer[TxBufferSize > 0 ? TxBufferSize : 1]{}; // 至少1个字节以避免零长度数组
        k_sem m_TxDoneSem{};
        size_t m_TxSize{0};

        RPL::Deserializer<RxPackets...> m_Deserializer{};
        RPL::Parser<RxPackets...> m_Parser;
        uint8_t m_RxBuffer[RxBufferSize]{};

        bool m_RxEnabled{false};

        /**
         * @brief UART 中断回调函数
         */
        static void uart_rx_callback(const device* dev, void* user_data)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            auto* bridge = static_cast<CommBridge*>(user_data);

            if (!uart_irq_update(dev))
            {
                return;
            }

            if (uart_irq_rx_ready(dev))
            {
                uint8_t buffer[64];
                int len = uart_fifo_read(dev, buffer, sizeof(buffer));
                if (len > 0)
                {
                    bridge->m_Parser.push_data(buffer, len);
                }
            }
        }
    };

    // 便捷别名，用于创建只接收不发送的 CommBridge
    template <typename... RxPackets>
    using RxOnlyCommBridge = CommBridge<std::tuple<EmptyPacket>, std::tuple<RxPackets...>>;

    // 便捷别名，用于创建只发送不接收的 CommBridge
    template <typename... TxPackets>
    using TxOnlyCommBridge = CommBridge<std::tuple<TxPackets...>, std::tuple<EmptyPacket>>;
}

#endif //OF_LIB_COMMBRIDGE_MANAGER_HPP

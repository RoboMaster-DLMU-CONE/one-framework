#ifndef OF_LIB_COMMBRIDGE_MANAGER_HPP
#define OF_LIB_COMMBRIDGE_MANAGER_HPP

#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

namespace OF::CommBridge
{
    namespace detail
    {
        template <size_t StackSize>
        struct ThreadStackHolder
        {
            alignas(Z_KERNEL_STACK_OBJ_ALIGN)
            static inline uint8_t stack[K_KERNEL_STACK_LEN(StackSize)]{};
        };
    }

    template <typename TxPackets, typename RxPackets>
    class Manager;

    template <typename... TxPackets, typename... RxPackets>
    class Manager<std::tuple<TxPackets...>, std::tuple<RxPackets...>>
    {
    public:
        explicit Manager(const device* uart_dev) :
            m_UartDev(uart_dev),
            m_Deserializer(),
            m_Parser(m_Deserializer)
        {
            LOG_MODULE_DECLARE(CommBridgeManager, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            LOG_INF("Creating Manager");
            if (!device_is_ready(m_UartDev))
            {
                LOG_ERR("UART not ready");
                k_oops();
            }
            LOG_INF("Init sem");

            k_sem_init(&m_TxDoneSem, 1, 1);
            k_sem_init(&m_ThreadStartSem, 0, 1);

            LOG_INF("Manager initialized, UART device: %s", m_UartDev->name);
        }

        ~Manager()
        {
            stop_receive();
        }

        /**
         * @brief 启动专用接收线程
         */
        void start_receive()
        {
            LOG_MODULE_DECLARE(CommBridgeManager, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            if (m_ThreadRunning)
            {
                LOG_WRN("Receive thread already running");
                return;
            }

            m_ThreadRunning = true;

            // 创建并启动接收线程
            m_RxThreadId = k_thread_create(
                &m_RxThreadData,
                reinterpret_cast<k_thread_stack_t*>(&detail::ThreadStackHolder<RX_THREAD_STACK_SIZE>::stack),
                RX_THREAD_STACK_SIZE,
                rx_thread_entry,
                this, nullptr, nullptr,
                RX_THREAD_PRIORITY,
                K_ESSENTIAL,
                K_NO_WAIT
                );

            k_thread_name_set(m_RxThreadId, "uart_rx");

            // 等待线程启动完成
            k_sem_take(&m_ThreadStartSem, K_FOREVER);

            LOG_INF("UART RX thread started (priority=%d, stack=0x%p)",
                    RX_THREAD_PRIORITY,
                    &detail::ThreadStackHolder<RX_THREAD_STACK_SIZE>::stack);

        }

        /**
         * @brief 停止接收线程
         */
        void stop_receive()
        {
            if (!m_ThreadRunning)
            {
                return;
            }

            m_ThreadRunning = false;
            k_thread_join(m_RxThreadId, K_FOREVER);
            LOG_MODULE_DECLARE(CommBridgeManager, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            LOG_INF("UART RX thread stopped");
        }

        /**
         * @brief 发送数据包(使用中断模式,避免阻塞接收线程)
         */
        template <typename... Packets>
            requires(RPL::Serializable<Packets, TxPackets...> && ...)
        void send(const Packets&... packets)
        {
            LOG_MODULE_DECLARE(CommBridgeManager, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            // 等待上一次发送完成
            k_sem_take(&m_TxDoneSem, K_FOREVER);

            // 序列化
            auto res = m_Serializer.serialize(m_TxBuffer, TxBufferSize, packets...);
            if (!res)
            {
                LOG_ERR("Serialize failed");
                k_sem_give(&m_TxDoneSem);
                return;
            }

            m_TxSize = res.value();

            // 使用轮询发送(避免中断冲突)
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
            requires RPL::Deserializable<T, RxPackets...>
        T get() const noexcept
        {
            return m_Deserializer.template get<T>();
        }

        /**
         * @brief 获取 Parser 引用
         */
        RPL::Parser<RxPackets...>& get_parser() noexcept
        {
            return m_Parser;
        }

    private:
        static constexpr size_t TxBufferSize = (RPL::Serializer<TxPackets...>::template frame_size<TxPackets>() + ...);
        static constexpr size_t RxBufferSize = CONFIG_COMM_BRIDGE_MAX_RX_SIZE;

        // 接收线程优先级(必须低于 USB 驱动)
        static constexpr int RX_THREAD_PRIORITY = 7; // USB 通常是 0-5
        static constexpr size_t RX_THREAD_STACK_SIZE = 2048;

        const device* m_UartDev;

        // 发送相关
        RPL::Serializer<TxPackets...> m_Serializer{};
        uint8_t m_TxBuffer[TxBufferSize]{};
        k_sem m_TxDoneSem{};
        size_t m_TxSize{0};

        // 接收相关
        RPL::Deserializer<RxPackets...> m_Deserializer{};
        RPL::Parser<RxPackets...> m_Parser;
        uint8_t m_RxBuffer[RxBufferSize]{};

        // 线程控制
        bool m_ThreadRunning{false};
        k_thread m_RxThreadData{};
        k_tid_t m_RxThreadId{nullptr};
        k_sem m_ThreadStartSem{};

        /**
         * @brief 接收线程入口函数
         */
        static void rx_thread_entry(void* p1, void* p2, void* p3)
        {
            auto* manager = static_cast<Manager*>(p1);
            LOG_MODULE_DECLARE(CommBridgeManager, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            LOG_INF("RX thread started");

            // 通知主线程启动完成
            k_sem_give(&manager->m_ThreadStartSem);

            // 清空 UART FIFO
            uint8_t dummy;
            while (uart_poll_in(manager->m_UartDev, &dummy) == 0)
            {
                // 丢弃残留数据
            }

            // 主循环:轮询接收
            while (manager->m_ThreadRunning)
            {
                uint8_t ch;
                int ret = uart_poll_in(manager->m_UartDev, &ch);

                if (ret == 0)
                {
                    // 成功读取一个字节
                    manager->m_Parser.push_data(&ch, 1);
                }
                else
                {
                    // 没有数据,让出 CPU
                    k_yield();
                }
            }

            LOG_INF("RX thread exiting");
        }
    };
}

#endif //OF_LIB_COMMBRIDGE_MANAGER_HPP

#ifndef OF_COMM_BRIDGE_SENDER_HPP
#define OF_COMM_BRIDGE_SENDER_HPP

#include <RPL/Serializer.hpp>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

namespace OF::CommBridge
{

    template <typename... Ts>
    class Sender
    {
    public:
        explicit Sender(const device* uart_dev, bool wait_for_completion = true) :
            m_UartDev(uart_dev),
            m_WaitForCompletion(wait_for_completion)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            if (!device_is_ready(m_UartDev))
            {
                LOG_ERR("指定的UART设备未就绪");
                k_oops();
            }

            // 检测 UART 是否支持异步 API（通常意味着启用了 DMA）
            int ret = uart_callback_set(m_UartDev, uart_async_callback, this);
            if (ret == 0)
            {
                m_UseAsyncApi = true;
                k_sem_init(&m_TxDoneSem, 1, 1); // 初始值为1，表示可以发送
                printk("UART Async API (DMA) 已启用，阻塞模式=%d\n", wait_for_completion);
            }
            else if (ret == -ENOSYS)
            {
                m_UseAsyncApi = false;
                printk("UART Async API 不可用，将使用中断模式\n");
                // 设置中断模式
                uart_irq_callback_user_data_set(m_UartDev, uart_irq_callback, this);
                k_sem_init(&m_TxDoneSem, 1, 1);
            }
            else
            {
                LOG_ERR("指定的UART设备未就绪");
                k_oops();
            }
        };

        template <typename... Packets>
            requires(RPL::Serializable<Packets, Ts...> && ...)
        void send(const Packets&... packets)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            // 等待上一次发送完成（防止buffer被覆盖）
            k_sem_take(&m_TxDoneSem, K_FOREVER);

            // 选择当前buffer（双缓冲机制，仅在异步非阻塞模式下使用）
            uint8_t* current_buffer = m_UseAsyncApi && !m_WaitForCompletion
                ? m_Buffers[m_CurrentBufferIdx]
                : m_Buffers[0];

            size_t total_size = m_Serializer.serialize(current_buffer, BufferSize, m_Seq, packets...);
            m_Seq++;

            if (m_UseAsyncApi)
            {
                // 使用异步 API (DMA) - 最快
                send_async(current_buffer, total_size);

                // 如果启用双缓冲，切换buffer索引
                if (!m_WaitForCompletion)
                {
                    m_CurrentBufferIdx = 1 - m_CurrentBufferIdx;
                }
            }
            else
            {
                // 使用中断驱动模式
                send_interrupt(current_buffer, total_size);
            }

            // 如果需要阻塞等待传输完成
            if (m_WaitForCompletion)
            {
                // 等待传输完成信号
                k_sem_take(&m_TxDoneSem, K_FOREVER);
                // 立即释放，允许下次发送
                k_sem_give(&m_TxDoneSem);
            }
        }

    private:
        static constexpr size_t BufferSize = (RPL::Serializer<Ts...>::template frame_size<Ts>() + ...);

        RPL::Serializer<Ts...> m_Serializer{};
        const device* m_UartDev;
        bool m_UseAsyncApi{false};
        bool m_WaitForCompletion; // 是否阻塞等待传输完成

        // 双缓冲机制：仅在异步非阻塞模式下使用第二个buffer
        uint8_t m_Buffers[2][BufferSize]{};
        uint8_t m_CurrentBufferIdx{0};
        uint8_t m_Seq{};

        // 信号量用于同步（初始值为1）
        k_sem m_TxDoneSem{};

        // 中断模式使用的变量
        size_t m_TxBytesRemaining{0};
        const uint8_t* m_TxCurrentPtr{nullptr};

        // 异步模式发送（使用 DMA）
        void send_async(const uint8_t* buffer, const size_t len)
        {
            int ret = uart_tx(m_UartDev, buffer, len, SYS_FOREVER_US);
            if (ret != 0)
            {
                LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

                LOG_WRN("UART async tx 启动失败: %d\n", ret);
                // 发送失败，释放信号量
                k_sem_give(&m_TxDoneSem);
            }
            // 成功启动后，等待回调中的 k_sem_give
        }

        // 中断模式发送
        void send_interrupt(const uint8_t* buffer, const size_t len)
        {
            m_TxBytesRemaining = len;
            m_TxCurrentPtr = buffer;

            // 启动中断传输
            uart_irq_tx_enable(m_UartDev);
        }

        // 异步模式回调
        static void uart_async_callback(const device* dev,
                                        uart_event* evt,
                                        void* user_data)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            auto* sender = static_cast<Sender*>(user_data);

            switch (evt->type)
            {
            case UART_TX_DONE:
                // 传输完成，释放信号量
                k_sem_give(&sender->m_TxDoneSem);
                break;

            case UART_TX_ABORTED:
                // 传输中止，也释放信号量
                LOG_WRN("UART TX 中止");
                k_sem_give(&sender->m_TxDoneSem);
                break;

            default:
                break;
            }
        }

        // 中断模式回调
        static void uart_irq_callback(const device* dev, void* user_data)
        {
            auto* sender = static_cast<Sender*>(user_data);

            uart_irq_update(dev);

            if (uart_irq_tx_ready(dev) && sender->m_TxBytesRemaining > 0)
            {
                // 填充 FIFO
                int sent = uart_fifo_fill(dev, sender->m_TxCurrentPtr,
                                          sender->m_TxBytesRemaining);
                sender->m_TxCurrentPtr += sent;
                sender->m_TxBytesRemaining -= sent;

                if (sender->m_TxBytesRemaining == 0)
                {
                    // 传输完成，禁用 TX 中断
                    uart_irq_tx_disable(dev);
                    // 释放信号量
                    k_sem_give(&sender->m_TxDoneSem);
                }
            }
        }
    };
}

#endif //OF_COMM_BRIDGE_SENDER_HPP

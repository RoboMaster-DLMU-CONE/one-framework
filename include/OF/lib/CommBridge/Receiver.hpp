#ifndef OF_COMM_BRIDGE_RECIVER_HPP
#define OF_COMM_BRIDGE_RECIVER_HPP

#include <zephyr/device.h>
#include <RPL/Parser.hpp>
#include <zephyr/kernel.h>
#include "zephyr/drivers/uart.h"


namespace OF::CommBridge
{

    template <typename... Ts>
    class Receiver
    {
    public:
        explicit Receiver(const device* uart_dev) :
            m_UartDev(uart_dev), m_Deserializer(), m_Parser(m_Deserializer)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            if (!device_is_ready(m_UartDev))
            {
                LOG_ERR("指定的UART设备未就绪");
                k_oops();
            }
            if (int ret = uart_callback_set(m_UartDev, uart_async_callback, this); ret == 0)
            {
                m_UseAsyncApi = true;
                LOG_INF("使用 UART 异步 API");
                start_receive();
            }
            else
            {
                m_UseAsyncApi = false;
                LOG_INF("使用 UART 中断 API");
                uart_irq_callback_user_data_set(m_UartDev, uart_irq_callback, this);
            }
        }

        void start()
        {
            if (m_UseAsyncApi)
            {
                start_receive();
            }
            else
            {
                uart_irq_rx_enable(m_UartDev);
            }
        }

        void stop() const
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);
            if (m_UseAsyncApi)
            {
                int ret = uart_rx_disable(m_UartDev);
                if (ret < 0)
                {
                    LOG_ERR("Failed to disable UART RX: %d", ret);
                }
            }
            else
            {
                uart_irq_rx_disable(m_UartDev);
            }
        }

        template <typename T>
            requires RPL::Deserializable<T, Ts...>
        T get() const noexcept
        {
            return m_Deserializer.get();
        }

    private:
        void start_receive()
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);


            int ret = uart_rx_enable(m_UartDev, m_Buffer, CONFIG_COMM_BRIDGE_MAX_RX_SIZE,
                                     CONFIG_COMM_BRIDGE_RX_TIMEOUT_US);
            if (ret < 0)
            {
                LOG_ERR("Failed to enable UART RX: %d", ret);
            }
        }

        static void uart_irq_callback(const device* dev, void* user_data)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            auto* receiver = static_cast<Receiver*>(user_data);

            if (!uart_irq_update(dev))
            {
                return;
            }

            if (uart_irq_rx_ready(dev))
            {
                // 一次性读取FIFO中的所有数据
                while (true)
                {
                    int read_len = uart_fifo_read(dev, receiver->m_Buffer, CONFIG_COMM_BRIDGE_MAX_RX_SIZE);
                    if (read_len <= 0)
                    {
                        // FIFO已空
                        break;
                    }
                    LOG_DBG("Read %d bytes from FIFO", read_len);
                    receiver->m_Parser.push_data(receiver->m_Buffer, read_len);
                }
            }
        }

        static void uart_async_callback(const device* dev,
                                        uart_event* evt,
                                        void* user_data)
        {
            LOG_MODULE_DECLARE(CommBridge, CONFIG_COMM_BRIDGE_LOG_LEVEL);

            auto* receiver = static_cast<Receiver*>(user_data);
            switch (evt->type)
            {
            case UART_RX_RDY:
            {
                receiver->m_Parser.push_data(receiver->m_Buffer, evt->data.rx.len);
                break;
            }
            case UART_RX_BUF_REQUEST:
            {
                LOG_DBG("RX_BUF_REQUEST");
                receiver->m_Parser.push_data(receiver->m_Buffer, evt->data.rx.len);


                const int ret = uart_rx_buf_rsp(dev, receiver->m_Buffer, CONFIG_COMM_BRIDGE_MAX_RX_SIZE);
                if (ret < 0)
                {
                    LOG_WRN("Failed to provide next RX buffer: %d", ret);
                }
                break;
            }
            case UART_RX_BUF_RELEASED:
            {
                LOG_DBG("RX_BUF_RELEASED");
                break;
            }
            case UART_RX_DISABLED:
            {
                LOG_DBG("RX_DISABLED");
            }
            case UART_RX_STOPPED:
            {
                LOG_WRN("RX_STOPPED: reason=%d", evt->data.rx_stop.reason);
                if (evt->data.rx_stop.data.len > 0)
                {
                    receiver->m_Parser.push_data(receiver->m_Buffer, evt->data.rx.len);
                }
                receiver->start_receive();
                break;
            }
            default:
                LOG_DBG("Unhandled UART event: %d", evt->type);
                break;
            }
        }

        bool m_UseAsyncApi{false};
        const device* m_UartDev;
        RPL::Deserializer<Ts...> m_Deserializer;
        RPL::Parser<Ts...> m_Parser;
        uint8_t m_Buffer[CONFIG_COMM_BRIDGE_MAX_RX_SIZE]{};
    };
}

#endif //OF_COMM_BRIDGE_RECIVER_HPP

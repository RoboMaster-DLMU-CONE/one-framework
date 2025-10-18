#ifndef OF_COMM_BRIDGE_RECIVER_HPP
#define OF_COMM_BRIDGE_RECIVER_HPP

#include <zephyr/device.h>

#include <RPL/Parser.hpp>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

#include "zephyr/drivers/uart.h"

LOG_MODULE_REGISTER(CommBridgeReceiver, CONFIG_COMM_BRIDGE_LOG_LEVEL);

namespace OF::CommBridge
{
    template <typename... Ts>
    class Receiver
    {
    public:
        explicit Receiver(const device* uart_dev) :
            m_UartDev(uart_dev), m_Deserializer(), m_Parser(m_Deserializer)
        {
            if (!device_is_ready(m_UartDev))
            {
                LOG_ERR("指定的UART设备未就绪");
                k_oops();
            }
            if (int ret = uart_callback_set(m_UartDev, uart_async_callback, this); ret == 0)
            {
                m_UseAsyncApi = true;

            }

        }

    private:
        static void uart_async_callback(const device* dev,
                                        uart_event* evt,
                                        void* user_data)
        {
            auto* receiver = static_cast<Receiver*>(user_data);
            switch (evt->type)
            {
            case UART_RX_RDY:
                break;
            default:
                break;
            }
        }

        bool m_UseAsyncApi{false};
        const device* m_UartDev;
        RPL::Deserializer<Ts...> m_Deserializer;
        RPL::Parser<Ts...> m_Parser;
    };
}

#endif //OF_COMM_BRIDGE_RECIVER_HPP

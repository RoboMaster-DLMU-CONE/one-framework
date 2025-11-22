#include <OF/drivers/output/status_leds.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#include "zephyr/drivers/uart.h"
#include <RPL/Packets/Sample/SampleA.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static const device* uart_dev;

// 定义环形缓冲区用于存储接收到的数据
#define RING_BUF_SIZE 1024
uint8_t ring_buffer_data[RING_BUF_SIZE];
struct ring_buf ringbuf;

// UART 中断回调函数
static void uart_isr_callback(const struct device* dev, void* user_data)
{
    ARG_UNUSED(user_data);


    // 必须调用 uart_irq_update 来更新中断状态
    if (!uart_irq_update(dev))
    {
        return;
    }

    // 处理接收中断
    if (uart_irq_rx_ready(dev))
    {
        // 定义缓冲区，一次性读取尽可能多的数据
        uint8_t recv_buf[64]; // 可以根据您的数据包大小调整
        int recv_len;

        // 一次性读取 FIFO 中的所有数据
        recv_len = uart_fifo_read(dev, recv_buf, sizeof(recv_buf));

        if (recv_len > 0)
        {
            // 将接收到的数据批量放入环形缓冲区
            ring_buf_put(&ringbuf, recv_buf, recv_len);
        }
    }

    // 处理发送中断
    if (uart_irq_tx_ready(dev))
    {
        uart_irq_tx_disable(dev);
    }
}

int main()
{


    const struct device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    // 初始化环形缓冲区
    ring_buf_init(&ringbuf, sizeof(ring_buffer_data), ring_buffer_data);

    /* Poll if the DTR flag was set */
    // 临时注释掉 DTR 等待，方便调试
    // TODO: 调试完成后可以恢复这段代码
    /*
    while (!dtr)
    {
        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
    */

    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }

    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("Console 设备未就绪\n");
        return -1;
    }

    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);
    led_api->set_heartbeat(status_led_dev);

    LOG_INF("程序开始运行……");
    printk("USB CDC Echo 测试程序 (中断驱动模式 + 环形缓冲区)\n");
    printk("请输入任意字符，将会回显您输入的内容\n");
    printk("======================================\n");

    // 配置 UART 中断
    uart_irq_callback_user_data_set(uart_dev, uart_isr_callback, NULL);

    // 启用接收中断
    uart_irq_rx_enable(uart_dev);

    // 主循环 - 从环形缓冲区读取数据并回显
    while (true)
    {
        uint8_t c;

        // 从环形缓冲区读取数据
        if (ring_buf_get(&ringbuf, &c, 1) == 1)
        {
            // 回显字符
            printk("%c", c);

            // 如果是回车，额外发送换行
            if (c == '\r')
            {
                printk("\n");
            }
        }
        else
        {
            // 没有数据时短暂休眠
            k_sleep(K_MSEC(10));
        }
    }

    return 0;
}

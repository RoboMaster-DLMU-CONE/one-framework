// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#define DT_DRV_COMPAT dji_dbus

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/input/input.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(dji_dbus, CONFIG_INPUT_LOG_LEVEL);

/* 驱动配置 */
struct dbus_input_channel
{
    uint32_t dbus_channel;
    uint32_t type;
    uint32_t zephyr_code;
};

/* DBUS使用的UART配置 - 大疆遥控器使用100000波特率奇偶校验 */
const struct uart_config uart_cfg_dbus = {.baudrate = 100000,
                                          .parity = UART_CFG_PARITY_EVEN,
                                          .stop_bits = UART_CFG_STOP_BITS_2,
                                          .data_bits = UART_CFG_DATA_BITS_8,
                                          .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

struct input_dbus_config
{
    uint8_t num_channels;
    const struct dbus_input_channel* channel_info;
    const struct device* uart_dev;
    uart_irq_callback_user_data_t cb;
};

#define DBUS_FRAME_LEN 18
#define DBUS_TRANSMISSION_TIME_MS 4
#define DBUS_INTERFRAME_SPACING_MS 20
#define DBUS_CHANNEL_COUNT 28 /* 通道总数：5个摇杆+2个开关+4个鼠标+1个滚轮+16个键盘 */

#define RC_CH_VALUE_OFFSET 1024
#define REPORT_FILTER CONFIG_INPUT_DBUS_REPORT_FILTER
#define CHANNEL_VALUE_ZERO CONFIG_INPUT_DBUS_CHANNEL_VALUE_ZERO
#define CHANNEL_VALUE_ONE CONFIG_INPUT_DBUS_CHANNEL_VALUE_ONE


struct input_dbus_data
{
    struct k_thread thread;
    struct k_sem report_lock;

    uint16_t xfer_bytes;
    uint8_t rd_data[DBUS_FRAME_LEN];
    uint8_t dbus_frame[DBUS_FRAME_LEN];
    bool in_sync;
    uint32_t last_rx_time;

    uint16_t last_reported_value[DBUS_CHANNEL_COUNT];
    int8_t channel_mapping[DBUS_CHANNEL_COUNT];

    K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_INPUT_DBUS_THREAD_STACK_SIZE);
};

static void input_dbus_report(const struct device* dev, unsigned int dbus_channel, unsigned int value)
{
    const struct input_dbus_config* const config = dev->config;
    struct input_dbus_data* const data = dev->data;

    int channel = data->channel_mapping[dbus_channel];

    /* 未映射 */
    if (channel == -1)
    {
        return;
    }

    if (value >= (data->last_reported_value[channel] + REPORT_FILTER) ||
        value <= (data->last_reported_value[channel] - REPORT_FILTER))
    {
        switch (config->channel_info[channel].type)
        {
        case INPUT_EV_ABS:
        case INPUT_EV_MSC:
            input_report(dev, config->channel_info[channel].type, config->channel_info[channel].zephyr_code, value,
                         false, K_FOREVER);
            break;

        default:
            if (value > CHANNEL_VALUE_ONE)
            {
                input_report_key(dev, config->channel_info[channel].zephyr_code, 1, false, K_FOREVER);
            }
            else if (value < CHANNEL_VALUE_ZERO)
            {
                input_report_key(dev, config->channel_info[channel].zephyr_code, 0, false, K_FOREVER);
            }
        }
        data->last_reported_value[channel] = value;
    }
}

static void input_dbus_input_report_thread(const struct device* dev, void* dummy2, void* dummy3)
{
    struct input_dbus_data* const data = dev->data;
    uint8_t* dbus_buf;
    uint16_t keyboard, last_keyboard = 0;
    bool connected_reported = false;

    ARG_UNUSED(dummy2);
    ARG_UNUSED(dummy3);

    while (true)
    {
        if (!data->in_sync)
        {
            k_sem_take(&data->report_lock, K_FOREVER);
            if (data->in_sync)
            {
                LOG_DBG("DBUS receiver connected");
            }
            else
            {
                continue;
            }
        }
        else
        {
            int ret = k_sem_take(&data->report_lock, K_MSEC(DBUS_INTERFRAME_SPACING_MS));
            if (ret == -EBUSY)
            {
                continue;
            }
            else if (ret < 0 || !data->in_sync)
            {
                /* 与UART接收器失去同步 */
                unsigned int key = irq_lock();

                data->in_sync = false;
                data->xfer_bytes = 0;
                irq_unlock(key);

                connected_reported = false;
                LOG_DBG("DBUS receiver connection lost");

                /* 报告连接丢失 */
                continue;
            }
        }

        if (!connected_reported)
        {
            LOG_DBG("DBUS controller connected");
            connected_reported = true;
        }

        /* 解析DBUS数据 */
        dbus_buf = data->dbus_frame;

        /* 报告通道1-4（遥控器摇杆） */
        input_dbus_report(dev, 0, ((uint16_t)dbus_buf[0] | ((uint16_t)dbus_buf[1] << 8)) & 0x07FF);
        input_dbus_report(dev, 1, (((uint16_t)dbus_buf[1] >> 3) | ((uint16_t)dbus_buf[2] << 5)) & 0x07FF);
        input_dbus_report(
            dev, 2,
            (((uint16_t)dbus_buf[2] >> 6) | ((uint16_t)dbus_buf[3] << 2) | ((uint16_t)dbus_buf[4] << 10)) & 0x07FF);
        input_dbus_report(dev, 3, (((uint16_t)dbus_buf[4] >> 1) | ((uint16_t)dbus_buf[5] << 7)) & 0x07FF);

        /* 报告开关位置 - 通道5-6 */
        input_dbus_report(dev, 4, ((dbus_buf[5] >> 4) & 0x0003));
        input_dbus_report(dev, 5, ((dbus_buf[5] >> 6) & 0x0003));

        /* 鼠标数据 - 通道7-10 */
        input_dbus_report(dev, 6, (int16_t)(dbus_buf[6] | (dbus_buf[7] << 8))); /* X轴 */
        input_dbus_report(dev, 7, (int16_t)(dbus_buf[8] | (dbus_buf[9] << 8))); /* Y轴 */
        input_dbus_report(dev, 8, (int16_t)(dbus_buf[10] | (dbus_buf[11] << 8))); /* Z轴 */
        input_dbus_report(dev, 9, (dbus_buf[12] & 0x01) ? 1 : 0); /* 左键 */
        input_dbus_report(dev, 10, (dbus_buf[12] & 0x02) ? 1 : 0); /* 右键 */

        /* 滚轮数据 - 通道11 */
        input_dbus_report(dev, 11, ((uint16_t)dbus_buf[16] | ((uint16_t)dbus_buf[17] << 8)) & 0x07FF);

        /* 键盘数据 - 通道12-27 */
        keyboard = dbus_buf[14] | (dbus_buf[15] << 8);
        if (keyboard != last_keyboard)
        {
            for (int i = 0; i < 16; i++)
            {
                if ((keyboard & (1 << i)) != (last_keyboard & (1 << i)))
                {
                    input_dbus_report(dev, 12 + i, (keyboard & (1 << i)) ? 1 : 0);
                }
            }
            last_keyboard = keyboard;
        }

#ifdef CONFIG_INPUT_DBUS_SEND_SYNC
        input_report(dev, 0, 0, 0, true, K_FOREVER);
#endif
    }
}

static void dbus_uart_isr(const struct device* uart_dev, void* user_data)
{
    const struct device* dev = user_data;
    struct input_dbus_data* const data = dev->data;
    uint8_t* rd_data = data->rd_data;

    if (uart_dev == NULL)
    {
        LOG_DBG("UART设备为NULL");
        return;
    }

    if (!uart_irq_update(uart_dev))
    {
        LOG_DBG("无法开始处理中断");
        return;
    }

    while (uart_irq_rx_ready(uart_dev) && data->xfer_bytes < DBUS_FRAME_LEN)
    {
        if (data->in_sync)
        {
            if (data->xfer_bytes == 0)
            {
                data->last_rx_time = k_uptime_get_32();
            }
            data->xfer_bytes += uart_fifo_read(uart_dev, &rd_data[data->xfer_bytes], DBUS_FRAME_LEN - data->xfer_bytes);
        }
        else
        {
            /* DBUS没有特定的帧头/帧尾，使用帧长度来同步 */
            data->xfer_bytes += uart_fifo_read(uart_dev, &rd_data[data->xfer_bytes], DBUS_FRAME_LEN - data->xfer_bytes);

            if (data->xfer_bytes == DBUS_FRAME_LEN)
            {
                data->in_sync = true;
            }
        }
    }

    if (data->in_sync && (k_uptime_get_32() - data->last_rx_time > DBUS_INTERFRAME_SPACING_MS))
    {
        data->in_sync = false;
        data->xfer_bytes = 0;
        k_sem_give(&data->report_lock);
    }
    else if (data->in_sync && data->xfer_bytes == DBUS_FRAME_LEN)
    {
        data->xfer_bytes = 0;
        memcpy(data->dbus_frame, rd_data, DBUS_FRAME_LEN);
        k_sem_give(&data->report_lock);
    }
}

static int input_dbus_init(const struct device* dev)
{
    const struct input_dbus_config* const config = dev->config;
    struct input_dbus_data* const data = dev->data;
    int i, ret;

    uart_irq_rx_disable(config->uart_dev);
    uart_irq_tx_disable(config->uart_dev);

    LOG_DBG("初始化DBUS驱动");

    for (i = 0; i < DBUS_CHANNEL_COUNT; i++)
    {
        data->last_reported_value[i] = 0;
        data->channel_mapping[i] = -1;
    }

    data->xfer_bytes = 0;
    data->in_sync = false;
    data->last_rx_time = 0;

    for (i = 0; i < config->num_channels; i++)
    {
        data->channel_mapping[config->channel_info[i].dbus_channel] = i;
    }

    ret = uart_configure(config->uart_dev, &uart_cfg_dbus);
    if (ret < 0)
    {
        LOG_ERR("无法配置UART端口: %d", ret);
        return ret;
    }

    ret = uart_irq_callback_user_data_set(config->uart_dev, config->cb, (void*)dev);
    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            LOG_ERR("中断驱动的UART API支持未启用");
        }
        else if (ret == -ENOSYS)
        {
            LOG_ERR("UART设备不支持中断驱动API");
        }
        else
        {
            LOG_ERR("设置UART回调错误: %d", ret);
        }
        return ret;
    }

    k_sem_init(&data->report_lock, 0, 1);

    uart_irq_rx_enable(config->uart_dev);

    k_thread_create(&data->thread, data->thread_stack, K_KERNEL_STACK_SIZEOF(data->thread_stack),
                    (k_thread_entry_t)input_dbus_input_report_thread, (void*)dev, NULL, NULL,
                    CONFIG_INPUT_DBUS_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(&data->thread, dev->name);

    return ret;
}

#define INPUT_CHANNEL_CHECK(input_channel_id)                                                                          \
    BUILD_ASSERT(IN_RANGE(DT_PROP(input_channel_id, channel), 0, 26), "无效的通道号");                                 \
    BUILD_ASSERT(DT_PROP(input_channel_id, type) == INPUT_EV_ABS || DT_PROP(input_channel_id, type) == INPUT_EV_KEY || \
                     DT_PROP(input_channel_id, type) == INPUT_EV_MSC,                                                  \
                 "无效的通道类型");

#define DBUS_INPUT_CHANNEL_INITIALIZER(input_channel_id)                                                               \
    {                                                                                                                  \
        .dbus_channel = DT_PROP(input_channel_id, channel),                                                            \
        .type = DT_PROP(input_channel_id, type),                                                                       \
        .zephyr_code = DT_PROP(input_channel_id, zephyr_code),                                                         \
    },

#define INPUT_DBUS_INIT(n)                                                                                             \
                                                                                                                       \
    static const struct dbus_input_channel input_##n[] = {DT_INST_FOREACH_CHILD(n, DBUS_INPUT_CHANNEL_INITIALIZER)};   \
    DT_INST_FOREACH_CHILD(n, INPUT_CHANNEL_CHECK)                                                                      \
                                                                                                                       \
    static struct input_dbus_data dbus_data_##n;                                                                       \
                                                                                                                       \
    static const struct input_dbus_config dbus_cfg_##n = {                                                             \
        .channel_info = input_##n,                                                                                     \
        .uart_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                                                                     \
        .num_channels = ARRAY_SIZE(input_##n),                                                                         \
        .cb = dbus_uart_isr,                                                                                           \
    };                                                                                                                 \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(n, input_dbus_init, NULL, &dbus_data_##n, &dbus_cfg_##n, POST_KERNEL,                        \
                          CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_DBUS_INIT)

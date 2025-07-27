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

/* DBUS UART配置 - 100kbps波特率 奇偶校验 */
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

static const struct dbus_input_channel input_channels_full[DBUS_CHANNEL_COUNT] = {
    {0, INPUT_EV_ABS, INPUT_ABS_RX}, /* right_stick_x */
    {1, INPUT_EV_ABS, INPUT_ABS_RY}, /* right_stick_y */
    {2, INPUT_EV_ABS, INPUT_ABS_X}, /* left_stick_x */
    {3, INPUT_EV_ABS, INPUT_ABS_Y}, /* left_stick_y */
    {4, INPUT_EV_KEY, INPUT_KEY_F1}, /* switch_right */
    {5, INPUT_EV_KEY, INPUT_KEY_F2}, /* switch_left */
    {6, INPUT_EV_REL, INPUT_REL_X}, /* mouse_x */
    {7, INPUT_EV_REL, INPUT_REL_Y}, /* mouse_y */
    {8, INPUT_EV_REL, INPUT_REL_Z}, /* mouse_z */
    {9, INPUT_EV_KEY, INPUT_BTN_LEFT}, /* mouse_left */
    {10, INPUT_EV_KEY, INPUT_BTN_RIGHT}, /* mouse_right */
    {11, INPUT_EV_ABS, INPUT_ABS_WHEEL}, /* wheel */
    /* 键盘 16 键 */
    {12, INPUT_EV_KEY, INPUT_KEY_W},
    {13, INPUT_EV_KEY, INPUT_KEY_S},
    {14, INPUT_EV_KEY, INPUT_KEY_D},
    {15, INPUT_EV_KEY, INPUT_KEY_A},
    {16, INPUT_EV_KEY, INPUT_KEY_RIGHTSHIFT},
    {17, INPUT_EV_KEY, INPUT_KEY_RIGHTCTRL},
    {18, INPUT_EV_KEY, INPUT_KEY_Q},
    {19, INPUT_EV_KEY, INPUT_KEY_E},
    {20, INPUT_EV_KEY, INPUT_KEY_R},
    {21, INPUT_EV_KEY, INPUT_KEY_F},
    {22, INPUT_EV_KEY, INPUT_KEY_G},
    {23, INPUT_EV_KEY, INPUT_KEY_Z},
    {24, INPUT_EV_KEY, INPUT_KEY_X},
    {25, INPUT_EV_KEY, INPUT_KEY_C},
    {26, INPUT_EV_KEY, INPUT_KEY_V},
    {27, INPUT_EV_KEY, INPUT_KEY_B},
};

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

    LOG_DBG("report dbus_ch=%u map_idx=%d type=%u code=%u val=%u", dbus_channel, channel,
            config->channel_info[channel].type, config->channel_info[channel].zephyr_code, value);
    /* 未映射 */
    if (channel < 0)
        return;

    if (value < data->last_reported_value[channel] + REPORT_FILTER &&
        value > data->last_reported_value[channel] - REPORT_FILTER)
        return;

    input_report(dev, config->channel_info[channel].type, config->channel_info[channel].zephyr_code, value, false,
                 K_FOREVER);
}

static void input_dbus_input_report_thread(const struct device* dev, void* dummy2, void* dummy3)
{
    struct input_dbus_data* const data = dev->data;
    const struct input_dbus_config* const config = dev->config;
    uint16_t last_keyboard = 0;
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
        const uint8_t* dbus_buf = data->dbus_frame;

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
        const uint16_t keyboard = dbus_buf[14] | (dbus_buf[15] << 8);
        if (keyboard != last_keyboard)
        {
            for (int i = 0; i < 16; i++)
            {
                const uint16_t bit = 1u << i;
                if ((keyboard & bit) != (last_keyboard & bit))
                {
                    /* 只报变化那一位 */
                    const struct dbus_input_channel* ch = &config->channel_info[12 + i];
                    input_report_key(dev, ch->zephyr_code, (keyboard & bit) ? 1 : 0, false, K_FOREVER);
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

    LOG_DBG("num_channels: %d", config->num_channels);
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

static struct input_dbus_data dbus_data;
static const struct input_dbus_config dbus_cfg = {
    .channel_info = input_channels_full,
    .uart_dev = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(0))),
    .num_channels = ARRAY_SIZE(input_channels_full),
    .cb = dbus_uart_isr,
};

DEVICE_DT_INST_DEFINE(0, input_dbus_init, NULL, &dbus_data, &dbus_cfg, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

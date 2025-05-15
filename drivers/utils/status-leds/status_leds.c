#define DT_DRV_COMPAT status_leds

#include <OF/drivers/utils/status_leds.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(status_leds, CONFIG_STATUS_LEDS_LOG_LEVEL);


static void heartbeat_timer_handler(const struct k_timer* timer)
{
    const struct device* dev = k_timer_user_data_get(timer);
    struct status_leds_data* data = dev->data;
    const struct status_leds_config* config = dev->config;

    if (!data->have_error)
    {
        if (!data->heartbeat_always_on)
        {
            data->heartbeat_state = !data->heartbeat_state;
            if (data->heartbeat_state)
            {
                led_on(config->leds, config->heartbeat_led_index);
            }
            else
            {
                led_off(config->leds, config->heartbeat_led_index);
            }
        }
        else
        {
            led_on(config->leds, config->heartbeat_led_index);
        }
    }
}

static void error_single_blink_handler(const struct k_timer* timer)
{
    const struct device* dev = k_timer_user_data_get(timer);
    struct status_leds_data* data = dev->data;
    const struct status_leds_config* config = dev->config;

    // LED状态翻转
    data->error_led_state = !data->error_led_state;

    if (data->error_led_state)
    {
        led_on(config->leds, config->error_led_index);
    }
    else
    {
        led_off(config->leds, config->error_led_index);
        data->error_blink_count++; // 完成一次闪烁

        // 如果达到设定次数，停止闪烁
        if (data->error_blink_count >= data->error_blink_times)
        {
            k_timer_stop(&data->error_single_blink_timer);
        }
    }
}

/**
 * @brief 错误闪烁定时器处理函数
 *
 * @param timer k_timer 定时器指针
 */
static void error_blink_timer_handler(const struct k_timer* timer)
{
    const struct device* dev = k_timer_user_data_get(timer);
    struct status_leds_data* data = dev->data;
    const struct status_leds_config* config = dev->config;

    if (data->have_error)
    {
        // 重置计数和状态
        data->error_blink_count = 0;
        data->error_led_state = false;
        led_off(config->leds, config->error_led_index);

        if (data->error_blink_times > 0)
        {
            // 计算单次闪烁时间，确保每次闪烁时间合理
            // 总闪烁时间为周期的70%，每次完整闪烁需要两次状态切换
            const uint32_t active_period_ms = config->error_interval_ms * 70 / 100;
            uint32_t blink_period_ms = active_period_ms / (data->error_blink_times * 2);

            // 确保每次闪烁至少有最小间隔
            blink_period_ms = MAX(blink_period_ms, 50);

            // 启动单次闪烁定时器
            k_timer_start(&data->error_single_blink_timer, K_NO_WAIT, K_MSEC(blink_period_ms));
        }
    }
}

static void status_set_heartbeat(const struct device* dev)
{
    struct status_leds_data* data = dev->data;
    data->heartbeat_always_on = false;
}

static void status_set_heartbeat_always_on(const struct device* dev)
{
    struct status_leds_data* data = dev->data;
    data->heartbeat_always_on = true;
}

static int status_leds_set_error_always_on(const struct device* dev)
{
    const struct status_leds_config* config = dev->config;
    struct status_leds_data* data = dev->data;
    data->have_error = true;
    led_off(config->leds, config->heartbeat_led_index);
    k_timer_stop(&data->error_blink_timer);
    k_timer_stop(&data->error_single_blink_timer);
    return led_on(config->leds, config->error_led_index);
}

static int status_leds_set_error_blink(const struct device* dev, const uint8_t times)
{
    const struct status_leds_config* config = dev->config;
    struct status_leds_data* data = dev->data;

    data->have_error = true;
    data->error_blink_times = times;
    data->error_blink_count = 0;
    data->error_led_state = false;

    led_off(config->leds, config->heartbeat_led_index);

    led_off(config->leds, config->error_led_index);

    k_timer_start(&data->error_blink_timer, K_NO_WAIT, K_MSEC(config->error_interval_ms));

    return 0;
}

static void status_leds_remove_error(const struct device* dev)
{
    const struct status_leds_config* config = dev->config;
    struct status_leds_data* data = dev->data;
    // 停止所有错误LED相关的定时器
    k_timer_stop(&data->error_blink_timer);
    k_timer_stop(&data->error_single_blink_timer);
    // 关闭错误LED
    led_off(config->leds, config->error_led_index);

    // 清除错误状态
    data->have_error = false;
    data->error_blink_count = 0;
    data->error_led_state = false;

    // 如果有心跳定时器，确保重启心跳LED的工作
    if (config->heartbeat_blink_interval_ms > 0)
    {
        if (data->heartbeat_always_on)
        {
            led_on(config->leds, config->heartbeat_led_index);
        }
        else
        {
            data->heartbeat_state = true;
            led_on(config->leds, config->heartbeat_led_index);
        }
    }
}

static const struct status_leds_api status_leds_funcs = {
    .set_heartbeat = &status_set_heartbeat,
    .set_heartbeat_always_on = &status_set_heartbeat_always_on,
    .set_error_always_on = &status_leds_set_error_always_on,
    .set_error_blink = &status_leds_set_error_blink,
    .remove_error = &status_leds_remove_error,
};

static int status_leds_init(const struct device* dev)
{
    const struct status_leds_config* config = dev->config;
    struct status_leds_data* data = dev->data;


    if (!device_is_ready(config->leds))
    {
        return -ENODEV;
    }

    k_timer_init(&data->heartbeat_timer, (k_timer_expiry_t)heartbeat_timer_handler, NULL);
    k_timer_user_data_set(&data->heartbeat_timer, (void*)dev);
    k_timer_init(&data->error_blink_timer, (k_timer_expiry_t)error_blink_timer_handler, NULL);
    k_timer_user_data_set(&data->error_blink_timer, (void*)dev);

    k_timer_init(&data->error_single_blink_timer, (k_timer_expiry_t)error_single_blink_handler, NULL);
    k_timer_user_data_set(&data->error_single_blink_timer, (void*)dev);

    if (config->heartbeat_blink_interval_ms > 0)
        k_timer_start(&data->heartbeat_timer, K_MSEC(config->heartbeat_blink_interval_ms),
                      K_MSEC(config->heartbeat_blink_interval_ms));

    return 0;
}

#define STATUS_LEDS_DEFINE(inst)                                                                                       \
    static struct status_leds_data data##inst;                                                                         \
    static struct status_leds_config config##inst = {                                                                  \
        .leds = DEVICE_DT_GET(DT_INST_PHANDLE(inst, leds)),                                                            \
        .heartbeat_led_index = DT_INST_PROP_OR(inst, heartbeat_led_index, 0U),                                         \
        .error_led_index = DT_INST_PROP_OR(inst, error_led_index, 0U),                                                 \
        .heartbeat_blink_interval_ms = CONFIG_HEARTBEAT_LED_BLINK_INTERVAL,                                            \
        .error_interval_ms = CONFIG_ERROR_LED_INTERVAL,                                                                \
    };                                                                                                                 \
    DEVICE_DT_INST_DEFINE(inst, status_leds_init, NULL, &data##inst, &config##inst, POST_KERNEL,                       \
                          CONFIG_STATUS_LEDS_INIT_PRIORITY, &status_leds_funcs);

DT_INST_FOREACH_STATUS_OKAY(STATUS_LEDS_DEFINE)

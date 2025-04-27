#define DT_DRV_COMPAT status_leds

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/devicetree.h>
#include <app/drivers/status_leds.h>

#include "zephyr/logging/log.h"

#if !DT_HAS_COMPAT_STATUS_OKAY(status_leds)
#error "No utils,status-leds nodes found - check compatible string!"
#endif
LOG_MODULE_REGISTER(status_leds, CONFIG_STATUS_LEDS_LOG_LEVEL);


static void heartbeat_timer_handler(struct k_timer *timer) {
    const struct device *dev = k_timer_user_data_get(timer);
    struct status_leds_data *data = dev->data;
    const struct status_leds_config *config = dev->config;

    data->heartbeat_state = !data->heartbeat_state;

    if (data->heartbeat_state) {
        led_on(config->leds, config->heartbeat_led_index);
    } else {
        led_off(config->leds, config->heartbeat_led_index);
    }
}

static int status_leds_set_error(const struct device *dev, const bool state) {
    const struct status_leds_config *config = dev->config;

    if (state) {
        return led_on(config->leds, config->error_led_index);
    }
    return led_off(config->leds, config->error_led_index);
}

static const struct status_leds_api status_leds_funcs = {
    .set_error = &status_leds_set_error,
};

static int status_leds_init(const struct device *dev) {
    const struct status_leds_config *config = dev->config;
    struct status_leds_data *data = dev->data;


    if (!device_is_ready(config->leds)) {
        return -ENODEV;
    }

    k_timer_init(&data->heartbeat_timer, heartbeat_timer_handler, NULL);
    k_timer_user_data_set(&data->heartbeat_timer, (void *) dev);
    if (config->interval_ms > 0)
        k_timer_start(&data->heartbeat_timer, K_MSEC(config->interval_ms), K_MSEC(config->interval_ms));

    return 0;
}

#define STATUS_LEDS_DEFINE(inst)                                \
    static struct status_leds_data data##inst;                  \
    static struct status_leds_config config##inst = {           \
        .leds = DEVICE_DT_GET(DT_INST_PHANDLE(inst, leds)),     \
        .heartbeat_led_index = DT_INST_PROP_OR(inst, heartbeat_led_index, 0U),                                \
        .error_led_index = DT_INST_PROP_OR(inst, error_led_index, 0U),                                \
        .interval_ms = CONFIG_HEARTBEAT_LED_INTERVAL,           \
    };                                                          \
    DEVICE_DT_INST_DEFINE(inst,                                 \
                          status_leds_init,                     \
                          NULL,                                 \
                          &data##inst,                          \
                          &config##inst,                        \
                          POST_KERNEL,                          \
                          CONFIG_STATUS_LEDS_INIT_PRIORITY,     \
                          &status_leds_funcs);

DT_INST_FOREACH_STATUS_OKAY(STATUS_LEDS_DEFINE)
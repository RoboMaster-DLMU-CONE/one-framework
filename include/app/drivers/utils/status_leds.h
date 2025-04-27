#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H
#include <zephyr/device.h>

struct status_leds_config
{
    const struct device* leds;
    const uint32_t heartbeat_led_index;
    const uint32_t error_led_index;
    unsigned int heartbeat_blink_interval_ms;
    unsigned int error_interval_ms;
};

struct status_leds_data
{
    struct k_timer heartbeat_timer;
    struct k_timer error_blink_timer;
    struct k_timer error_single_blink_timer;
    bool heartbeat_state;
    bool heartbeat_always_on;
    bool have_error;
    uint8_t error_blink_times;
    uint8_t error_blink_count;
    bool error_led_state;
};

__subsystem struct status_leds_api
{
    void (*set_heartbeat)(const struct device* dev);
    void (*set_heartbeat_always_on)(const struct device* dev);
    int (*set_error_always_on)(const struct device* dev);
    int (*set_error_blink)(const struct device* dev, const uint8_t times);
    void (*remove_error)(const struct device* dev);
};

#endif //STATUS_LEDS_H

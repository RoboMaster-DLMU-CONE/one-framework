#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>

struct status_leds_config {
    const struct device* leds;
    const uint32_t heartbeat_led_index;
    const uint32_t error_led_index;
    unsigned int interval_ms;
};

struct status_leds_data {
    struct k_timer heartbeat_timer;
    bool heartbeat_state;
};

__subsystem struct status_leds_api {
    int (*set_error)(const struct device *dev, const bool state);
};

#endif //STATUS_LEDS_H
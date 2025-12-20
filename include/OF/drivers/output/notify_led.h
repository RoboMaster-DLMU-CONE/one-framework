#ifndef OF_NOTIFY_LED_H
#define OF_NOTIFY_LED_H

#include <zephyr/device.h>
#include <stdint.h>

struct led_color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef int (*led_set_pixel_t)(const struct device* dev, struct led_color color);

__subsystem struct notify_led_api
{
    led_set_pixel_t set_pixel;
};

/**
 * @brief 设置逻辑 LED 的颜色
 *
 * @param dev 设备指针 (指向你的虚拟设备)
 * @param color RGB 颜色
 * @return 0 成功, 负数 失败
 */
static int notify_led_set_pixel(const struct device* dev, const struct led_color color)
{
    const struct notify_led_api* api =
        (const struct notify_led_api*)dev->api;
    return api->set_pixel(dev, color);
}

#endif //OF_NOTIFY_LED_H

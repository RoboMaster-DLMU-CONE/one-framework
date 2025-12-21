#ifndef OF_LED_PIXEL_H
#define OF_LED_PIXEL_H

#include <zephyr/device.h>
#include <stdint.h>

#define _HEX_C2I(c) ( \
(c) >= '0' && (c) <= '9' ? (c) - '0' : \
(c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
(c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : 0 \
)

/* 2. 判断是否有 '#' 前缀，决定偏移量
 *    如果 s[0] 是 '#', 偏移量为 1, 否则为 0
 */
#define _HEX_OFF(s) ((s)[0] == '#' ? 1 : 0)

/* 3. 获取第 I 个字节的高位和低位字符，合成字节
 *    s: 字符串
 *    i: 第几个颜色分量 (0=R, 1=G, 2=B)
 *    off: 偏移量
 */
#define _HEX_BYTE(s, i, off) ( \
(_HEX_C2I((s)[(off) + (i)*2]) << 4) | \
(_HEX_C2I((s)[(off) + (i)*2 + 1])) \
)

#define COLOR_HEX(s) { \
.r = _HEX_BYTE(s, 0, _HEX_OFF(s)), \
.g = _HEX_BYTE(s, 1, _HEX_OFF(s)), \
.b = _HEX_BYTE(s, 2, _HEX_OFF(s))  \
}


struct led_color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef int (*led_set_pixel_t)(const struct device* dev, struct led_color color, const uint8_t bright_percent);
typedef int (*led_pixel_off_t)(const struct device* dev);
typedef int (*led_pixel_on_t)(const struct device* dev);

__subsystem struct led_pixel_api
{
    led_set_pixel_t set_led_pixel;
    led_pixel_off_t close_led;
    led_pixel_on_t open_led;
};

/**
 * @brief 设置像素 LED 的颜色
 *
 * @param dev 设备指针 (指向你的虚拟设备)
 * @param color RGB 颜色
 * @param bright_percent 亮度百分比 (0 ~ 100)
 * @return 0 成功, 负数 失败
 */
static inline int led_pixel_set(const struct device* dev, const struct led_color color, const uint8_t bright_percent)
{
    const struct led_pixel_api* api =
        (const struct led_pixel_api*)dev->api;
    return api->set_led_pixel(dev, color, bright_percent);
}

static inline int led_pixel_off(const struct device* dev)
{
    const struct led_pixel_api* api = (const struct led_pixel_api*)dev->api;
    return api->close_led(dev);
}

static inline int led_pixel_on(const struct device* dev)
{
    const struct led_pixel_api* api = (const struct led_pixel_api*)dev->api;
    return api->open_led(dev);
}

#endif //OF_LED_PIXEL_H

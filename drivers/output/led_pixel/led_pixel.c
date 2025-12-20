#define DT_DRV_COMPAT led_pixel_pwm
// #define DT_DRV_COMPAT_STRIP notify_led_strip

#include <OF/drivers/output/led_pixel.h>

#include <zephyr/drivers/led.h>

static inline uint8_t led_scale_int(const uint16_t v)
{
    /* (v * 100 + 256) >> 9    等价于 round((v*100)/512) */
    return (uint8_t)(((uint32_t)v * 100U + 256U) >> 9);
}

struct led_pixel_pwm_config
{
    const struct device *dev_r, *dev_g, *dev_b;
    uint32_t ch_r, ch_g, ch_b;
};

static int led_pixel_pwm_set(const struct device* dev, const struct led_color color)
{
    const struct led_pixel_pwm_config* cfg = dev->config;
    const struct
    {
        const struct device* dev;
        uint32_t ch;
        uint8_t bright;
    } leds[] = {
        {cfg->dev_r, cfg->ch_r, led_scale_int(color.r)},
        {cfg->dev_g, cfg->ch_g, led_scale_int(color.g)},
        {cfg->dev_b, cfg->ch_b, led_scale_int(color.b)},
    };

    for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i)
    {
        int ret = led_set_brightness(leds[i].dev, leds[i].ch, leds[i].bright);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

static int led_pixel_pwm_off(const struct device* dev)
{
    const struct led_pixel_pwm_config* cfg = dev->config;
    const struct
    {
        const struct device* dev;
        uint32_t ch;
    } leds[] = {
        {cfg->dev_r, cfg->ch_r},
        {cfg->dev_g, cfg->ch_g},
        {cfg->dev_b, cfg->ch_b},
    };

    for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i)
    {
        int ret = led_off(leds[i].dev, leds[i].ch);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

static int led_pixel_pwm_on(const struct device* dev)
{
    const struct led_pixel_pwm_config* cfg = dev->config;
    const struct
    {
        const struct device* dev;
        uint32_t ch;
    } leds[] = {
        {cfg->dev_r, cfg->ch_r},
        {cfg->dev_g, cfg->ch_g},
        {cfg->dev_b, cfg->ch_b},
    };

    for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); ++i)
    {
        int ret = led_on(leds[i].dev, leds[i].ch);
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

// TODO: test ws2812 on cat board and add strip compatibility here.
// mew.

#define GET_LED_DEV(inst, prop) \
    DEVICE_DT_GET(DT_PARENT(DT_INST_PHANDLE(inst, prop)))

#define GET_LED_IDX(inst, prop) \
    DT_NODE_CHILD_IDX(DT_INST_PHANDLE(inst, prop))

#define INST_PWM(n) \
    static const struct led_pixel_pwm_config config_pwm_##n = { \
        .dev_r = GET_LED_DEV(n, red_led), \
        .dev_g = GET_LED_DEV(n, green_led), \
        .dev_b = GET_LED_DEV(n, blue_led), \
        .ch_r = GET_LED_IDX(n, red_led), \
        .ch_g = GET_LED_IDX(n, green_led), \
        .ch_b = GET_LED_IDX(n, blue_led), \
    }; \
    \
    static const struct led_pixel_api api_pwm_##n = { \
        .set_led_pixel = led_pixel_pwm_set, \
        .close_led = led_pixel_pwm_off, \
        .open_led = led_pixel_pwm_on, \
    }; \
    \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, \
                          &config_pwm_##n, POST_KERNEL, 50, \
                          &api_pwm_##n);

DT_INST_FOREACH_STATUS_OKAY(INST_PWM);

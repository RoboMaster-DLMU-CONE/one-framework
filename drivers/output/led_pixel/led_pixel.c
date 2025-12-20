#define DT_DRV_COMPAT led_pixel_pwm
// #define DT_DRV_COMPAT_STRIP notify_led_strip

#include <OF/drivers/output/led_pixel.h>

#include <zephyr/drivers/led.h>

struct led_pixel_pwm_config
{
    const struct device *dev_r, *dev_g, *dev_b;
    uint32_t ch_r, ch_g, ch_b;
};

static int led_pixel_pwm_set(const struct device* dev, const struct led_color color)
{
    const struct led_pixel_pwm_config* cfg = dev->config;

    led_set_brightness(cfg->dev_r, cfg->ch_r, (float)color.r / 512.0f * 100.0f);
    led_set_brightness(cfg->dev_g, cfg->ch_g, (float)color.g / 512.0f * 100.0f);
    led_set_brightness(cfg->dev_b, cfg->ch_b, (float)color.b / 512.0f * 100.0f);
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
        .set_pixel = led_pixel_pwm_set, \
    }; \
    \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, \
                          &config_pwm_##n, POST_KERNEL, 50, \
                          &api_pwm_##n);

DT_INST_FOREACH_STATUS_OKAY(INST_PWM);

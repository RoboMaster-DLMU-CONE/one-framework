// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#define DT_DRV_COMPAT pwm_buzzer

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "OF/drivers/output/buzzer.h"

LOG_MODULE_REGISTER(pwm_buzzer, CONFIG_PWM_BUZZER_LOG_LEVEL);

struct pwm_buzzer_config {
    struct pwm_dt_spec pwm;
    uint32_t base_frequency_hz;  /* 基础频率 */
};

struct pwm_buzzer_data {
    uint32_t current_frequency_hz;  /* 当前播放频率 */
    uint8_t current_volume;         /* 当前音量 (0-100) */
    bool is_playing;                /* 是否正在播放 */
};

int pwm_buzzer_play_tone(const struct device *dev, uint32_t frequency_hz, uint8_t volume)
{
    const struct pwm_buzzer_config *config = dev->config;
    struct pwm_buzzer_data *data = dev->data;

    if (volume > 100) {
        LOG_ERR("音量超出范围: %d", volume);
        return -EINVAL;
    }

    if (frequency_hz == 0 || volume == 0) {
        /* 停止播放 */
        data->is_playing = false;
        data->current_frequency_hz = 0;
        return pwm_set_dt(&config->pwm, 0, 0);
    }

    /* 计算PWM周期 (微秒) */
    uint32_t period_us = 1000000 / frequency_hz;
    
    /* 计算占空比 (音量控制) - 50%为基础，音量控制占空比 */
    uint32_t duty_cycle_us = (period_us * volume) / 200;  /* volume/100 * 50% */

    int ret = pwm_set_dt(&config->pwm, period_us, duty_cycle_us);
    if (ret < 0) {
        LOG_ERR("设置PWM失败: %d", ret);
        return ret;
    }

    data->current_frequency_hz = frequency_hz;
    data->current_volume = volume;
    data->is_playing = true;

    LOG_DBG("播放音调: %u Hz, 音量: %u%%", frequency_hz, volume);
    return 0;
}

int pwm_buzzer_stop(const struct device *dev)
{
    return pwm_buzzer_play_tone(dev, 0, 0);
}

int pwm_buzzer_set_volume(const struct device *dev, uint8_t volume)
{
    struct pwm_buzzer_data *data = dev->data;
    
    if (volume > 100) {
        LOG_ERR("音量超出范围: %d", volume);
        return -EINVAL;
    }
    
    data->current_volume = volume;
    
    /* 如果正在播放，使用新的音量重新播放当前频率 */
    if (data->is_playing && data->current_frequency_hz > 0) {
        return pwm_buzzer_play_tone(dev, data->current_frequency_hz, volume);
    }
    
    return 0;
}

int pwm_buzzer_get_volume(const struct device *dev, uint8_t *volume)
{
    const struct pwm_buzzer_data *data = dev->data;
    
    if (volume == NULL) {
        return -EINVAL;
    }
    
    *volume = data->current_volume;
    return 0;
}

int pwm_buzzer_play_note(const struct device *dev, float note_multiplier, uint8_t volume)
{
    const struct pwm_buzzer_config *config = dev->config;
    
    if (note_multiplier <= 0) {
        LOG_ERR("音符倍数必须为正数: %f", (double)note_multiplier);
        return -EINVAL;
    }
    
    uint32_t frequency_hz = (uint32_t)(config->base_frequency_hz * note_multiplier);
    return pwm_buzzer_play_tone(dev, frequency_hz, volume);
}

static int pwm_buzzer_init(const struct device *dev)
{
    const struct pwm_buzzer_config *config = dev->config;
    struct pwm_buzzer_data *data = dev->data;

    if (!device_is_ready(config->pwm.dev)) {
        LOG_ERR("PWM设备未就绪");
        return -ENODEV;
    }

    /* 初始化数据 */
    data->current_frequency_hz = 0;
    data->current_volume = CONFIG_PWM_BUZZER_DEFAULT_VOLUME;
    data->is_playing = false;

    /* 确保初始状态为停止 */
    pwm_set_dt(&config->pwm, 0, 0);

    LOG_INF("PWM蜂鸣器初始化完成，基础频率: %u Hz", config->base_frequency_hz);
    return 0;
}

/* 设备树宏定义 */
#define PWM_BUZZER_INIT(n)                                                     \
    static const struct pwm_buzzer_config pwm_buzzer_config_##n = {           \
        .pwm = PWM_DT_SPEC_INST_GET(n),                                       \
        .base_frequency_hz = DT_INST_PROP(n, base_frequency_hz),              \
    };                                                                         \
                                                                               \
    static struct pwm_buzzer_data pwm_buzzer_data_##n;                        \
                                                                               \
    DEVICE_DT_INST_DEFINE(n, pwm_buzzer_init, NULL,                          \
                          &pwm_buzzer_data_##n, &pwm_buzzer_config_##n,       \
                          POST_KERNEL, CONFIG_PWM_BUZZER_INIT_PRIORITY,        \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(PWM_BUZZER_INIT)
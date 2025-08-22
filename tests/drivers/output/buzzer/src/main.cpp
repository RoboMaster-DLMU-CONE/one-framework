// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "OF/drivers/output/buzzer.h"
#include "OF/drivers/utils/status_leds.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪");
        return -1;
    }
    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);
    led_api->set_heartbeat(status_led_dev);

    // 初始化PWM蜂鸣器
    const device* buzzer = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer));
    if (!device_is_ready(buzzer))
    {
        LOG_ERR("PWM蜂鸣器设备未就绪");
        return -1;
    }

    LOG_INF("PWM蜂鸣器测试开始");

    int ret;
    uint8_t volume;

    // 测试获取当前音量
    ret = pwm_buzzer_get_volume(buzzer, &volume);
    if (ret == 0) {
        LOG_INF("当前音量: %u%%", volume);
    } else {
        LOG_ERR("获取音量失败: %d", ret);
    }

    // 测试设置音量
    ret = pwm_buzzer_set_volume(buzzer, 20);
    if (ret == 0) {
        LOG_INF("音量设置为20%%");
    } else {
        LOG_ERR("设置音量失败: %d", ret);
    }

    // 播放一系列测试音调
    const uint32_t test_frequencies[] = {440, 523, 659, 784, 880}; // A4, C5, E5, G5, A5
    const char* note_names[] = {"A4", "C5", "E5", "G5", "A5"};

    while (true)
    {
        for (int i = 0; i < 5; i++) {
            LOG_INF("播放音符: %s (%u Hz)", note_names[i], test_frequencies[i]);
            
            ret = pwm_buzzer_play_tone(buzzer, test_frequencies[i], 1);
            if (ret < 0) {
                LOG_ERR("播放音调失败: %d", ret);
            }
            
            k_sleep(K_MSEC(500));
        }

        // 停止播放
        LOG_INF("停止播放");
        ret = pwm_buzzer_stop(buzzer);
        if (ret < 0) {
            LOG_ERR("停止播放失败: %d", ret);
        }

        k_sleep(K_MSEC(1000));

        // 测试基于基础频率的音符播放
        LOG_INF("测试音符播放功能");
        const float note_multipliers[] = {1.0f, 1.125f, 1.25f, 1.5f, 2.0f}; // 基础音、大二度、小三度、完全五度、八度
        const char* interval_names[] = {"基础音", "大二度", "小三度", "完全五度", "八度"};

        for (int i = 0; i < 5; i++) {
            LOG_INF("播放音程: %s (倍数: %.3f)", interval_names[i], (double)note_multipliers[i]);
            
            ret = pwm_buzzer_play_note(buzzer, note_multipliers[i], 70);
            if (ret < 0) {
                LOG_ERR("播放音符失败: %d", ret);
            }
            
            k_sleep(K_MSEC(400));
        }

        // 停止播放
        pwm_buzzer_stop(buzzer);
        k_sleep(K_MSEC(2000));
    }
}
#ifndef PWM_BUZZER_H
#define PWM_BUZZER_H

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 播放指定频率的声音
 *
 * @param dev 蜂鸣器设备
 * @param frequency_hz 频率（Hz），0表示停止播放
 * @param volume 音量（0-100），0表示静音，100表示最大音量
 * @return 0表示成功，负数表示错误
 */
int pwm_buzzer_play_tone(const struct device *dev, uint32_t frequency_hz, uint8_t volume);

/**
 * @brief 停止播放声音
 *
 * @param dev 蜂鸣器设备
 * @return 0表示成功，负数表示错误
 */
int pwm_buzzer_stop(const struct device *dev);

/**
 * @brief 设置蜂鸣器音量
 *
 * @param dev 蜂鸣器设备
 * @param volume 音量（0-100）
 * @return 0表示成功，负数表示错误
 */
int pwm_buzzer_set_volume(const struct device *dev, uint8_t volume);

/**
 * @brief 获取当前音量
 *
 * @param dev 蜂鸣器设备
 * @param volume 指向存储当前音量的变量
 * @return 0表示成功，负数表示错误
 */
int pwm_buzzer_get_volume(const struct device *dev, uint8_t *volume);

/**
 * @brief 播放音符（基于基础频率的倍数）
 *
 * @param dev 蜂鸣器设备
 * @param note_multiplier 音符倍数（例如2.0表示高八度）
 * @param volume 音量（0-100）
 * @return 0表示成功，负数表示错误
 */
int pwm_buzzer_play_note(const struct device *dev, float note_multiplier, uint8_t volume);

#ifdef __cplusplus
}
#endif

#endif // PWM_BUZZER_H
#ifndef PWM_HEATER_H
#define PWM_HEATER_H

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief 获取加热器当前温度
 *
 * @param dev 加热器设备
 * @param temp_celsius 指向存储当前温度的变量 (摄氏度x100)
 * @return 0表示成功，负数表示错误
 */
int pwm_heater_get_current_temp(const struct device *dev, int32_t *temp_celsius);

/**
 * @brief 启用加热器
 *
 * @param dev 加热器设备
 * @return 0表示成功，负数表示错误
 */
int pwm_heater_enable(const struct device *dev);

/**
 * @brief 禁用加热器
 *
 * @param dev 加热器设备
 * @return 0表示成功，负数表示错误
 */
int pwm_heater_disable(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif //PWM_HEATER_H
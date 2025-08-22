# PWM蜂鸣器驱动

这是一个基于Zephyr PWM子系统的通用蜂鸣器驱动。

## 功能特性

- 基于PWM产生不同频率的音调
- 支持音量控制(0-100%)
- 支持基于基础频率的音符播放
- 设备树配置支持
- 符合Zephyr驱动标准

## 设备树配置

在你的设备树文件(`.dts`或`.overlay`)中添加蜂鸣器节点：

```dts
/ {
    pwm_buzzer: pwm-buzzer {
        compatible = "pwm-buzzer";
        pwms = <&pwm1 1 0 PWM_POLARITY_NORMAL>; // 使用PWM1控制器的通道1
        base-frequency-hz = <440>; // 基础频率为440Hz (A4音符)
        status = "okay";
    };
};
```

## Kconfig配置

在你的`prj.conf`文件中启用相关配置：

```
CONFIG_OUTPUT=y
CONFIG_PWM_BUZZER=y
CONFIG_PWM_BUZZER_DEFAULT_VOLUME=50
```

## API使用示例

```c
#include "OF/drivers/output/buzzer.h"

int main()
{
    // 获取蜂鸣器设备
    const struct device *buzzer = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer));
    if (!device_is_ready(buzzer)) {
        printk("蜂鸣器设备未就绪\n");
        return -1;
    }

    // 播放440Hz的声音，音量70%
    pwm_buzzer_play_tone(buzzer, 440, 70);
    k_sleep(K_MSEC(1000));

    // 基于基础频率播放音符 (2倍频率 = 高八度)
    pwm_buzzer_play_note(buzzer, 2.0f, 50);
    k_sleep(K_MSEC(500));

    // 停止播放
    pwm_buzzer_stop(buzzer);

    return 0;
}
```

## API参考

### pwm_buzzer_play_tone()
```c
int pwm_buzzer_play_tone(const struct device *dev, uint32_t frequency_hz, uint8_t volume);
```
播放指定频率的声音。

**参数:**
- `dev`: 蜂鸣器设备指针
- `frequency_hz`: 频率(Hz)，0表示停止播放
- `volume`: 音量(0-100)

**返回值:** 0表示成功，负数表示错误

### pwm_buzzer_play_note()
```c
int pwm_buzzer_play_note(const struct device *dev, float note_multiplier, uint8_t volume);
```
基于基础频率播放音符。

**参数:**
- `dev`: 蜂鸣器设备指针  
- `note_multiplier`: 音符倍数(例如2.0表示高八度)
- `volume`: 音量(0-100)

### pwm_buzzer_stop()
```c
int pwm_buzzer_stop(const struct device *dev);
```
停止播放声音。

### pwm_buzzer_set_volume() / pwm_buzzer_get_volume()
```c
int pwm_buzzer_set_volume(const struct device *dev, uint8_t volume);
int pwm_buzzer_get_volume(const struct device *dev, uint8_t *volume);
```
设置/获取蜂鸣器音量。
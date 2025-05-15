#include <OF/drivers/utils/status_leds.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const struct device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }
    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);

    led_api->set_heartbeat(status_led_dev);

    const struct device* accel = DEVICE_DT_GET(DT_NODELABEL(bmi088_accel));
    const struct device* gyro = DEVICE_DT_GET(DT_NODELABEL(bmi088_gyro));
    if (!device_is_ready(accel) || !device_is_ready(gyro))
    {
        LOG_ERR("BMI088 devices not ready");
        return -1;
    }
    LOG_INF("程序开始运行……");

    while (true)
    {
        k_sleep(K_MSEC(100));
    }

    return 0;
}

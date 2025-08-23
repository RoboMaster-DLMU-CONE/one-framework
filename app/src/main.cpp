#include <OF/drivers/output/status_leds.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

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


    LOG_INF("程序开始运行……");

    while (true)
    {
        k_sleep(K_MSEC(1000));
        printk("Hello\n");
    }

    return 0;
}

#include <OF/drivers/output/status_leds.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <OF/lib/CommBridge/Sender.hpp>
#include <OF/lib/CommBridge/Receiver.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static const device* uart_dev;

int main()
{

    const device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    OF::CommBridge::Sender<SampleA> receiver(uart_dev);

    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }

    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("Console 设备未就绪\n");
        return -1;
    }

    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);
    led_api->set_heartbeat(status_led_dev);

    LOG_INF("程序开始运行……");

    return 0;
}

#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/drivers/utils/status_leds.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
	const struct device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));;
	if (!device_is_ready(status_led_dev))
	{
		LOG_ERR("状态LED设备未就绪\n");
		return 0;
	}
	const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);

	led_api->set_heartbeat_always_on(status_led_dev);
	LOG_INF("状态LED设备心跳灯常亮... \n");
	k_sleep(K_SECONDS(5U));
	led_api->set_heartbeat(status_led_dev);
	LOG_INF("状态LED设备心跳灯闪烁... \n");
	k_sleep(K_SECONDS(5U));
	led_api->set_error_always_on(status_led_dev);
	LOG_INF("状态LED设备错误灯常亮... \n");
	k_sleep(K_SECONDS(5U));
	led_api->set_error_blink(status_led_dev, 3);
	LOG_INF("状态LED设备错误灯三闪... \n");
	k_sleep(K_SECONDS(5U));
	led_api->remove_error(status_led_dev);
	LOG_INF("状态LED设备移除错误... \n");
	k_sleep(K_SECONDS(5U));
	while (1)
	{
		LOG_INF("程序运行中... \n");
		k_sleep(K_SECONDS(1U));
	}

	return 0;
}

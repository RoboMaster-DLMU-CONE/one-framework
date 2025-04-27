#include <stdio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/drivers/status_leds.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void) {
	const struct device *status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));;
	if (!device_is_ready(status_led_dev)) {
		LOG_ERR("状态LED设备未就绪\n");
		return 0;
	}
	uint8_t counter = 0;
	const struct status_leds_api *led_api = status_led_dev->api;

	led_api->set_error(status_led_dev, false);
	while (1) {
		LOG_INF("程序运行中... \n");
		k_sleep(K_SECONDS(1U));

		if (++counter % 5 == 0) {
			static bool error_state = false;
			error_state = !error_state;
			led_api->set_error(status_led_dev, error_state);
			LOG_INF("错误LED状态: %s", error_state ? "开启" : "关闭");
		}
	}

	return 0;
}

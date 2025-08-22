// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/led.h>

#include "OF/drivers/output/status_leds.h"

LOG_MODULE_REGISTER(status_leds_test, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device* status_led_dev;
static const struct device* leds_dev;
static const struct status_leds_api* led_api;

static void test_status_leds_setup(void)
{
    status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    zassert_not_null(status_led_dev, "Status LED device should not be null");
    zassert_true(device_is_ready(status_led_dev), "Status LED device should be ready");
    
    leds_dev = DEVICE_DT_GET(DT_NODELABEL(leds));
    zassert_not_null(leds_dev, "LEDs device should not be null");
    zassert_true(device_is_ready(leds_dev), "LEDs device should be ready");
    
    led_api = static_cast<const status_leds_api*>(status_led_dev->api);
    zassert_not_null(led_api, "Status LED API should not be null");
    
    // Verify API functions are available
    zassert_not_null(led_api->set_heartbeat, "set_heartbeat API should not be null");
    zassert_not_null(led_api->set_heartbeat_always_on, "set_heartbeat_always_on API should not be null");
    zassert_not_null(led_api->set_error_always_on, "set_error_always_on API should not be null");
    zassert_not_null(led_api->set_error_blink, "set_error_blink API should not be null");
    zassert_not_null(led_api->remove_error, "remove_error API should not be null");
}

ZTEST(status_leds_tests, test_device_initialization)
{
    test_status_leds_setup();
    LOG_INF("Status LED device initialized successfully");
}

ZTEST(status_leds_tests, test_heartbeat_function)
{
    test_status_leds_setup();
    
    LOG_INF("Testing heartbeat function");
    
    // Test normal heartbeat mode
    led_api->set_heartbeat(status_led_dev);
    LOG_INF("Set to normal heartbeat mode");
    
    // Allow some time for heartbeat to start
    k_sleep(K_MSEC(100));
    
    LOG_INF("Heartbeat function test completed");
}

ZTEST(status_leds_tests, test_heartbeat_always_on)
{
    test_status_leds_setup();
    
    LOG_INF("Testing heartbeat always on function");
    
    // Test heartbeat always on mode
    led_api->set_heartbeat_always_on(status_led_dev);
    LOG_INF("Set heartbeat to always on mode");
    
    // Allow some time for change to take effect
    k_sleep(K_MSEC(100));
    
    LOG_INF("Heartbeat always on test completed");
}

ZTEST(status_leds_tests, test_error_always_on)
{
    test_status_leds_setup();
    
    LOG_INF("Testing error always on function");
    
    // Test error always on mode
    int ret = led_api->set_error_always_on(status_led_dev);
    zassert_equal(ret, 0, "Error always on should return 0 on success");
    LOG_INF("Set error LED to always on mode");
    
    // Allow some time for change to take effect
    k_sleep(K_MSEC(100));
    
    LOG_INF("Error always on test completed");
}

ZTEST(status_leds_tests, test_error_blink)
{
    test_status_leds_setup();
    
    LOG_INF("Testing error blink function");
    
    // Test error blink with different blink counts
    const uint8_t test_blink_counts[] = {1, 3, 5};
    const size_t num_tests = ARRAY_SIZE(test_blink_counts);
    
    for (size_t i = 0; i < num_tests; i++) {
        uint8_t blink_count = test_blink_counts[i];
        LOG_INF("Testing error blink with %u blinks", blink_count);
        
        int ret = led_api->set_error_blink(status_led_dev, blink_count);
        zassert_equal(ret, 0, "Error blink should return 0 on success");
        
        // Allow time for blink pattern to start
        k_sleep(K_MSEC(200));
        
        // Remove error to prepare for next test
        led_api->remove_error(status_led_dev);
        k_sleep(K_MSEC(100));
    }
    
    LOG_INF("Error blink test completed");
}

ZTEST(status_leds_tests, test_remove_error)
{
    test_status_leds_setup();
    
    LOG_INF("Testing remove error function");
    
    // First set an error state
    int ret = led_api->set_error_always_on(status_led_dev);
    zassert_equal(ret, 0, "Setting error should succeed");
    
    k_sleep(K_MSEC(100));
    
    // Then remove the error
    led_api->remove_error(status_led_dev);
    LOG_INF("Error state removed");
    
    // Allow time for change to take effect
    k_sleep(K_MSEC(100));
    
    LOG_INF("Remove error test completed");
}

ZTEST(status_leds_tests, test_error_blink_sequence)
{
    test_status_leds_setup();
    
    LOG_INF("Testing complete error blink sequence");
    
    // Test a complete blink sequence
    uint8_t blink_count = 3;
    int ret = led_api->set_error_blink(status_led_dev, blink_count);
    zassert_equal(ret, 0, "Error blink should return 0 on success");
    
    LOG_INF("Starting error blink sequence with %u blinks", blink_count);
    
    // Wait long enough for the complete blink sequence
    // CONFIG_ERROR_LED_INTERVAL default is 1000ms, plus some time for blinks
    k_sleep(K_MSEC(1500));
    
    // Remove error state
    led_api->remove_error(status_led_dev);
    
    LOG_INF("Error blink sequence test completed");
}

ZTEST(status_leds_tests, test_state_transitions)
{
    test_status_leds_setup();
    
    LOG_INF("Testing state transitions");
    
    // Test transition from heartbeat to error and back
    led_api->set_heartbeat(status_led_dev);
    k_sleep(K_MSEC(200));
    
    int ret = led_api->set_error_blink(status_led_dev, 2);
    zassert_equal(ret, 0, "Transitioning to error blink should succeed");
    k_sleep(K_MSEC(300));
    
    led_api->remove_error(status_led_dev);
    k_sleep(K_MSEC(200));
    
    led_api->set_heartbeat_always_on(status_led_dev);
    k_sleep(K_MSEC(200));
    
    LOG_INF("State transitions test completed");
}

ZTEST_SUITE(status_leds_tests, NULL, NULL, NULL, NULL, NULL);

// Main function for manual testing when not using ZTest framework
int main()
{
    LOG_INF("Status LEDs driver test starting");
    
    if (IS_ENABLED(CONFIG_ZTEST)) {
        return 0; // ZTest will handle execution
    }
    
    // Manual test execution (fallback)
    test_status_leds_setup();
    
    LOG_INF("Running manual tests...");
    
    // Test heartbeat
    led_api->set_heartbeat(status_led_dev);
    k_sleep(K_SECONDS(2));
    
    // Test error blink
    led_api->set_error_blink(status_led_dev, 3);
    k_sleep(K_SECONDS(3));
    
    // Test error always on
    led_api->set_error_always_on(status_led_dev);
    k_sleep(K_SECONDS(2));
    
    // Remove error and back to heartbeat
    led_api->remove_error(status_led_dev);
    led_api->set_heartbeat_always_on(status_led_dev);
    k_sleep(K_SECONDS(2));
    
    LOG_INF("Manual tests completed");
    return 0;
}
#include <OF/drivers/output/status_leds.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <OF/lib/CommBridge/Manager.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    const device* uart_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));

    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("LED not ready\n");
        return -1;
    }

    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);
    led_api->set_heartbeat(status_led_dev);

    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("USB CDC ACM not ready\n");
        return -1;
    }


    LOG_INF("Creating CommBridge Manager...");

    // 创建统一的 Manager
    // 第一个模板参数是发送的数据包类型，第二个是接收的数据包类型
    OF::CommBridge::Manager<std::tuple<SampleA>, std::tuple<SampleB>> manager(uart_dev);


    LOG_INF("Start receiving");
    manager.start_receive();


    SampleA packet_send{};

    constexpr double idx = 1.0;

    LOG_INF("Enter main loop...");

    while (true)
    {
        SampleB packet_received{};
        auto& [a, b, c, d] = packet_send;
        a += idx;
        b += idx;
        c += static_cast<float>(idx / 10.0);
        d += idx / 100.0;
        // 通过 Manager 发送
        manager.send(packet_send);

        // 通过 Manager 获取接收到的数据
        // packet_received = manager.get<SampleB>();

        LOG_INF("Sending Sample A: a=%d, b=%d, c=%.2f, d=%.2lf", a, b, c, d);
        // LOG_INF("Receiving Sample B: x=%d, y=%.2lf", packet_received.x, packet_received.y);

        k_sleep(K_MSEC(1000));
    }
    return 0;
}

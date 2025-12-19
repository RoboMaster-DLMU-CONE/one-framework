#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <OF/lib/CommBridge/CommBridge.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{
    const device* uart_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    if (!device_is_ready(uart_dev))
    {
        LOG_ERR("USB CDC ACM not ready\n");
        return -1;
    }


    LOG_INF("Creating CommBridge...");

    // 使用工厂函数创建 CommBridge 对象
    const auto bridge = OF::CommBridge<std::tuple<SampleA>, std::tuple<SampleB>>::create(uart_dev);


    LOG_INF("Start receiving");
    bridge->start_receive();


    SampleA packet_send{};

    LOG_INF("Enter main loop...");

    while (true)
    {
        constexpr double idx = 1.0;
        SampleB packet_received{};
        auto& [a, b, c, d] = packet_send;
        a += idx;
        b += idx;
        c += static_cast<float>(idx / 10.0);
        d += idx / 100.0;
        // 通过 CommBridge 发送
        bridge->send(packet_send);

        // 通过 CommBridge 获取接收到的数据
        packet_received = bridge->get<SampleB>();

        LOG_INF("Sending Sample A: a=%d, b=%d, c=%.2f, d=%.2lf", a, b, c, d);
        LOG_INF("Receiving Sample B: x=%d, y=%.2lf", packet_received.x, packet_received.y);

        k_sleep(K_MSEC(1000));
    }
    return 0;
}

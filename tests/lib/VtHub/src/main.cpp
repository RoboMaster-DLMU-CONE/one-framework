#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <OF/lib/VtHub/VtHub.hpp>

// Include packet definitions to print them if needed
#include <RPL/Packets/RoboMaster/RemoteControl.hpp>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

using namespace OF;

int main(void)
{
    LOG_INF("VtHub Test Started");

    // VtHub initializes via SYS_INIT

    while (true) {
        k_sleep(K_MSEC(100));

        // Try to get RemoteControl packet
        auto res = VtHub::get<RemoteControl>();
        
        if (res) {
            auto rc = res.value();
            LOG_INF("Received RemoteControl: Data received");
            // Since we don't know the exact fields of RemoteControl, we just log success.
            // If we knew, we would print.
        } else {
            // LOG_WRN("Failed to get RC: %s", res.error().message);
        }
    }
    return 0;
}

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <OF/lib/VtHub/VtHub.hpp>
#include "RPL/Packets/VT03RemotePacket.hpp"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

using namespace OF;

int main(void)
{
    LOG_INF("VtHub Test Started");

    while (true)
    {
        k_sleep(K_MSEC(100));

        auto res = VtHub::get<VT03RemotePacket>();

        if (res)
        {
            auto& rc = res.value();
            LOG_INF("VT03: LX=%u LY=%u RX=%u RY=%u W=%u SW=%u", 
                    (unsigned)rc.left_stick_x, (unsigned)rc.left_stick_y, 
                    (unsigned)rc.right_stick_x, (unsigned)rc.right_stick_y,
                    (unsigned)rc.wheel, (unsigned)rc.switch_state);
        }
    }
    return 0;
}
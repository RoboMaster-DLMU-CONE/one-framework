#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>

#include "OF/drivers/output/buzzer.h"


LOG_MODULE_REGISTER(node_test, CONFIG_LOG_DEFAULT_LEVEL);

using namespace OF;


int main()
{
    LOG_INF("main");

    while (true)
    {
        uint32_t load = cpu_load_get(false);
        LOG_INF("cpu: %u.%u%%", load / 10, load % 10);
        k_sleep(K_MSEC(500));
    }
}

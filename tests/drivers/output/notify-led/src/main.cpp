#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main()
{

    while (true)
    {
        k_sleep(K_MSEC(1000));
    }
}

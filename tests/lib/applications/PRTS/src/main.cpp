#include "zephyr/kernel.h"

[[noreturn]] int main()
{
    while (true)
    {
        k_sleep(K_SECONDS(1U));
    }
    return 0;
}

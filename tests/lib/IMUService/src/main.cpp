// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/IMUCenter/IMUCenter.hpp>
#include <zephyr/logging/log.h>
#include <autoconf.h>
#include <zephyr/kernel.h>


LOG_MODULE_REGISTER(imu_test, CONFIG_LOG_DEFAULT_LEVEL);


int main()
{
    while (true)
    {
        k_sleep(K_MSEC(500));
    }
}

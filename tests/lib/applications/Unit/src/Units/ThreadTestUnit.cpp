// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include "ThreadTestUnit.hpp"

void ThreadTestUnit::run()
{
    counter++;
    pid.compute(100, counter);
    k_sleep(K_MSEC(50));
}

REGISTER_UNIT(ThreadTestUnit)

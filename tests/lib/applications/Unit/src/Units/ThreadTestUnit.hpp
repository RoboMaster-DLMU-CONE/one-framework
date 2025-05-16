// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef THREADTESTUNIT_HPP
#define THREADTESTUNIT_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>

#include "OF/lib/utils/PID/PID.hpp"
using namespace OF;

class ThreadTestUnit final :
    public

Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(ThreadTestUnit, "ThreadTestUnit", "Unit for thread testing", 2048, 3)

    void run() override;

    PIDController<Positional, int> pid{1, 1, 1};
    int counter = 0;
};

#endif

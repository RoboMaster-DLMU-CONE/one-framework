// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef THREADTESTUNIT_HPP
#define THREADTESTUNIT_HPP

#include <OF/lib/Unit/Unit.hpp>
#include <OF/lib/Unit/UnitRegistry.hpp>

#include <OneMotor/Control/PID.hpp>
using OneMotor::Control::PIDController;
using OneMotor::Control::PID_Params;
using OneMotor::Control::Positional;
using namespace OF;

class ThreadTestUnit final :
    public

    Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(ThreadTestUnit, "ThreadTestUnit", "Unit for thread testing", 2048, 3)

    void run() override;

    PIDController<Positional, int> pid{{1, 1, 1}};
    int counter = 0;
};

#endif


// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TESTUNIT2_HPP
#define TESTUNIT2_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
using namespace OF;

class TestUnit2 final : public Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(TestUnit2, "TestUnit2", "Unit2 for testing", 4096, 10)
    void init() override {}
    void run() override {}
};


#endif // TESTUNIT2_HPP

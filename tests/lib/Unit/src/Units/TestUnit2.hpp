
// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TESTUNIT2_HPP
#define TESTUNIT2_HPP

#include <OF/lib/Unit/Unit.hpp>
#include <OF/lib/Unit/UnitRegistry.hpp>
using namespace OF;

class TestUnit2 final : public Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(TestUnit2, "TestUnit2", "Unit2 for testing", 4096, 10)
    void run() override { k_sleep(K_SECONDS(1)); }
};


#endif // TESTUNIT2_HPP

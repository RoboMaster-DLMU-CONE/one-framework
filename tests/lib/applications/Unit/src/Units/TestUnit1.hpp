// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TESTUNIT1_HPP
#define TESTUNIT1_HPP

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
using namespace OF;

// 测试Unit实现1
class TestUnit1 final : public Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(TestUnit1, "TestUnit1", "First test unit", 2048, 5)
    void init() override {}
    void run() override {}

    bool initialized = false;
    bool runCalled = false;
    bool cleanupCalled = false;
};

#endif // TESTUNIT1_HPP

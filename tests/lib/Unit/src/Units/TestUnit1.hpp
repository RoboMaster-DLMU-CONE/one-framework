// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TESTUNIT1_HPP
#define TESTUNIT1_HPP

#include <OF/lib/Unit/Unit.hpp>
#include <OF/lib/Unit/UnitRegistry.hpp>
using namespace OF;

// 测试Unit实现1
class TestUnit1 final : public Unit
{
public:
    DEFINE_UNIT_DESCRIPTOR(TestUnit1, "TestUnit1", "First test unit", 1024, 5)
    void run() override { runCalled = true; }

    bool initialized = false;
    bool runCalled = false;
    bool cleanupCalled = false;

protected:
    void initCustom() override { initialized = true; }
    void cleanupCustom() override { cleanupCalled = true; }
};

#endif // TESTUNIT1_HPP

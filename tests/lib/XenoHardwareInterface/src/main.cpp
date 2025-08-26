// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/ztest.h>
#include <OF/lib/XenoHardwareInterface/XenoHardwareInterface.hpp>

ZTEST_SUITE(XenoHardwareInterface, nullptr, nullptr, nullptr, nullptr, nullptr);

ZTEST(XenoHardwareInterface, test_header_inclusion)
{
    // This test primarily ensures that the headers compile correctly
    // and the namespace is properly defined
    
    // Basic compilation test - if this compiles, the headers are working
    zassert_true(true, "XenoHardwareInterface headers compile successfully");
}

// Note: Full hardware interface testing would require proper CAN hardware setup
// For now, we focus on basic compilation tests
// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/ztest.h>
#include <OF/lib/XenoControl/Joints.hpp>

ZTEST_SUITE(XenoControl, nullptr, nullptr, nullptr, nullptr, nullptr);

ZTEST(XenoControl, test_joint_enum_values)
{
    // Test that joint enum values are correctly mapped
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::LIFT), 0);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::STRETCH), 1);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::SHIFT), 2);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::ARM1), 3);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::ARM2), 4);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::ARM3), 5);
    zassert_equal(static_cast<uint8_t>(OF::XenoControl::JointType::SUCK), 6);
}

ZTEST(XenoControl, test_joint_data_initialization)
{
    // Test JointData structure initialization
    OF::XenoControl::JointData data{};
    
    zassert_equal(data.velocity_reading, 0.0f);
    zassert_equal(data.velocity_calibration, 1.0f);
    zassert_equal(data.position_reading, 0.0f);
    zassert_equal(data.position_calibration, 0.0f);
}

ZTEST(XenoControl, test_constants)
{
    // Test that basic constants are properly defined
    zassert_equal(OF::XenoControl::Joints::JOINT_COUNT, 7);
}
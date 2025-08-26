// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/XenoHardwareInterface/XenoHardwareInterface.hpp>

namespace OF::XenoHardwareInterface
{
    XenoHardwareInterface::XenoHardwareInterface(OneMotor::Can::CanDriver& can_driver)
        : can_driver_(can_driver)
    {
        joints_ = std::make_unique<XenoControl::Joints>(can_driver_);
    }

    bool XenoHardwareInterface::initialize()
    {
        if (initialized_) {
            return true;
        }

        // 这里可以添加初始化检查逻辑
        // 例如：检查CAN总线连接，验证电机响应等

        initialized_ = true;
        return true;
    }

    void XenoHardwareInterface::enableAllJoints()
    {
        joints_->enableJoint(XenoControl::JointType::LIFT);
        joints_->enableJoint(XenoControl::JointType::STRETCH);
        joints_->enableJoint(XenoControl::JointType::SHIFT);
        joints_->enableJoint(XenoControl::JointType::ARM1);
        joints_->enableJoint(XenoControl::JointType::ARM2);
        joints_->enableJoint(XenoControl::JointType::ARM3);
        joints_->enableJoint(XenoControl::JointType::SUCK);
    }

    void XenoHardwareInterface::disableAllJoints()
    {
        joints_->disableJoint(XenoControl::JointType::LIFT);
        joints_->disableJoint(XenoControl::JointType::STRETCH);
        joints_->disableJoint(XenoControl::JointType::SHIFT);
        joints_->disableJoint(XenoControl::JointType::ARM1);
        joints_->disableJoint(XenoControl::JointType::ARM2);
        joints_->disableJoint(XenoControl::JointType::ARM3);
        joints_->disableJoint(XenoControl::JointType::SUCK);
    }

    void XenoHardwareInterface::setLiftPosition(float position)
    {
        joints_->setTargetPosition(XenoControl::JointType::LIFT, position);
    }

    void XenoHardwareInterface::setStretchPosition(float position)
    {
        joints_->setTargetPosition(XenoControl::JointType::STRETCH, position);
    }

    void XenoHardwareInterface::setShiftPosition(float position)
    {
        joints_->setTargetPosition(XenoControl::JointType::SHIFT, position);
    }

    void XenoHardwareInterface::setArmPositions(float arm1_pos, float arm2_pos, float arm3_pos)
    {
        joints_->setTargetPosition(XenoControl::JointType::ARM1, arm1_pos);
        joints_->setTargetPosition(XenoControl::JointType::ARM2, arm2_pos);
        joints_->setTargetPosition(XenoControl::JointType::ARM3, arm3_pos);
    }

    void XenoHardwareInterface::setSuckPosition(float position)
    {
        joints_->setTargetPosition(XenoControl::JointType::SUCK, position);
    }

    void XenoHardwareInterface::updateSensorReadings()
    {
        joints_->updateReadings();
    }

    float XenoHardwareInterface::getLiftPosition() const
    {
        return joints_->getPositionReading(XenoControl::JointType::LIFT);
    }

    float XenoHardwareInterface::getStretchPosition() const
    {
        return joints_->getPositionReading(XenoControl::JointType::STRETCH);
    }

    float XenoHardwareInterface::getShiftPosition() const
    {
        return joints_->getPositionReading(XenoControl::JointType::SHIFT);
    }

    void XenoHardwareInterface::getArmPositions(float& arm1_pos, float& arm2_pos, float& arm3_pos) const
    {
        arm1_pos = joints_->getPositionReading(XenoControl::JointType::ARM1);
        arm2_pos = joints_->getPositionReading(XenoControl::JointType::ARM2);
        arm3_pos = joints_->getPositionReading(XenoControl::JointType::ARM3);
    }

    float XenoHardwareInterface::getSuckPosition() const
    {
        return joints_->getPositionReading(XenoControl::JointType::SUCK);
    }

} // namespace OF::XenoHardwareInterface
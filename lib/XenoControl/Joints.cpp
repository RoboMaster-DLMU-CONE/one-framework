// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/XenoControl/Joints.hpp>
#include <OneMotor/Control/PID.hpp>

namespace OF::XenoControl
{
    Joints::Joints(OneMotor::Can::CanDriver& can_driver)
        : can_driver_(can_driver)
    {
        // 默认PID参数，实际使用时应根据具体电机调整
        const OneMotor::Control::PID_Params<float> default_pos_params{
            .kp = 10.0f,
            .ki = 0.1f,
            .kd = 1.0f,
            .max_out = 16384.0f,
            .max_iout = 5000.0f
        };

        const OneMotor::Control::PID_Params<float> default_ang_params{
            .kp = 5.0f,
            .ki = 0.05f,
            .kd = 0.5f,
            .max_out = 16384.0f,
            .max_iout = 3000.0f
        };

        // 初始化7个电机，每个使用不同的CAN ID
        motor1_ = std::make_unique<OneMotor::Motor::DJI::M3508<1, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor2_ = std::make_unique<OneMotor::Motor::DJI::M3508<2, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor3_ = std::make_unique<OneMotor::Motor::DJI::M3508<3, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor4_ = std::make_unique<OneMotor::Motor::DJI::M3508<4, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor5_ = std::make_unique<OneMotor::Motor::DJI::M3508<5, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor6_ = std::make_unique<OneMotor::Motor::DJI::M3508<6, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);
        motor7_ = std::make_unique<OneMotor::Motor::DJI::M3508<7, OneMotor::Motor::DJI::MotorMode::Position>>(
            can_driver_, default_pos_params, default_ang_params);

        // 初始化关节数据
        for (auto& data : joint_data_) {
            data = JointData{};
        }
    }

    float Joints::getVelocityReading(JointType joint) const
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return 0.0f;
        }

        data_lock_.lock();
        float result = joint_data_[index].velocity_reading;
        data_lock_.unlock();
        return result;
    }

    float Joints::getVelocityCalibration(JointType joint) const
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return 1.0f;
        }

        data_lock_.lock();
        float result = joint_data_[index].velocity_calibration;
        data_lock_.unlock();
        return result;
    }

    float Joints::getPositionReading(JointType joint) const
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return 0.0f;
        }

        data_lock_.lock();
        float result = joint_data_[index].position_reading;
        data_lock_.unlock();
        return result;
    }

    float Joints::getPositionCalibration(JointType joint) const
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return 0.0f;
        }

        data_lock_.lock();
        float result = joint_data_[index].position_calibration;
        data_lock_.unlock();
        return result;
    }

    void Joints::setVelocityCalibration(JointType joint, float calibration)
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return;
        }

        data_lock_.lock();
        joint_data_[index].velocity_calibration = calibration;
        data_lock_.unlock();
    }

    void Joints::setPositionCalibration(JointType joint, float calibration)
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return;
        }

        data_lock_.lock();
        joint_data_[index].position_calibration = calibration;
        data_lock_.unlock();
    }

    void Joints::setTargetVelocity(JointType joint, float velocity)
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return;
        }

        // 应用速度标定值
        float calibrated_velocity;
        data_lock_.lock();
        calibrated_velocity = velocity * joint_data_[index].velocity_calibration;
        data_lock_.unlock();

        // 根据关节类型设置对应电机的速度
        switch (joint) {
            case JointType::LIFT:
                motor1_->setAngRef(calibrated_velocity);
                break;
            case JointType::STRETCH:
                motor2_->setAngRef(calibrated_velocity);
                break;
            case JointType::SHIFT:
                motor3_->setAngRef(calibrated_velocity);
                break;
            case JointType::ARM1:
                motor4_->setAngRef(calibrated_velocity);
                break;
            case JointType::ARM2:
                motor5_->setAngRef(calibrated_velocity);
                break;
            case JointType::ARM3:
                motor6_->setAngRef(calibrated_velocity);
                break;
            case JointType::SUCK:
                motor7_->setAngRef(calibrated_velocity);
                break;
        }
    }

    void Joints::setTargetPosition(JointType joint, float position)
    {
        const auto index = getJointIndex(joint);
        if (!isValidJointIndex(index)) {
            return;
        }

        // 应用位置标定值
        float calibrated_position;
        data_lock_.lock();
        calibrated_position = position + joint_data_[index].position_calibration;
        data_lock_.unlock();

        // 根据关节类型设置对应电机的位置
        switch (joint) {
            case JointType::LIFT:
                motor1_->setPosRef(calibrated_position);
                break;
            case JointType::STRETCH:
                motor2_->setPosRef(calibrated_position);
                break;
            case JointType::SHIFT:
                motor3_->setPosRef(calibrated_position);
                break;
            case JointType::ARM1:
                motor4_->setPosRef(calibrated_position);
                break;
            case JointType::ARM2:
                motor5_->setPosRef(calibrated_position);
                break;
            case JointType::ARM3:
                motor6_->setPosRef(calibrated_position);
                break;
            case JointType::SUCK:
                motor7_->setPosRef(calibrated_position);
                break;
        }
    }

    void Joints::enableJoint(JointType joint)
    {
        // 根据关节类型启用对应电机
        switch (joint) {
            case JointType::LIFT:
                motor1_->enable();
                break;
            case JointType::STRETCH:
                motor2_->enable();
                break;
            case JointType::SHIFT:
                motor3_->enable();
                break;
            case JointType::ARM1:
                motor4_->enable();
                break;
            case JointType::ARM2:
                motor5_->enable();
                break;
            case JointType::ARM3:
                motor6_->enable();
                break;
            case JointType::SUCK:
                motor7_->enable();
                break;
        }
    }

    void Joints::disableJoint(JointType joint)
    {
        // 根据关节类型禁用对应电机
        switch (joint) {
            case JointType::LIFT:
                motor1_->disable();
                break;
            case JointType::STRETCH:
                motor2_->disable();
                break;
            case JointType::SHIFT:
                motor3_->disable();
                break;
            case JointType::ARM1:
                motor4_->disable();
                break;
            case JointType::ARM2:
                motor5_->disable();
                break;
            case JointType::ARM3:
                motor6_->disable();
                break;
            case JointType::SUCK:
                motor7_->disable();
                break;
        }
    }

    void Joints::updateReadings()
    {
        // 从各个电机获取状态并更新读取值
        auto update_joint_data = [this](size_t index, auto& motor) {
            auto status = motor->getStatus();
            data_lock_.lock();
            joint_data_[index].velocity_reading = status.angular;
            joint_data_[index].position_reading = status.total_angle;
            data_lock_.unlock();
        };

        update_joint_data(0, motor1_);  // LIFT
        update_joint_data(1, motor2_);  // STRETCH
        update_joint_data(2, motor3_);  // SHIFT
        update_joint_data(3, motor4_);  // ARM1
        update_joint_data(4, motor5_);  // ARM2
        update_joint_data(5, motor6_);  // ARM3
        update_joint_data(6, motor7_);  // SUCK
    }

} // namespace OF::XenoControl
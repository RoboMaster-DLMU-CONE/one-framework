// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef OF_XENO_CONTROL_JOINTS_HPP
#define OF_XENO_CONTROL_JOINTS_HPP

#include <OneMotor/Motor/DJI/M3508.hpp>
#include <OneMotor/Util/SpinLock.hpp>
#include <array>
#include <memory>

namespace OF::XenoControl
{
    /**
     * @brief 关节枚举，定义机械臂的7个关节
     */
    enum class JointType : uint8_t
    {
        LIFT = 0,    ///< 升降关节
        STRETCH = 1, ///< 伸缩关节
        SHIFT = 2,   ///< 平移关节
        ARM1 = 3,    ///< 机械臂关节1
        ARM2 = 4,    ///< 机械臂关节2
        ARM3 = 5,    ///< 机械臂关节3
        SUCK = 6     ///< 吸盘关节
    };

    /**
     * @brief 关节数据结构，存储关节的读取值和标定值
     */
    struct JointData
    {
        float velocity_reading = 0.0f;      ///< 速度读取值 (度/秒)
        float velocity_calibration = 1.0f;  ///< 速度标定值 (倍数)
        float position_reading = 0.0f;      ///< 位置读取值 (度)
        float position_calibration = 0.0f;  ///< 位置标定值 (度偏移)
    };

    /**
     * @brief 关节控制类，管理机械臂的7个关节
     * 
     * @details 该类提供了对机械臂7个关节的统一控制接口，
     *          支持读取和设置速度/位置的读取值和标定值。
     *          使用OneMotor的M3508电机进行实际控制。
     */
    class Joints
    {
    public:
        static constexpr size_t JOINT_COUNT = 7;

        /**
         * @brief 构造函数
         * @param can_driver CAN总线驱动引用
         */
        explicit Joints(OneMotor::Can::CanDriver& can_driver);

        /**
         * @brief 析构函数
         */
        ~Joints() = default;

        // 禁用拷贝和移动
        Joints(const Joints&) = delete;
        Joints& operator=(const Joints&) = delete;
        Joints(Joints&&) = delete;
        Joints& operator=(Joints&&) = delete;

        /**
         * @brief 获取指定关节的速度读取值
         * @param joint 关节类型
         * @return 速度读取值 (度/秒)
         */
        float getVelocityReading(JointType joint) const;

        /**
         * @brief 获取指定关节的速度标定值
         * @param joint 关节类型
         * @return 速度标定值 (倍数)
         */
        float getVelocityCalibration(JointType joint) const;

        /**
         * @brief 获取指定关节的位置读取值
         * @param joint 关节类型
         * @return 位置读取值 (度)
         */
        float getPositionReading(JointType joint) const;

        /**
         * @brief 获取指定关节的位置标定值
         * @param joint 关节类型
         * @return 位置标定值 (度偏移)
         */
        float getPositionCalibration(JointType joint) const;

        /**
         * @brief 设置指定关节的速度标定值
         * @param joint 关节类型
         * @param calibration 速度标定值 (倍数)
         */
        void setVelocityCalibration(JointType joint, float calibration);

        /**
         * @brief 设置指定关节的位置标定值
         * @param joint 关节类型
         * @param calibration 位置标定值 (度偏移)
         */
        void setPositionCalibration(JointType joint, float calibration);

        /**
         * @brief 设置指定关节的目标速度
         * @param joint 关节类型
         * @param velocity 目标速度 (度/秒)
         */
        void setTargetVelocity(JointType joint, float velocity);

        /**
         * @brief 设置指定关节的目标位置
         * @param joint 关节类型
         * @param position 目标位置 (度)
         */
        void setTargetPosition(JointType joint, float position);

        /**
         * @brief 启用指定关节
         * @param joint 关节类型
         */
        void enableJoint(JointType joint);

        /**
         * @brief 禁用指定关节
         * @param joint 关节类型
         */
        void disableJoint(JointType joint);

        /**
         * @brief 更新所有关节的读取值
         * @details 从电机状态更新关节数据，应定期调用
         */
        void updateReadings();

    private:
        /**
         * @brief 获取关节索引
         * @param joint 关节类型
         * @return 关节索引
         */
        static constexpr size_t getJointIndex(JointType joint)
        {
            return static_cast<size_t>(joint);
        }

        /**
         * @brief 验证关节索引有效性
         * @param index 关节索引
         * @return 是否有效
         */
        static constexpr bool isValidJointIndex(size_t index)
        {
            return index < JOINT_COUNT;
        }

        OneMotor::Can::CanDriver& can_driver_;  ///< CAN总线驱动引用
        
        // 使用位置模式的M3508电机，支持位置和速度控制
        // 每个关节使用不同的电机ID (1-7)
        std::unique_ptr<OneMotor::Motor::DJI::M3508<1, OneMotor::Motor::DJI::MotorMode::Position>> motor1_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<2, OneMotor::Motor::DJI::MotorMode::Position>> motor2_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<3, OneMotor::Motor::DJI::MotorMode::Position>> motor3_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<4, OneMotor::Motor::DJI::MotorMode::Position>> motor4_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<5, OneMotor::Motor::DJI::MotorMode::Position>> motor5_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<6, OneMotor::Motor::DJI::MotorMode::Position>> motor6_;
        std::unique_ptr<OneMotor::Motor::DJI::M3508<7, OneMotor::Motor::DJI::MotorMode::Position>> motor7_;
        
        // 关节数据，使用互斥锁保证线程安全
        mutable std::array<JointData, JOINT_COUNT> joint_data_;
        mutable OneMotor::Util::SpinLock data_lock_;
    };

} // namespace OF::XenoControl

#endif // OF_XENO_CONTROL_JOINTS_HPP
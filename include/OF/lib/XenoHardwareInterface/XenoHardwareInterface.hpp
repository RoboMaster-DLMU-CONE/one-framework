// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef OF_XENO_HARDWARE_INTERFACE_HPP
#define OF_XENO_HARDWARE_INTERFACE_HPP

#include <OF/lib/XenoControl/Joints.hpp>
#include <OneMotor/Can/CanDriver.hpp>
#include <memory>

namespace OF::XenoHardwareInterface
{
    /**
     * @brief 机械臂硬件接口类
     * 
     * @details 该类提供了机械臂的高级控制接口，
     *          封装了底层的关节控制，提供统一的硬件操作接口。
     */
    class XenoHardwareInterface
    {
    public:
        /**
         * @brief 构造函数
         * @param can_driver CAN总线驱动引用
         */
        explicit XenoHardwareInterface(OneMotor::Can::CanDriver& can_driver);

        /**
         * @brief 析构函数
         */
        ~XenoHardwareInterface() = default;

        // 禁用拷贝和移动
        XenoHardwareInterface(const XenoHardwareInterface&) = delete;
        XenoHardwareInterface& operator=(const XenoHardwareInterface&) = delete;
        XenoHardwareInterface(XenoHardwareInterface&&) = delete;
        XenoHardwareInterface& operator=(XenoHardwareInterface&&) = delete;

        /**
         * @brief 初始化硬件接口
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 获取关节控制器引用
         * @return 关节控制器引用
         */
        XenoControl::Joints& getJoints() { return *joints_; }

        /**
         * @brief 获取关节控制器常量引用
         * @return 关节控制器常量引用
         */
        const XenoControl::Joints& getJoints() const { return *joints_; }

        /**
         * @brief 启用所有关节
         */
        void enableAllJoints();

        /**
         * @brief 禁用所有关节
         */
        void disableAllJoints();

        /**
         * @brief 设置升降关节目标位置
         * @param position 目标位置 (度)
         */
        void setLiftPosition(float position);

        /**
         * @brief 设置伸缩关节目标位置
         * @param position 目标位置 (度)
         */
        void setStretchPosition(float position);

        /**
         * @brief 设置平移关节目标位置
         * @param position 目标位置 (度)
         */
        void setShiftPosition(float position);

        /**
         * @brief 设置机械臂关节目标位置
         * @param arm1_pos Arm1关节位置 (度)
         * @param arm2_pos Arm2关节位置 (度)
         * @param arm3_pos Arm3关节位置 (度)
         */
        void setArmPositions(float arm1_pos, float arm2_pos, float arm3_pos);

        /**
         * @brief 设置吸盘关节目标位置
         * @param position 目标位置 (度)
         */
        void setSuckPosition(float position);

        /**
         * @brief 更新所有传感器读数
         * @details 应定期调用以更新关节状态
         */
        void updateSensorReadings();

        /**
         * @brief 获取升降关节当前位置
         * @return 当前位置 (度)
         */
        float getLiftPosition() const;

        /**
         * @brief 获取伸缩关节当前位置
         * @return 当前位置 (度)
         */
        float getStretchPosition() const;

        /**
         * @brief 获取平移关节当前位置
         * @return 当前位置 (度)
         */
        float getShiftPosition() const;

        /**
         * @brief 获取机械臂关节当前位置
         * @param arm1_pos Arm1关节位置输出 (度)
         * @param arm2_pos Arm2关节位置输出 (度)
         * @param arm3_pos Arm3关节位置输出 (度)
         */
        void getArmPositions(float& arm1_pos, float& arm2_pos, float& arm3_pos) const;

        /**
         * @brief 获取吸盘关节当前位置
         * @return 当前位置 (度)
         */
        float getSuckPosition() const;

    private:
        OneMotor::Can::CanDriver& can_driver_;  ///< CAN总线驱动引用
        std::unique_ptr<XenoControl::Joints> joints_;  ///< 关节控制器
        bool initialized_ = false;  ///< 初始化状态
    };

} // namespace OF::XenoHardwareInterface

#endif // OF_XENO_HARDWARE_INTERFACE_HPP
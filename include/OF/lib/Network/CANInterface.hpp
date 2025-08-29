// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CANINTERFACE_HPP
#define CANINTERFACE_HPP

#include <OF/lib/Network/Interface.hpp>
#include <stdexcept>

namespace OF::Network
{
    class Daemon;

    /**
     * @brief CAN 接口类
     *
     * 表示物理 CAN 接口，在构造时验证接口存在性。
     * 如果指定的接口不存在，构造函数将抛出异常。
     */
    class CANInterface : public Interface
    {
    public:
        /**
         * @brief 构造函数
         * @param name 接口名称
         * @param daemon 网络守护进程引用
         * @throws std::runtime_error 如果接口不存在
         */
        CANInterface(std::string_view name, Daemon& daemon);

        /**
         * @brief 析构函数
         */
        ~CANInterface() override = default;

        /**
         * @brief 检查接口是否已启用
         * @return true 如果接口已启用，false 否则
         */
        bool is_up() const override;

        /**
         * @brief 启用接口
         * @return true 如果启用成功，false 否则
         */
        bool bring_up();

        /**
         * @brief 禁用接口
         * @return true 如果禁用成功，false 否则
         */
        bool bring_down();

    private:
        Daemon& daemon_; //!< 网络守护进程引用
    };

    /**
     * @brief CAN 接口不存在异常
     */
    class CANInterfaceNotFoundException : public std::runtime_error
    {
    public:
        explicit CANInterfaceNotFoundException(std::string_view interfaceName)
            : std::runtime_error("CAN interface not found: " + std::string(interfaceName))
        {
        }
    };

} // namespace OF::Network

#endif // CANINTERFACE_HPP
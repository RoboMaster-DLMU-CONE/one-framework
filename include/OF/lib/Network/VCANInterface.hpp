// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef VCANINTERFACE_HPP
#define VCANINTERFACE_HPP

#include <OF/lib/Network/Interface.hpp>

namespace OF::Network
{
    class Daemon;

    /**
     * @brief VCAN (虚拟 CAN) 接口类
     *
     * 表示虚拟 CAN 接口，在构造时自动通过 Daemon 创建新的 VCAN 接口。
     * 用于测试和开发环境，无需物理 CAN 硬件。
     */
    class VCANInterface : public Interface
    {
    public:
        /**
         * @brief 构造函数
         * @param name 接口名称
         * @param daemon 网络守护进程引用
         */
        VCANInterface(std::string_view name, Daemon& daemon);

        /**
         * @brief 析构函数
         * 自动删除创建的 VCAN 接口
         */
        ~VCANInterface() override;

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

        /**
         * @brief 检查接口是否由此实例创建
         * @return true 如果由此实例创建，false 否则
         */
        bool is_created_by_this_instance() const { return createdByThisInstance_; }

    private:
        Daemon& daemon_; //!< 网络守护进程引用
        bool createdByThisInstance_; //!< 标记接口是否由此实例创建
    };

} // namespace OF::Network

#endif // VCANINTERFACE_HPP
// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef DAEMON_HPP
#define DAEMON_HPP

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>

namespace OF::Network
{
    class Interface;
    class Netlink;

    /**
     * @brief 网络接口管理守护进程
     *
     * 提供网络接口的创建、管理和状态监控功能。
     * 支持 CAN 和 VCAN 接口的管理，并提供统一的接口解析和操作方法。
     */
    class Daemon
    {
    public:
        /**
         * @brief 构造函数
         */
        Daemon();

        /**
         * @brief 析构函数
         */
        ~Daemon();

        /**
         * @brief 初始化守护进程
         * @return true 如果初始化成功，false 否则
         */
        bool initialize();

        /**
         * @brief 关闭守护进程
         */
        void shutdown();

        /**
         * @brief 创建 VCAN 接口
         * @param interfaceName 接口名称
         * @return true 如果创建成功，false 否则
         */
        bool createVCANInterface(std::string_view interfaceName);

        /**
         * @brief 删除网络接口
         * @param interfaceName 接口名称
         * @return true 如果删除成功，false 否则
         */
        bool deleteInterface(std::string_view interfaceName);

        /**
         * @brief 检查接口是否存在
         * @param interfaceName 接口名称
         * @return true 如果接口存在，false 否则
         */
        bool interfaceExists(std::string_view interfaceName) const;

        /**
         * @brief 启用网络接口
         * @param interfaceName 接口名称
         * @return true 如果启用成功，false 否则
         */
        bool bringInterfaceUp(std::string_view interfaceName);

        /**
         * @brief 禁用网络接口
         * @param interfaceName 接口名称
         * @return true 如果禁用成功，false 否则
         */
        bool bringInterfaceDown(std::string_view interfaceName);

        /**
         * @brief 解析接口类型
         * @param interfaceName 接口名称
         * @return 接口类型字符串（"can", "vcan", "unknown"）
         */
        std::string parseInterfaceType(std::string_view interfaceName) const;

        /**
         * @brief 解析接口配置参数
         * @param interfaceName 接口名称
         * @param configParams 配置参数字符串
         * @return true 如果解析成功，false 否则
         */
        bool parseInterfaceConfig(std::string_view interfaceName, std::string_view configParams);

        /**
         * @brief 获取 Netlink 实例的引用
         * @return Netlink 实例的引用
         */
        Netlink& getNetlink() { return *netlink_; }
        const Netlink& getNetlink() const { return *netlink_; }

    private:
        std::unique_ptr<Netlink> netlink_; //!< Netlink 通信实例
        std::unordered_map<std::string, std::string> interfaceTypes_; //!< 接口类型映射
        bool initialized_; //!< 初始化状态

        /**
         * @brief 执行系统命令的内部方法
         * @param command 要执行的命令
         * @return true 如果命令执行成功，false 否则
         */
        bool executeCommand(std::string_view command) const;

        /**
         * @brief 验证接口名称的有效性
         * @param interfaceName 接口名称
         * @return true 如果名称有效，false 否则
         */
        bool validateInterfaceName(std::string_view interfaceName) const;
    };

} // namespace OF::Network

#endif // DAEMON_HPP
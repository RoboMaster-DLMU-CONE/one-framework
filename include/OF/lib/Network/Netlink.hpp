// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <string>
#include <string_view>

namespace OF::Network
{
    /**
     * @brief Netlink 通信类
     *
     * 提供与内核网络子系统的 Netlink 套接字通信功能，
     * 用于查询和管理网络接口状态。
     */
    class Netlink
    {
    public:
        /**
         * @brief 构造函数
         */
        Netlink();

        /**
         * @brief 析构函数
         */
        ~Netlink();

        /**
         * @brief 检查指定接口是否已启用
         * @param interfaceName 接口名称
         * @return true 如果接口已启用，false 否则
         */
        bool is_up(std::string_view interfaceName) const;

        /**
         * @brief 初始化 Netlink 套接字
         * @return true 如果初始化成功，false 否则
         */
        bool initialize();

        /**
         * @brief 关闭 Netlink 套接字
         */
        void close();

    private:
        int socketFd_; //!< Netlink 套接字文件描述符
        bool initialized_; //!< 初始化状态

        /**
         * @brief 查询接口状态的内部实现
         * @param interfaceName 接口名称
         * @return true 如果接口已启用，false 否则
         */
        bool queryInterfaceStatus(std::string_view interfaceName) const;
    };

} // namespace OF::Network

#endif // NETLINK_HPP
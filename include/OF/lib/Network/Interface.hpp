// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <string>
#include <string_view>

namespace OF::Network
{
    /**
     * @brief 网络接口基类
     *
     * 提供网络接口的基本抽象，包括接口名称管理和状态检查功能。
     * 所有具体的网络接口类型（如 CAN、VCAN）都应继承此类。
     */
    class Interface
    {
    public:
        /**
         * @brief 构造函数
         * @param name 接口名称
         */
        explicit Interface(std::string_view name);

        /**
         * @brief 虚析构函数
         */
        virtual ~Interface() = default;

        /**
         * @brief 检查接口是否已启用
         * @return true 如果接口已启用，false 否则
         */
        virtual bool is_up() const = 0;

        /**
         * @brief 获取接口名称
         * @return 接口名称
         */
        const std::string& getName() const { return name_; }

    protected:
        std::string name_; //!< 接口名称
    };

} // namespace OF::Network

#endif // INTERFACE_HPP
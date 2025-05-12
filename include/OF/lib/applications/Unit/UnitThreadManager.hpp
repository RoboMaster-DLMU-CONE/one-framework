// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UNITTHREADMANAGER_HPP
#define UNITTHREADMANAGER_HPP

#include <memory>
#include <vector>

#include "Unit.hpp"

namespace OF
{
    /**
     * @brief 单元线程管理器类
     *
     * @details 负责为Unit实例创建和初始化线程。此类提供了将Unit实例
     *          与Zephyr操作系统线程关联的功能，使每个Unit可以在独立的
     *          线程中执行其run方法。
     */
    class UnitThreadManager
    {
    public:
        /**
         * @brief 为所有单元初始化线程
         *
         * @details 为提供的每个Unit实例创建一个Zephyr线程，并根据Unit的配置
         *          设置线程的优先级和栈大小。此方法还会调用每个Unit的init方法，
         *          并将线程映射信息注册到UnitRegistry中。
         *
         * @param units 包含所有Unit实例的向量
         */
        static void initializeThreads(const std::vector<std::unique_ptr<Unit>>& units);

    private:
        /**
         * @brief 线程入口函数
         *
         * @details 此函数作为Zephyr线程的入口点，它会调用提供的Unit实例的run方法。
         *          当Unit的run方法返回时，线程将终止。
         *
         * @param unit 指向Unit实例的指针
         */
        static void threadEntryFunction(void* unit, void*, void*);
    };
} // namespace OF


#endif // UNITTHREADMANAGER_HPP

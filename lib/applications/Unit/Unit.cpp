// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/applications/Unit/Unit.hpp>
#include <OF/lib/applications/Unit/UnitRegistry.hpp>
#include <OF/lib/applications/Unit/UnitThreadManager.hpp>

namespace OF
{

    /**
     * @brief Unit 类的 cleanup 方法默认实现。
     * @details 默认情况下不执行任何操作。派生类可以重写此方法以实现自定义的清理逻辑。
     */
    void Unit::cleanup() {}

    /**
     * @brief Unit 类的析构函数默认实现。
     */
    Unit::~Unit() = default;

    /**
     * @brief 全局单元实例向量。
     * @details 存储所有通过 UnitRegistry 创建的单元实例的 unique_ptr。
     */
    static std::vector<std::unique_ptr<Unit>> g_units;

    /**
     * @brief 启动所有已注册的单元。
     * @details 此函数负责初始化单元注册表，创建所有已注册单元的实例，
     *          并将这些实例交给 UnitThreadManager 来初始化和启动它们各自的线程。
     *          此函数具有幂等性，多次调用只有第一次有效。
     */
    void StartUnits()
    {
        static bool initialized = false;
        if (initialized)
        {
            return;
        }

        UnitRegistry::initialize();
        g_units = std::move(UnitRegistry::createAllUnits());
        UnitThreadManager::initializeThreads(g_units);
        initialized = true;
    }


} // namespace OF

// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UNITREGISTRY_HPP
#define UNITREGISTRY_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "Unit.hpp"

#if defined(CONFIG_UNIT_THREAD_ANALYZER)
#include <zephyr/debug/thread_analyzer.h>
#endif

#define OF_CONCAT_INTERNAL(x, y) x##y
#define OF_CONCAT(x, y) OF_CONCAT_INTERNAL(x, y)

namespace OF
{
    /**
     * @brief 单元注册类
     *
     * @details UnitRegistry负责管理框架中所有Unit类型的注册、创建和状态追踪。
     *          它提供了一种机制，允许Unit类型在编译时注册，并在运行时动态创建实例。
     *          同时，它还跟踪运行中Unit的状态和资源使用情况。
     */
    class UnitRegistry
    {
    public:
        /**
         * @brief 单元注册函数类型
         * @details 用于在启动时注册Unit类型的函数指针类型
         */
        using UnitRegistrationFunction = void (*)();

        /**
         * @brief 单元工厂函数类型
         * @details 用于创建Unit实例的函数指针类型
         */
        using UnitFactoryFunction = std::unique_ptr<Unit> (*)();

        /**
         * @brief 注册单个单元类型
         *
         * @details 将指定的Unit类型添加到注册表中，包括它的元数据和创建方法。
         *          此方法通常通过REGISTER_UNIT宏自动调用。
         *
         * @tparam UnitType 满足UnitDeriveConcept的Unit派生类型
         */
        template <typename UnitType>
        static void registerUnit()
        {
            g_unitFactories.push_back([]() -> std::unique_ptr<Unit> { return std::make_unique<UnitType>(); });
        }

        /**
         * @brief 初始化所有注册的单元
         *
         * @details 清理注册表，并调用所有已注册的注册函数，以便注册所有Unit类型。
         *          此方法在系统启动时调用一次。
         */
        static std::unordered_map<std::string_view, std::unique_ptr<Unit>>& initialize();

        /**
         * @brief 通过名称查找单元信息
         *
         * @param name 要查找的单元名称
         * @return std::optional<Unit*> 找到时返回指向Unit的指针，否则返回空optional
         */
        static std::optional<Unit*> findUnit(std::string_view name);

        static void tryStartUnit(std::string_view name);

        static void tryStopUnit(std::string_view name);

        static void tryRestartUnit(std::string_view name);

        /**
         * @brief 更新所有单元的资源使用统计
         */
        static void updateAllUnitStats();

    private:
        static std::vector<UnitFactoryFunction> g_unitFactories; //!< 所有注册的单元工厂函数
        static std::vector<UnitRegistrationFunction> g_registrationFunctions; //!< 所有注册的注册函数
        static std::unordered_map<std::string_view, std::unique_ptr<Unit>> g_units;

#if defined(CONFIG_UNIT_THREAD_ANALYZER)
        /**
         * @brief 线程统计回调函数
         *
         * @details 此函数被Zephyr线程分析器调用，用于收集线程性能统计数据
         *
         * @param info 线程分析器提供的统计信息
         */
        static void threadStatCallback(thread_analyzer_info* info);
#endif
    };

    /**
     * @brief 定义单元类型ID
     *
     * @details 此宏用于在Unit派生类中定义类型ID和getTypeId方法
     *
     * @param TypeId 类型的唯一标识符
     */
#define DEFINE_UNIT_TYPE(TypeId)                                                                                       \
    static constexpr uint32_t TYPE_ID = TypeId;                                                                        \
    uint32_t getTypeId() const override { return TYPE_ID; }

    /**
     * @brief 计算类型名称的哈希值
     *
     * @details 用于生成基于类型名称的唯一哈希值
     *
     * @param str 类型名称字符串
     * @return uint32_t 生成的哈希值
     */
    constexpr uint32_t typeNameHash(const char* str)
    {
        uint32_t hash = 5381;
        for (size_t i = 0; str[i]; ++i)
        {
            hash = ((hash << 5) + hash) ^ str[i];
        }
        return hash;
    }

    /**
     * @brief 自动生成单元类型ID
     *
     * @details 此宏基于类型名称自动生成类型ID和getTypeId方法
     *
     * @param TypeName 类型名称
     */
#define AUTO_UNIT_TYPE(TypeName)                                                                                       \
    static constexpr uint32_t TYPE_ID = typeNameHash(#TypeName);                                                       \
    uint32_t getTypeId() const override { return TYPE_ID; }

    /**
     * @brief 注册单元类宏
     *
     * @details 此宏自动将Unit派生类注册到UnitRegistry
     *
     * @param UnitClass 要注册的Unit派生类名称
     */

#undef REGISTER_UNIT
#define REGISTER_UNIT(UnitClass) \
    extern "C" { \
        OF::UnitRegistry::UnitFactoryFunction \
        _unit_factory_##UnitClass __attribute__((section(".unit_registry"),used)) = \
        []() -> std::unique_ptr<OF::Unit> { return std::make_unique<UnitClass>(); }; \
    }

} // namespace OF

#endif // UNITREGISTRY_HPP

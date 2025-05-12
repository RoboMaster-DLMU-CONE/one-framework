// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UNITREGISTRY_HPP
#define UNITREGISTRY_HPP

#include <memory>
#include <optional>
#include <span>
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
     * @brief 单元特性结构体模板
     *
     * @details 此模板为每种Unit类型提供类型特征信息，包括创建实例的方法
     *          以及类型的元数据信息。用于UnitRegistry自动注册和管理不同类型的Unit。
     *
     * @tparam T 必须满足UnitDeriveConcept约束的Unit派生类型
     */
    template <UnitDeriveConcept T>
    struct UnitTraits
    {
        /**
         * @brief 单元信息静态常量，包含Unit类型的所有元数据
         */
        static constexpr UnitInfo info{.name = T::name(),
                                       .description = T::description(),
                                       .stackSize = T::stackSize(),
                                       .priority = T::priority(),
                                       .isRunning = false,
                                       .typeId = T::TYPE_ID,
                                       .stats = {}};

        /**
         * @brief 创建指定类型Unit实例的工厂方法
         * @return 返回指向新创建的Unit实例的unique_ptr
         */
        static std::unique_ptr<Unit> create() { return std::make_unique<T>(); }
    };

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
            g_unitInfos.push_back(UnitTraits<UnitType>::info);
            g_unitFactories.push_back(UnitTraits<UnitType>::create);
        }

        /**
         * @brief 添加注册函数到注册表
         *
         * @details 添加一个在初始化时将被调用的函数，用于注册Unit类型。
         *          此方法通常由REGISTER_UNIT宏自动调用。
         *
         * @param func 指向注册函数的指针
         */
        static void addRegistrationFunction(const UnitRegistrationFunction func);

        /**
         * @brief 初始化所有注册的单元
         *
         * @details 清理注册表，并调用所有已注册的注册函数，以便注册所有Unit类型。
         *          此方法在系统启动时调用一次。
         */
        static void initialize();

        /**
         * @brief 获取所有注册的单元信息
         *
         * @return std::span<const UnitInfo> 包含所有注册的Unit信息的只读视图
         */
        static std::span<const UnitInfo> getUnits();

        /**
         * @brief 创建所有注册单元的实例
         *
         * @return std::vector<std::unique_ptr<Unit>> 包含所有创建的Unit实例的向量
         */
        static std::vector<std::unique_ptr<Unit>> createAllUnits();

        /**
         * @brief 注册线程与单元索引的映射关系
         *
         * @param name 线程名称
         * @param unitIndex 单元在注册表中的索引
         */
        static void registerThreadMapping(std::string_view name, size_t unitIndex);

        /**
         * @brief 通过名称查找单元信息
         *
         * @param name 要查找的单元名称
         * @return std::optional<UnitInfo*> 找到时返回指向UnitInfo的指针，否则返回空optional
         */
        static std::optional<UnitInfo*> findUnit(std::string_view name);

        /**
         * @brief 更新单元运行状态
         *
         * @param idx 单元在注册表中的索引
         * @param running 单元是否正在运行
         */
        static void updateUnitStatus(size_t idx, bool running);

        /**
         * @brief 更新单元资源使用统计
         *
         * @param idx 单元在注册表中的索引
         * @param cpu CPU使用率
         * @param mem 内存使用量
         */
        static void updateUnitStats(size_t idx, uint32_t cpu, uint32_t mem);

        /**
         * @brief 更新所有单元的资源使用统计
         */
        static void updateAllUnitStats();

    private:
        static std::vector<UnitInfo> g_unitInfos; //!< 所有注册的单元信息
        static std::vector<UnitFactoryFunction> g_unitFactories; //!< 所有注册的单元工厂函数
        static std::vector<UnitRegistrationFunction> g_registrationFunctions; //!< 所有注册的注册函数
        static std::unordered_map<std::string_view, size_t> g_nameToUnitMap; //!< 单元名称到索引的映射

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
#define REGISTER_UNIT(UnitClass)                                                                                       \
    namespace                                                                                                          \
    {                                                                                                                  \
        void OF_CONCAT(registerUnit_, __LINE__)() { OF::UnitRegistry::registerUnit<UnitClass>(); }                     \
                                                                                                                       \
        struct OF_CONCAT(UnitRegistrar_, __LINE__)                                                                     \
        {                                                                                                              \
            OF_CONCAT(UnitRegistrar_, __LINE__)()                                                                      \
            {                                                                                                          \
                OF::UnitRegistry::addRegistrationFunction(OF_CONCAT(registerUnit_, __LINE__));                         \
            }                                                                                                          \
        };                                                                                                             \
                                                                                                                       \
        static OF_CONCAT(UnitRegistrar_, __LINE__) OF_CONCAT(unitRegistrarInstance_, __LINE__);                        \
    }
} // namespace OF

#endif // UNITREGISTRY_HPP

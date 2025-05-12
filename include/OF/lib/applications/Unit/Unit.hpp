// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UNIT_HPP
#define UNIT_HPP
#include <atomic>
#include <cstdint>
#include <string_view>

#include <zephyr/kernel.h>

namespace OF
{
    /**
     * @brief 单元信息结构体
     */
    struct UnitInfo
    {
        std::string_view name; //!< 单元名称
        std::string_view description; //!< 单元描述
        uint32_t stackSize; //!< 栈内存分配大小
        uint8_t priority; //!< 线程优先级
        bool isRunning; //!< 是否正在运行
        uint32_t typeId; //!< 属性ID

        /**
         * @brief 运行时统计信息结构体
         */
        struct RuntimeStats
        {
            uint32_t cpuUsage; //!< CPU 使用率
            uint32_t memoryUsage; //!< 内存使用量
        };
        RuntimeStats stats; //!< 运行时统计信息
    };


    /**
     * @brief 单元基类，定义了单元的基本行为和属性。
     *
     * 单元是框架中可独立运行的组件，通常拥有自己的线程。
     * 派生类需要实现纯虚函数，并可以通过静态成员函数提供默认配置。
     */
    class Unit
    {
    public:
        /**
         * @brief 初始化单元。
         * @details 此方法在单元线程启动前被调用，用于执行必要的初始化操作。
         *          派生类必须实现此方法。
         */
        virtual void init() = 0;

        /**
         * @brief 单元的主运行函数。
         * @details 此方法在单元线程中循环执行，直到 shouldStop 标志被设置为 true。
         *          派生类必须实现此方法。
         */
        virtual void run() = 0;

        /**
         * @brief 清理单元资源。
         * @details 此方法在单元线程停止后被调用，用于释放资源和执行清理操作。
         *          派生类可以重写此方法。
         */
        virtual void cleanup();

        /**
         * @brief 获取单元的类型ID。
         * @details 用于在运行时识别单元的具体类型，常用于安全的类型转换。
         *          派生类必须实现此方法，通常返回一个静态的、唯一的ID。
         * @return uint32_t 单元的类型ID。
         */
        virtual uint32_t getTypeId() const = 0;

        /**
         * @brief 获取单元的默认名称。
         * @return std::string_view 单元的默认名称。
         */
        static consteval std::string_view name() { return "Unnamed"; }

        /**
         * @brief 获取单元的默认描述。
         * @return std::string_view 单元的默认描述。
         */
        static consteval std::string_view description() { return ""; }

        /**
         * @brief 获取单元的默认栈大小。
         * @return size_t 单元的默认栈大小（字节）。
         */
        static consteval size_t stackSize() { return 1024; }

        /**
         * @brief 获取单元的默认线程优先级。
         * @return uint8_t 单元的默认线程优先级。
         */
        static consteval uint8_t priority() { return 0; }

        k_thread thread; //!< Zephyr 内核线程对象

        /**
         * @brief 单元的虚析构函数。
         */
        virtual ~Unit();

    protected:
        std::atomic<bool> shouldStop{false}; //!< 原子布尔值，用于指示单元是否应该停止运行。
    };

    /**
     * @brief Unit 派生类概念 (Concept)。
     * @details 用于约束模板参数 T 必须是 Unit 的派生类，并且满足特定的静态成员函数和类型定义要求。
     * @tparam T 需要检查的类型。
     */
    template <typename T>
    concept UnitDeriveConcept = std::derived_from<T, Unit> && requires {
        { T::name() } -> std::convertible_to<std::string_view>; //!< 必须有静态 name() 方法返回字符串视图
        { T::description() } -> std::convertible_to<std::string_view>; //!< 必须有静态 description() 方法返回字符串视图
        { T::stackSize() } -> std::convertible_to<size_t>; //!< 必须有静态 stackSize() 方法返回 size_t
        { T::priority() } -> std::convertible_to<uint8_t>; //!< 必须有静态 priority() 方法返回 uint8_t
        { T::TYPE_ID } -> std::convertible_to<uint32_t>; //!< 必须有静态 TYPE_ID 成员可转换为 uint32_t
        requires std::is_default_constructible_v<T>; //!< 必须是默认可构造的
    };

    /**
     * @brief 安全的单元类型转换函数 (类似 dynamic_cast)。
     * @tparam T 目标单元类型。
     * @param unit 指向 Unit 基类的指针。
     * @return 如果转换成功，返回指向目标类型 T 的指针；否则返回 nullptr。
     */
    template <typename T>
    T* unit_cast(Unit* unit)
    {
        return (unit && unit->getTypeId() == T::TYPE_ID) ? static_cast<T*>(unit) : nullptr;
    }

    /**
     * @brief 安全的单元类型转换函数 (const 版本)。
     * @tparam T 目标单元类型。
     * @param unit 指向 Unit 基类的 const 指针。
     * @return 如果转换成功，返回指向目标类型 T 的 const 指针；否则返回 nullptr。
     */
    template <typename T>
    const T* unit_cast(const Unit* unit)
    {
        return (unit && unit->getTypeId() == T::TYPE_ID) ? static_cast<const T*>(unit) : nullptr;
    }

    /**
     * @brief 启动所有已注册的单元。
     * @details 此函数会初始化单元注册表，创建所有单元实例，并为它们初始化和启动线程。
     *          此函数应在系统初始化阶段调用一次。
     */
    void StartUnits();

} // namespace OF

/** @} */ // End of app-Unit group

#endif // UNIT_HPP

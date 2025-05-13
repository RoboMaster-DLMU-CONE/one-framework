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
    struct UnitTypeDescriptor
    {
        std::string_view name{};
        std::string_view description{};
        size_t stackSize{};
        uint8_t priority{};
        uint32_t typeId{};
    };

    struct UnitRuntimeInfo
    {
        uint32_t cpuUsage{}; //!< CPU 使用率
        uint32_t memoryUsage{}; //!< 内存使用量
        bool isRunning{false};
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

        constexpr std::string_view getName() const { return typeDescriptor().name; }
        constexpr std::string_view getDescription() const { return typeDescriptor().description; }
        constexpr size_t getStackSize() const { return typeDescriptor().stackSize; }
        constexpr uint8_t getPriority() const { return typeDescriptor().priority; }
        constexpr uint32_t getTypeId() const { return typeDescriptor().typeId; }

        virtual const UnitTypeDescriptor& typeDescriptor() const = 0;

        k_thread _thread{}; //!< Zephyr 内核线程对象
        k_thread_stack_t* _stack{nullptr};
        UnitRuntimeInfo stats{};

        /**
         * @brief 单元的虚析构函数。
         */
        virtual ~Unit();
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

    // 简化Unit派生类定义的宏
#define DEFINE_UNIT_DESCRIPTOR(TypeName, NameStr, DescStr, StackSize, Priority)                                        \
    static constexpr UnitTypeDescriptor descriptor{NameStr, DescStr, StackSize, Priority, typeNameHash(#TypeName)};    \
    const UnitTypeDescriptor& typeDescriptor() const override { return descriptor; }

    /**
     * @brief 启动所有已注册的单元。
     * @details 此函数会初始化单元注册表，创建所有单元实例，并为它们初始化和启动线程。
     *          此函数应在系统初始化阶段调用一次。
     */
    void StartUnits();

} // namespace OF

#endif // UNIT_HPP

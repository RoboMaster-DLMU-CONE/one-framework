#ifndef UNIT_HPP
#define UNIT_HPP
#include <cstdint>
#include <string_view>

namespace OF
{

    struct UnitInfo
    {
        std::string_view name;
        std::string_view description;
        uint32_t stackSize;
        uint8_t priority;
        bool isRunning;
        uint32_t typeId; // 新增：类型ID

        struct RuntimeStats
        {
            uint32_t cpuUsage;
            uint32_t memoryUsage;
        } stats;
    };

    class Unit
    {
    public:
        virtual void init() = 0;
        virtual void run() = 0;
        virtual void cleanup();

        // 新增：获取类型ID的虚函数
        virtual uint32_t getTypeId() const = 0;

        static consteval std::string_view name() { return "Unnamed"; }
        static consteval std::string_view description() { return ""; }
        static consteval size_t stackSize() { return 1024; }
        static consteval uint8_t priority() { return 0; }

        virtual ~Unit();
    };

    // 代替dynamic_cast的安全类型转换函数
    template <typename T>
    T* unit_cast(Unit* unit)
    {
        return (unit && unit->getTypeId() == T::TYPE_ID) ? static_cast<T*>(unit) : nullptr;
    }

    template <typename T>
    const T* unit_cast(const Unit* unit)
    {
        return (unit && unit->getTypeId() == T::TYPE_ID) ? static_cast<const T*>(unit) : nullptr;
    }

} // namespace OF

#endif // UNIT_HPP

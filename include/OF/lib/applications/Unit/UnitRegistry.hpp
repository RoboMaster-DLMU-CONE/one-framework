#ifndef UNITREGISTRY_HPP
#define UNITREGISTRY_HPP

#include <memory>
#include <span>
#include <vector>
#include "Unit.hpp"

#define OF_CONCAT_INTERNAL(x, y) x##y
#define OF_CONCAT(x, y) OF_CONCAT_INTERNAL(x, y)

namespace OF
{
    // 单元特性
    template <typename T>
    struct UnitTraits
    {
        static constexpr UnitInfo info{.name = T::name(),
                                       .description = T::description(),
                                       .stackSize = T::stackSize(),
                                       .priority = T::priority(),
                                       .isRunning = false,
                                       .typeId = T::TYPE_ID,
                                       .stats = {}};

        static std::unique_ptr<Unit> create() { return std::make_unique<T>(); }
    };

    // 单元注册类
    class UnitRegistry
    {
    public:
        // 函数指针类型定义
        using UnitRegistrationFunction = void (*)();

        // 注册单个单元类型
        template <typename UnitType>
        static void registerUnit()
        {
            g_unitInfos.push_back(UnitTraits<UnitType>::info);
            g_unitFactories.push_back(UnitTraits<UnitType>::create);
        }

        // 添加注册函数到注册表
        static void addRegistrationFunction(const UnitRegistrationFunction func);
        // 初始化所有注册的单元
        static void initialize();
        // 标准访问方法
        static std::span<const UnitInfo> getUnits();
        static std::vector<std::unique_ptr<Unit>> createAllUnits();
        static const UnitInfo* findUnit(std::string_view name);
        static void updateUnitStatus(size_t idx, bool running);
        static void updateUnitStats(size_t idx, uint32_t cpu, uint32_t mem);

    private:
        static std::vector<UnitInfo> g_unitInfos;
        static std::vector<std::unique_ptr<Unit> (*)(void)> g_unitFactories;
        static std::vector<UnitRegistrationFunction> g_registrationFunctions;
    };

// 类型ID宏
#define DEFINE_UNIT_TYPE(TypeId)                                                                                       \
    static constexpr uint32_t TYPE_ID = TypeId;                                                                        \
    uint32_t getTypeId() const override { return TYPE_ID; }

    constexpr uint32_t typeNameHash(const char* str)
    {
        uint32_t hash = 5381;
        for (size_t i = 0; str[i]; ++i)
        {
            hash = ((hash << 5) + hash) ^ str[i];
        }
        return hash;
    }

#define AUTO_UNIT_TYPE(TypeName)                                                                                       \
    static constexpr uint32_t TYPE_ID = typeNameHash(#TypeName);                                                       \
    uint32_t getTypeId() const override { return TYPE_ID; }

// 简化版单元注册宏
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

#ifndef OF_LIB_HUBMANAGER_HPP
#define OF_LIB_HUBMANAGER_HPP

#include "HubBase.hpp"
#include <frozen/string.h>
#include <frozen/unordered_map.h>
#include <string_view>

namespace OF
{
    template <typename HubT, const auto& ConfigObj>
    struct HubEntry
    {
        using Type = HubT;
        static constexpr const auto& config = ConfigObj;
    };

    namespace GlobalCtx
    {
        template <typename T>
        struct HubStorage
        {
            static inline T* instance = nullptr;
        };
    }

    template <typename T>
    static void registerHub(T* ptr)
    {
        GlobalCtx::HubStorage<T>::instance = ptr;
    }

    template <typename T>
    static T* getHub()
    {
        return GlobalCtx::HubStorage<T>::instance;
    }

    template <typename... Entries>
    class HubManager
    {
    public:
        static void startAll()
        {
            (startOne<Entries>(), ...);
        }

        static auto* getHubByName(std::string_view name)
        {

            static constexpr auto map = frozen::make_unordered_map<frozen::string, IHub*>(
                {{Entries::Type::name, getHub<typename Entries::Type>()}...});

            auto it = map.find(name);
            if (it != map.end())
            {
                return (it->second)();
            }
            return nullptr;
        }

    private:
        template <typename Entry>
        static void startOne()
        {
            auto* hub = getHub<typename Entry::Type>();
            hub->configure(Entry::config);
            hub->init();
        }
    };
} // namespace OF
#endif

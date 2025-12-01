#ifndef OF_LIB_HUBMANAGER_HPP
#define OF_LIB_HUBMANAGER_HPP

#include "HubBase.hpp"

namespace OF
{
    template <typename T, typename... Args>
    concept Configurable = requires(T t, Args... args)
    {
        t.configure(args...);
    };

    class HubManager
    {
    public:
        class Builder
        {
        public:
            template <typename HubT, typename... Args>
            Builder& bind(std::vector<const device*> devs, Args&&... args)
            {
                LOG_MODULE_DECLARE(HubManager, CONFIG_HUB_MANAGER_LOG_LEVEL);
                HubT::getInstance().bindDevice(std::move(devs));
                if constexpr (sizeof...(Args) > 0)
                {
                    static_assert(Configurable<HubT, Args...>,
                                  "Error: You passed config arguments, but the Hub class does not have a matching configure() method.")
                        ;
                    HubT::getInstance().configure(std::forward<Args>(args)...);
                }
                return *this;
            }
        };

        static void startAll()
        {
            LOG_MODULE_DECLARE(HubManager, CONFIG_HUB_MANAGER_LOG_LEVEL);

            for (auto* hub : getHubs())
            {
                LOG_DBG("Init Hub: %s", hub->getName());
                hub->init();
            }
        }

        static void registerHub(IHub* hub)
        {
            getHubs().push_back(hub);
        }

    private:
        static std::vector<IHub*>& getHubs()
        {
            static std::vector<IHub*> hubs;
            return hubs;
        }
    };
}

#endif //OF_LIB_HUBMANAGER_HPP

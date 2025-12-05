#ifndef OF_LIB_HUB_REGISTRY_HPP
#define OF_LIB_HUB_REGISTRY_HPP

#include "HubManager.hpp"
#ifdef CONFIG_NOTIFY_HUB
#include <OF/lib/NotifyHub/NotifyHub.hpp>
#endif

#ifdef CONFIG_IMU_HUB
#include <OF/lib/ImuHub/ImuHub.hpp>
#endif

#ifdef CONFIG_CONTROLLER_HUB
#include <OF/lib/ControllerHub/ControllerHub.hpp>
#endif

#ifdef CONFIG_NOTIFY_HUB
extern const OF::NotifyHubConfig notify_hub_config;
#endif

#ifdef CONFIG_IMU_HUB
extern const OF::ImuHubConfig imu_hub_config;
#endif

#ifdef CONFIG_CONTROLLER_HUB
extern const OF::ControllerHubConfig controller_hub_config;
#endif

namespace OF
{

    using HubRegistry = HubManager<
#ifdef CONFIG_NOTIFY_HUB
        HubEntry<NotifyHub, notify_hub_config>
#endif

#ifdef CONFIG_IMU_HUB
        HubEntry<ImuHub, imu_hub_config>
#endif

#ifdef CONFIG_CONTROLLER_HUB
        HubEntry<ControllerHub, controller_hub_config>
#endif
    >;
}

#endif //OF_LIB_HUB_REGISTRY_HPP

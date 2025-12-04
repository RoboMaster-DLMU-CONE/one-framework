#ifndef OF_LIB_HUB_REGISTRY_HPP
#define OF_LIB_HUB_REGISTRY_HPP

#include "HubManager.hpp"
#include <OF/lib/NotifyHub/NotifyHub.hpp>

namespace OF
{
    extern constexpr NotifyHubConfig notify_hub_config = NotifyHubConfig{
        .status_leds_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds)),
        .pwm_buzzer_dev = DEVICE_DT_GET(DT_NODELABEL(pwm_buzzer))
    };

    using HubRegistry = HubManager<
        HubEntry<NotifyHub, notify_hub_config>
    >;
}

#endif //OF_LIB_HUB_REGISTRY_HPP
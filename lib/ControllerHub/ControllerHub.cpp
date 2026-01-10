// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/lib/ControllerHub/ControllerHub.hpp>
#include <frozen/unordered_map.h>
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(ControllerHub, CONFIG_CONTROLLER_HUB_LOG_LEVEL);


namespace OF
{
    const device* g_input_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(dji_dbus));
    OF_CCM_ATTR ControllerHub hub;

    namespace
    {
        struct ControllerHubRegistrar
        {
            ControllerHubRegistrar()
            {
                registerHub<ControllerHub>(&hub);
            }
        }
            __used controller_hub_registrar;
    }

    OF_CCM_ATTR SeqlockBuf<ControllerHubData> g_controller_buf;

    using enum ControllerHub::Channel;

    constexpr uint32_t make_key(const uint8_t type, const uint16_t code)
    {
        return (static_cast<uint32_t>(type) << 16) | code;
    }

    static constexpr frozen::unordered_map<uint32_t, ControllerHub::Channel, 7> g_map{
        {make_key(INPUT_EV_ABS, INPUT_ABS_X), LEFT_X},
        {make_key(INPUT_EV_ABS, INPUT_ABS_Y), LEFT_Y},
        {make_key(INPUT_EV_ABS, INPUT_ABS_RX), RIGHT_X},
        {make_key(INPUT_EV_ABS, INPUT_ABS_RY), RIGHT_Y},
        {make_key(INPUT_EV_ABS, INPUT_ABS_WHEEL), WHEEL},
        {make_key(INPUT_EV_KEY, INPUT_KEY_F1), SW_R},
        {make_key(INPUT_EV_KEY, INPUT_KEY_F2), SW_L},
    };


    ControllerHub::State ControllerHub::getData()
    {
        return g_controller_buf.read();
    }

    void ControllerHub::configure(const ControllerHubConfig& config)
    {
        if (config.input_device)
        {
            g_input_dev = config.input_device;
        }
    }


    void ControllerHub::setup()
    {
        if (!g_input_dev || !device_is_ready(g_input_dev))
        {
            LOG_ERR("invalid input device");
        }
    }

    static void input_cb(input_event* evt, void* user_data)
    {
        // filter message unwanted
        if (evt->type == 0)
            return;

        LOG_DBG("code: %d, value: %d", evt->code, evt->value);


        const auto key = make_key(evt->type, evt->code);
        // Get the reflected channel enum
        if (const auto it = g_map.find(key); it != g_map.end())
        {
            auto updater = [&](ControllerHubData& state)
            {
                state[it->second] = static_cast<int16_t>(evt->value);
            };
            g_controller_buf.manipulate(updater);
        }
    };

    INPUT_CALLBACK_DEFINE(g_input_dev, input_cb, nullptr);
} // namespace OF

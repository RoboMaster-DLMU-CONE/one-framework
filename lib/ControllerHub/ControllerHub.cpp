// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/lib/ControllerHub/ControllerHub.hpp>
#include <frozen/unordered_map.h>
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(ControllerHub, CONFIG_CONTROLLER_HUB_LOG_LEVEL);


namespace OF
{
    const device* g_input_dev;
    OF_CCM_ATTR ControllerHub hub;

    namespace
    {
        struct ControllerHubRegistrar
        {
            ControllerHubRegistrar()
            {
                registerHub<ControllerHub>(&hub);
            }
        } controller_hub_registrar;
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
        g_input_dev = config.input_device;
    }


    void ControllerHub::setup()
    {
        if (!g_input_dev || !device_is_ready(g_input_dev))
        {
            LOG_ERR("invalid input device");
        }
    }

    static std::function<void(ControllerHubData &)> func;


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
            // how about more optimization, maybe compare new and old value to optionally
            // tigger the data push, but that need another update and find()...

            func = [&](ControllerHubData& state)
            {
                // only set the channel we want
                state[it->second] = static_cast<int16_t>(evt->value);
            };

            // push the new value into seqlock buffer using HubBase's manipulation
            g_controller_buf.manipulate(func);
        }
    };

    INPUT_CALLBACK_DEFINE(g_input_dev, input_cb, nullptr);
} // namespace OF
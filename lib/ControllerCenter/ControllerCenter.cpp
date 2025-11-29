// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/ControllerCenter/ControllerCenter.hpp>
#include <frozen/unordered_map.h>
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(ControllerCenter, CONFIG_CONTROLLER_CENTER_LOG_LEVEL);


namespace OF
{

    using enum ControllerCenter::Channel;
    // TODO: make a tuple template unfold structure to gracefully add all reflect.
    static constexpr frozen::unordered_map<uint8_t, ControllerCenter::Channel, 7> MAP{
        {INPUT_ABS_X, LEFT_X},
        {INPUT_ABS_Y, LEFT_Y},
        {INPUT_ABS_RX, RIGHT_X},
        {INPUT_ABS_RY, RIGHT_Y},
        {INPUT_ABS_WHEEL, WHEEL},
        {INPUT_KEY_F1, SW_R},
        {INPUT_KEY_F2, SW_L},
    };
    ControllerCenter ControllerCenter::instance_;

    ControllerCenter& ControllerCenter::getInstance() { return instance_; }

    ControllerCenter::State ControllerCenter::getState()
    {
        return m_buf.read();
    }

    ControllerCenter::ControllerCenter()
    {
#ifdef CONFIG_INPUT_DBUS
        INPUT_CALLBACK_DEFINE(DEVICE_DT_GET_ANY(dji_dbus), input_cb, nullptr);
#endif
    }

    void ControllerCenter::input_cb(input_event* evt, void* user_data)
    {
        if (evt->type == 0)
            return;

        auto& inst = getInstance();
        if (const auto it = MAP.find(evt->type); it != MAP.end())
        {
            std::pair<uint8_t, Channel> new_value = {evt->value, it->second};
            static constexpr auto func = [](State& state, void* pair)
            {
                const auto [value, ch] = *static_cast<std::pair<uint8_t, Channel>*>(pair);
                state[ch] = value;
            };
            inst.m_buf.manipulate(func, &new_value);
        }
    };

} // namespace OF

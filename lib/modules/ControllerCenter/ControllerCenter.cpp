// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <OF/lib/modules/ControllerCenter/ControllerCenter.hpp>

#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(CC, CONFIG_CONTROLLER_CENTER_LOG_LEVEL);


namespace OF
{
    using enum ControllerCenter::Channel;
    ControllerCenter ControllerCenter::instance_;
    atomic_t ControllerCenter::idx = ATOMIC_INIT(0);

    ControllerCenter& ControllerCenter::getInstance() { return instance_; }
    // TODO: use template, shall we?
    int16_t ControllerCenter::operator[](const Channel ch) const
    {
        const int cur = atomic_get(&idx);
        const auto& [leftX, leftY, rightX, rightY, wheel, swL, swR] = buffers[cur];
        switch (ch)
        {
        case LEFT_X:
            return leftX;
        case LEFT_Y:
            return leftY;
        case RIGHT_X:
            return rightX;
        case RIGHT_Y:
            return rightY;
        case WHEEL:
            return wheel;
        case SW_L:
            return swL;
        case SW_R:
            return swR;
        default:;
        }
        return 0;
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
        const auto cur = atomic_get(&idx);
        const int next = 1 - cur;
        inst.buffers[next] = inst.buffers[cur];

        if (evt->type == INPUT_EV_ABS)
        {
            // LOG_DBG("code: %d, value: %d", evt->code, evt->value);

            switch (evt->code)
            {
            case INPUT_ABS_X:
                inst.buffers[next].leftX = evt->value;
                break;
            case INPUT_ABS_Y:
                inst.buffers[next].leftY = evt->value;
                break;
            case INPUT_ABS_RX:
                inst.buffers[next].rightX = evt->value;
                break;
            case INPUT_ABS_RY:
                inst.buffers[next].rightY = evt->value;
                break;
            case INPUT_ABS_WHEEL:
                inst.buffers[next].wheel = evt->value;
                break;
            default:;
            }
        }
        else if (evt->type == INPUT_EV_KEY)
        {
            switch (evt->code)
            {
            case INPUT_KEY_F1:
                inst.buffers[next].swR = evt->value;
                break;
            case INPUT_KEY_F2:
                inst.buffers[next].swL = evt->value;
                break;
            default:;
            }
        }
        atomic_set(&idx, next);
    };

} // namespace OF

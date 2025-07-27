// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONTROLLERCENTER_HPP
#define CONTROLLERCENTER_HPP
#include <zephyr/input/input.h>
#include <zephyr/sys/atomic.h>
namespace OF
{

    class ControllerCenter
    {
    public:
        enum class Channel
        {
            LEFT_X,
            LEFT_Y,
            RIGHT_X,
            RIGHT_Y,
            WHEEL,
            SW_L,
            SW_R,
        };

        enum class Switch
        {
#ifdef CONFIG_INPUT_DBUS
            UP = 1,
            MID = 3,
            DOWN = 2,
#else
#endif
        };

        static ControllerCenter& getInstance();

        int16_t operator[](Channel ch) const;

        ControllerCenter(ControllerCenter&) = delete;
        ControllerCenter& operator=(const ControllerCenter&) = delete;

    private:
        ControllerCenter();
        static ControllerCenter instance_;
        struct State
        {
            int16_t leftX{0}, leftY{0}, rightX{0}, rightY{0}, wheel{0}, swL{0}, swR{0};
        };
        State buffers[2];
        static atomic_t idx;
        static void input_cb(input_event* evt, void* user_data);
    };
} // namespace OF
#endif // CONTROLLERCENTER_HPP

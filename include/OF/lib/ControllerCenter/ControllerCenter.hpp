// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONTROLLERCENTER_HPP
#define CONTROLLERCENTER_HPP

#include <array>

#include <OF/utils/SeqlockBuf.hpp>

#include <zephyr/input/input.h>

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

        class State
        {
        public:
            int16_t& operator[](Channel ch)
            {
                return arr[static_cast<size_t>(ch)];
            }

            int16_t operator[](Channel ch) const
            {
                return arr[static_cast<size_t>(ch)];
            }

        private:
            std::array<int16_t, 7> arr{};
        };

        static ControllerCenter& getInstance();

        State getState();

        ControllerCenter(ControllerCenter&) = delete;
        ControllerCenter& operator=(const ControllerCenter&) = delete;

    private:
        ControllerCenter();
        static ControllerCenter instance_;


        SeqlockBuf<State> m_buf{};
        static void input_cb(input_event* evt, void* user_data);
    };
} // namespace OF
#endif // CONTROLLERCENTER_HPP

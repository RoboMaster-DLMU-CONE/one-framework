// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONTROLLERHUB_HPP
#define CONTROLLERHUB_HPP

#include <array>

#include <OF/lib/HubManager/HubBase.hpp>

#include <zephyr/input/input.h>

namespace OF
{
    // Forward declare State class for ControllerHub
    class ControllerHubState
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

    class ControllerHub : public HubBase<ControllerHub, ControllerHubState>
    {
    public:
        using Channel = ControllerHubState::Channel;
        using State = ControllerHubState;

        enum class Switch
        {
#ifdef CONFIG_INPUT_DBUS
            UP = 1,
            MID = 3,
            DOWN = 2,
#else
#endif
        };

        [[nodiscard]] constexpr char* getName() const override
        {
            return const_cast<char*>("ControllerHub");
        }

        State getState();

        void setup();

        ControllerHub(ControllerHub&) = delete;
        ControllerHub& operator=(const ControllerHub&) = delete;

    private:
        friend class HubBase<ControllerHub, ControllerHubState>;
        ControllerHub();

        static void input_cb(input_event* evt, void* user_data);
    };
} // namespace OF
#endif // CONTROLLERHUB_HPP

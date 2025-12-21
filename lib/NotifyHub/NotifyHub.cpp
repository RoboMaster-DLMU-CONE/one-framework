#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/utils/CCM.h>

#include <OF/drivers/output/buzzer.h>

#include <ankerl/unordered_dense.h>

namespace OF
{
    OF_CCM_ATTR NotifyHub hub;

    namespace
    {
        struct NotifyHubRegistrar
        {
            NotifyHubRegistrar()
            {
                registerHub<NotifyHub>(&hub);
            }
        } notifyHubRegistrar;
    }

    LOG_MODULE_REGISTER(NotifyHub, CONFIG_NOTIFY_HUB_LOG_LEVEL);

    OF_CCM_ATTR ankerl::unordered_dense::map<FixedString<MAX_KEY_LEN>, BuzzerStatus> g_buzzer_noti;
    OF_CCM_ATTR ankerl::unordered_dense::map<FixedString<MAX_KEY_LEN>, LEDStatus> g_led_noti;

    OF_CCM_ATTR k_msgq g_buzzer_msgq;
    OF_CCM_ATTR k_msgq g_led_msgq;

    OF_CCM_ATTR char __aligned(4) g_buzzer_msgq_buffer[16 * sizeof(BuzzerCommand)];
    OF_CCM_ATTR char __aligned(4) g_led_msgq_buffer[16 * sizeof(BuzzerCommand)];

    K_THREAD_STACK_DEFINE(g_buzzer_stack, 1024);
    K_THREAD_STACK_DEFINE(g_led_stack, 1024);


    void NotifyHub::configure(const NotifyHubConfig& config)
    {
        m_buzzer = config.pwm_buzzer_dev;
        m_led_pixel = config.led_pixel_dev;
    }

    void NotifyHub::setup()
    {
        m_buzzer_tid = k_thread_create(&m_buzzer_thread, g_buzzer_stack, K_THREAD_STACK_SIZEOF(g_buzzer_stack),
                                       m_buzzer_thread_entry,
                                       const_cast<device*>(m_buzzer), nullptr, nullptr,
                                       0, 0, K_NO_WAIT);
        m_led_tid = k_thread_create(&m_led_thread, g_led_stack, K_THREAD_STACK_SIZEOF(g_led_stack),
                                    m_led_thread_entry,
                                    const_cast<device*>(m_led_pixel), nullptr, nullptr,
                                    0, 0, K_NO_WAIT);

        k_msgq_init(&g_buzzer_msgq, g_buzzer_msgq_buffer, sizeof(BuzzerCommand), 16);
        k_msgq_init(&g_led_msgq, g_led_msgq_buffer, sizeof(LEDCommand), 16);
    }

    void NotifyHub::setBuzzerStatus(const FixedString<MAX_KEY_LEN>& key, const BuzzerStatus& status)
    {
        const BuzzerCommand command{CommandType::SET, key, status};
        while (k_msgq_put(&g_buzzer_msgq, &command, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&g_buzzer_msgq);
        }
    }

    void NotifyHub::removeBuzzerStatus(const FixedString<MAX_KEY_LEN>& key)
    {
        const BuzzerCommand command{CommandType::REMOVE, key, {}};
        while (k_msgq_put(&g_buzzer_msgq, &command, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&g_buzzer_msgq);
        }
    }

    void NotifyHub::setLEDStatus(const FixedString<MAX_KEY_LEN>& key, const LEDStatus& status)
    {
        const LEDCommand command{CommandType::SET, key, status};
        while (k_msgq_put(&g_led_msgq, &command, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&g_led_msgq);
        }
    }

    void NotifyHub::removeLEDStatus(const FixedString<MAX_KEY_LEN>& key)
    {
        const LEDCommand command{CommandType::REMOVE, key, {}};
        while (k_msgq_put(&g_led_msgq, &command, K_NO_WAIT) != 0)
        {
            k_msgq_purge(&g_led_msgq);
        }
    }


    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    [[noreturn]] void NotifyHub::m_buzzer_thread_entry(void* p1, void*, void*)
    {
        const auto buzzer_dev = static_cast<const device*>(p1);

        while (true)
        {
            BuzzerCommand command;
            while (k_msgq_get(&g_buzzer_msgq, &command, K_MSEC(800)) == 0)
            {
                switch (command.type)
                {
                case CommandType::SET:
                    g_buzzer_noti[command.key] = command.status;
                    break;
                case CommandType::REMOVE:
                    auto it = g_buzzer_noti.find(command.key);
                    if (it != g_buzzer_noti.end())
                    {
                        g_buzzer_noti.erase(it);
                    }
                    break;
                }
            }

            constexpr int DELAY_MSEC = 100;

            for (auto it = g_buzzer_noti.begin(); it != g_buzzer_noti.end();)
            {
                auto& [str, status] = *it;
                for (const auto& [ratio, duration_ms] : status.notes)
                {
                    pwm_buzzer_play_note(buzzer_dev, ratio, status.volume);
                    k_sleep(K_MSEC(duration_ms));
                    pwm_buzzer_stop(buzzer_dev);
                    k_sleep(K_MSEC(1));
                }
                pwm_buzzer_stop(buzzer_dev);
                if (status.once)
                {
                    it = g_buzzer_noti.erase(it);
                }
                else
                {
                    ++it;
                }
                k_sleep(K_MSEC(DELAY_MSEC));
            }
        }
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    [[noreturn]] void NotifyHub::m_led_thread_entry(void* p1, void* p2, void* p3)
    {
        const auto led_dev = static_cast<const device*>(p1);

        while (true)
        {
            LEDCommand command;
            while (k_msgq_get(&g_led_msgq, &command, K_MSEC(800)) == 0)
            {
                switch (command.type)
                {
                case CommandType::SET:
                    g_led_noti[command.key] = command.status;
                    break;
                case CommandType::REMOVE:
                    auto it = g_led_noti.find(command.key);
                    if (it != g_led_noti.end())
                    {
                        g_led_noti.erase(it);
                    }
                    break;
                }
            }

            constexpr int DELAY_MSEC = 100;

            for (auto it = g_led_noti.begin(); it != g_led_noti.end();)
            {
                auto& [str, status] = *it;
                auto& [color, mode, toggle_times, toggle_interval_ms, once] = status;
                switch (mode)
                {
                case LEDMode::Solid:
                    led_pixel_set(led_dev, color, 80);
                    break;
                case LEDMode::Blink:
                    for (uint8_t i{}; i < toggle_times * 2; ++i)
                    {
                        if (i % 2 == 0)
                        {
                            led_pixel_set(led_dev, color, 80);
                        }
                        else
                        {
                            led_pixel_off(led_dev);
                        }
                        k_sleep(K_MSEC(toggle_interval_ms));
                    }
                    break;
                case LEDMode::Breathing:
                    for (uint8_t i{}; i < toggle_times * 2; ++i)
                    {
                        const uint32_t cycle_start = k_cycle_get_32();
                        if (i % 2 == 0)
                        {
                            // 升亮阶段
                            while (true)
                            {
                                const uint32_t cycle_now = k_cycle_get_32();
                                const auto elapsed_us = k_cyc_to_us_ceil32(cycle_now - cycle_start);
                                const uint32_t elapsed_ms = elapsed_us / 1000;

                                uint8_t bright = (elapsed_ms * 100) / toggle_interval_ms;
                                if (bright > 100) bright = 100;
                                led_pixel_set(led_dev, color, bright);
                                k_sleep(K_USEC(100));

                                if (bright >= 100) break;
                            }
                        }
                        else
                        {
                            // 降暗阶段
                            while (true)
                            {
                                const uint32_t cycle_now = k_cycle_get_32();
                                const auto elapsed_us = k_cyc_to_us_ceil32(cycle_now - cycle_start);
                                const uint32_t elapsed_ms = elapsed_us / 1000;

                                int32_t bright = 100 - (elapsed_ms * 100) / toggle_interval_ms;
                                if (bright < 0) bright = 0;
                                led_pixel_set(led_dev, color, static_cast<uint8_t>(bright));
                                k_sleep(K_USEC(10));
                                if (bright <= 0) break;
                            }
                        }
                    }
                    break;
                }


                if (status.once)
                {
                    it = g_led_noti.erase(it);
                }
                else
                {
                    ++it;
                }
                k_sleep(K_MSEC(DELAY_MSEC));
            }
        }
    }
}

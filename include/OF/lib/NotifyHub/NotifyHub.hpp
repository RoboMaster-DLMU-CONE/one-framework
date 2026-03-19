#ifndef OF_NOTIFYHUB_HPP
#define OF_NOTIFYHUB_HPP

#include <OF/drivers/output/led_pixel.h>

#include <zephyr/kernel.h>
#include <OF/utils/FixedString.hpp>
#include <ems_parser.hpp>
#include <optional>
#include <span>

namespace OF
{
    constexpr size_t MAX_KEY_LEN = 32;

    struct BuzzerStatus
    {
        std::span<const ems::Note> notes;
        uint8_t volume;
        bool once = false;

        bool operator==(const BuzzerStatus& other) const
        {
            if (volume != other.volume || once != other.once) return false;
            if (notes.size() != other.notes.size()) return false;
            return notes.data() == other.notes.data(); // compare pointer (same span source)
        }
    };

    enum class LEDMode : uint8_t
    {
        Solid, // 常亮
        Blink, // 闪烁
        Breathing, // 呼吸
    };

    struct LEDStatus
    {
        led_color color;

        LEDMode mode;
        uint8_t toggle_times;
        uint16_t toggle_interval_ms;
        bool once = false;

        bool operator==(const LEDStatus& other) const
        {
            return color.r == other.color.r &&
                   color.g == other.color.g &&
                   color.b == other.color.b &&
                   mode == other.mode &&
                   toggle_times == other.toggle_times &&
                   toggle_interval_ms == other.toggle_interval_ms &&
                   once == other.once;
        }
    };

    enum class CommandType: uint8_t { SET, REMOVE };

    struct BuzzerCommand
    {
        CommandType type;
        FixedString<MAX_KEY_LEN> key;

        BuzzerStatus status;
    };

    struct LEDCommand
    {
        CommandType type;
        FixedString<MAX_KEY_LEN> key;

        LEDStatus status;
    };

    class NotifyHub final
    {
    public:
        static constexpr auto name = "NotifyHub";

        void setup();

        static void setBuzzerStatus(const FixedString<MAX_KEY_LEN>& key, const BuzzerStatus& status);
        static void removeBuzzerStatus(const FixedString<MAX_KEY_LEN>& key);

        static void setLEDStatus(const FixedString<MAX_KEY_LEN>& key, const LEDStatus& status);
        static void removeLEDStatus(const FixedString<MAX_KEY_LEN>& key);

    private:
        const device* m_buzzer{nullptr};
        const device* m_led_pixel{nullptr};

        [[noreturn]] static void m_buzzer_thread_entry(void* p1, void* p2, void* p3);
        [[noreturn]] static void m_led_thread_entry(void* p1, void* p2, void* p3);

        k_thread m_buzzer_thread{};
        k_thread m_led_thread{};
        k_tid_t m_buzzer_tid{};
        k_tid_t m_led_tid{};
    };
}

#endif //OF_NOTIFYHUB_HPP

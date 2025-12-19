#ifndef OF_NOTIFYHUB_HPP
#define OF_NOTIFYHUB_HPP
#include <OF/lib/HubManager/HubBase.hpp>
#include <ems_parser.hpp>
#include <tl/expected.hpp>
#include <optional>
#include <span>

namespace OF
{
    enum class LEDColor
    {
        SUCCESS,
        FAILURE
    };

    struct LEDStatus
    {
        LEDColor color;
        uint8_t freq;
        uint8_t times;
    };

    struct BuzzerStatus
    {
        std::span<ems::Note> notes;
    };

    struct Notification
    {
        std::optional<LEDStatus> led_status = std::nullopt;
        std::optional<BuzzerStatus> buzzer_status = std::nullopt;
    };

    struct NotifyHubConfig
    {
        const device* status_leds_dev;
        const device* pwm_buzzer_dev;
    };

    class NotifyHub final : public HubBase<NotifyHub>
    {
    public:
        static constexpr auto name = "NotifyHub";

        void configure(const NotifyHubConfig& config);

        void setup();

        static void setNotification(const std::string& unique_name, const Notification& notification);

    private:
        const device* m_leds{nullptr};
        const device* m_buzzer{nullptr};

        static void m_thread_entry(void* p1, void* p2, void*);

        k_thread m_thread{};
        k_tid_t m_tid{};
    };
}

#endif //OF_NOTIFYHUB_HPP

#ifndef OF_NOTIFYHUB_HPP
#define OF_NOTIFYHUB_HPP
#include <OF/lib/HubManager/HubBase.hpp>
#include <ems_parser.hpp>
#include <tl/expected.hpp>
#include <optional>
#include <span>

namespace OF
{
    struct BuzzerStatus
    {
        std::span<ems::Note> notes;
    };

    struct Notification
    {
        std::optional<BuzzerStatus> buzzer_status = std::nullopt;
    };

    struct NotifyHubConfig
    {
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
        const device* m_buzzer{nullptr};

        static void m_thread_entry(void* p1, void* p2, void* p3);

        k_thread m_thread{};
        k_tid_t m_tid{};
    };
}

#endif //OF_NOTIFYHUB_HPP

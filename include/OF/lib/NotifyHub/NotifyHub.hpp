#ifndef OF_NOTIFYHUB_HPP
#define OF_NOTIFYHUB_HPP
#include <OF/lib/HubManager/HubBase.hpp>

namespace OF
{
    struct NotifyData
    {
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

    private:
        const device* m_leds{nullptr};
        const device* m_buzzer{nullptr};
    };
}

#endif //OF_NOTIFYHUB_HPP
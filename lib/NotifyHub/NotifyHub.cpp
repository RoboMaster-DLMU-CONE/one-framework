#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/utils/CCM.h>

#include <OF/drivers/output/buzzer.h>

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

    void NotifyHub::configure(const NotifyHubConfig& config)
    {
        m_leds = config.status_leds_dev;
        m_buzzer = config.pwm_buzzer_dev;
    }

    void NotifyHub::setup()
    {
    }
}
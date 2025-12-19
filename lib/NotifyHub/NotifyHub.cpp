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

    OF_CCM_ATTR ankerl::unordered_dense::map<std::string, Notification> g_notifications;

    K_THREAD_STACK_DEFINE(g_notify_hub_stack, 1024);
    OF_CCM_ATTR k_work_delayable g_notify_hub_work;


    void NotifyHub::configure(const NotifyHubConfig& config)
    {
        m_buzzer = config.pwm_buzzer_dev;
    }

    void NotifyHub::setup()
    {
        k_thread_create(&m_thread, g_notify_hub_stack, K_THREAD_STACK_SIZEOF(g_notify_hub_stack), m_thread_entry,
                        const_cast<device*>(m_buzzer), nullptr, nullptr,
                        CONFIG_NUM_PREEMPT_PRIORITIES - 1, 0, K_NO_WAIT);
    }

    void NotifyHub::setNotification(const std::string& unique_name,
                                    const Notification& notification)
    {
        g_notifications[unique_name] = notification;
    }

    void NotifyHub::m_thread_entry(void* p1, void* p2, void* p3)
    {
        (void)p1;
        (void)p2;
        (void)p3;

        constexpr int DELAY_TIME = 100;

        for (auto& [_, noti] : g_notifications)
        {
            if (noti.buzzer_status)
            {
            }
        }
    }
}

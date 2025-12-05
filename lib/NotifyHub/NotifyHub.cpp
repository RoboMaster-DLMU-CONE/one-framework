#include <OF/lib/NotifyHub/NotifyHub.hpp>
#include <OF/lib/HubManager/HubManager.hpp>
#include <OF/utils/CCM.h>

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
}

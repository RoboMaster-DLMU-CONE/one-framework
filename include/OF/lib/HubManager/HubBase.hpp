#ifndef OF_LIB_HUBBASE_HPP
#define OF_LIB_HUBBASE_HPP

#include <zephyr/device.h>

#include <OF/utils/SeqlockBuf.hpp>
#include <zephyr/logging/log.h>

#include <algorithm>

#include "OF/utils/CCM.h"

namespace OF
{
    // Forward declaration
    class HubManager;

    class IHub
    {
    public:
        [[nodiscard]] virtual const char* getName() const = 0;
        virtual void init() = 0;
        virtual bool isReady() = 0;

    protected:
        virtual ~IHub() = default;
    };

    template <typename T, typename DataT, bool PlaceInCcm = false>
    class HubBase : public IHub
    {
    public:
        static T& getInstance()
        {
            return Storage::instance;
        }

        void bindDevice(std::vector<const device*> devices)
        {
            m_devs = std::move(devices);
        }

        void init() override
        {
            LOG_MODULE_DECLARE(HubManager, CONFIG_HUB_MANAGER_LOG_LEVEL);
            for (size_t i = 0; i < m_devs.size(); ++i)
            {
                if (!device_is_ready(m_devs[i]))
                {
                    LOG_ERR("Device %d for %s is not ready!", i, getName());
                    return;
                }
            }
            static_cast<T*>(this)->setup();
        };

        bool isReady() override
        {
            return std::ranges::all_of(m_devs, [](auto* dev) { return device_is_ready(dev); });
        };

        DataT getData()
        {
            return m_data.read();
        }

    protected:
        HubBase() = default;
        std::vector<const device*> m_devs;

        template <typename Func>
        void manipulateData(Func& func)
        {
            m_data.manipulate(func);
        }

        void updateData(const DataT& data)
        {
            m_data.write(data);
        }

    private:
        SeqlockBuf<DataT> m_data;

        struct StorageDefault
        {
            static T instance;
        };

        struct StorageCcm
        {
            static OF_CCM_ATTR T instance;
        };

        using Storage = std::conditional_t<PlaceInCcm, StorageCcm, StorageDefault>;
    };

    template <typename T, typename DataT, bool PlaceInCcm>
    T HubBase<T, DataT, PlaceInCcm>::StorageDefault::instance;

    template <typename T, typename DataT, bool PlaceInCcm>
    OF_CCM_ATTR T HubBase<T, DataT, PlaceInCcm>::StorageCcm::instance;

};

#endif //OF_LIB_HUBBASE_HPP

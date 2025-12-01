#ifndef OF_LIB_HUBBASE_HPP
#define OF_LIB_HUBBASE_HPP

#include <zephyr/device.h>

#include <OF/utils/SeqlockBuf.hpp>
#include <zephyr/logging/log.h>

#include <algorithm>
#include <atomic>
#include <mutex>

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

    template <typename T, typename DataT>
    class HubBase : public IHub
    {
    public:
        static T& getInstance()
        {
            static T instance;
            static std::atomic<bool> registered{false};
            bool expected = false;
            if (registered.compare_exchange_strong(expected, true))
            {
                HubManager::registerHub(&instance);
            }
            return instance;
        }

        void bindDevice(std::vector<const device*> devices)
        {
            m_devs = std::move(devices);
        }

        void init() override
        {
            LOG_MODULE_DECLARE(HubManager, CONFIG_HUB_MANAGER_LOG_LEVEL);
            for (int i = 0; i < m_devs.size(); ++i)
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
            std::call_once(m_init_flag, [this]() { this->init(); });
            return m_data.read();
        }

        template<typename Func, typename... Args>
        void manipulateData(Func func, Args&&... args)
        {
            m_data.manipulate(func, std::forward<Args>(args)...);
        }

    protected:
        HubBase() = default;
        std::vector<const device*> m_devs;

        void updateData(const DataT& data)
        {
            m_data.write(data);
        }

    private:
        SeqlockBuf<DataT> m_data;
        std::once_flag m_init_flag;
    };
};

#endif //OF_LIB_HUBBASE_HPP

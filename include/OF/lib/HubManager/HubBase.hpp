#ifndef OF_LIB_HUBBASE_HPP
#define OF_LIB_HUBBASE_HPP

#include <zephyr/device.h>

#include "OF/utils/SeqlockBuf.hpp"
#include "zephyr/logging/log.h"

namespace OF
{
    class IHub
    {
        [[nodiscard]] virtual constexpr char* getName() const = 0;
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
            return instance;
        }

        void bindDevice(const device* device)
        {
            m_dev = device;
        }

        void init() override
        {
            if (!m_dev)
            {
                LOG_ERR("%s has no device bound", getName());
                return;
            }
            if (!device_is_ready(m_dev))
            {
                LOG_ERR("Device for %s is not ready!", getName());
                return;
            }
            static_cast<T*>(this)->setup();
        };

        bool isReady() override
        {
            return m_dev && device_is_ready(m_dev);
        };

        DataT getData()
        {
            return m_data.read();
        }

    protected:
        HubBase() = default;
        const device* m_dev{nullptr};

        void updateData(const DataT& data)
        {
            m_data.write(data);
        }

    private:
        SeqlockBuf<DataT> m_data;
    };
};

#endif //OF_LIB_HUBBASE_HPP

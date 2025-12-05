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
    template <typename... Entries>
    class HubManager;

    class IHub
    {
    public:
        virtual void init() = 0;
        virtual const char* getName() = 0;

    protected:
        virtual ~IHub() = default;
    };

    template <typename T>
    class HubBase : public IHub
    {
    public:
        HubBase() = default;


        void init() override
        {
            static_cast<T*>(this)->setup();
        };

        const char* getName() override
        {
            return static_cast<T*>(this)->name;
        };
    };

}; // namespace OF

#endif // OF_LIB_HUBBASE_HPP

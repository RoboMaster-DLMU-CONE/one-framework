#ifndef OF_DOUBLEBUFFER_HPP
#define OF_DOUBLEBUFFER_HPP

#include <functional>

#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>

namespace OF
{

    template <typename T>
    class SeqlockBuf
    {
    public:
        // no lock
        void write(const T& val)
        {
            // 1. version + 1 (odd), writing...
            atomic_inc(&m_version);

            // 2. writing data
            compiler_barrier();
            m_data = val;
            compiler_barrier();

            // 3. version + 1 (even)，written done.
            atomic_inc(&m_version);
        }

        void manipulate(std::function<void(T&, void*)> func, void* data)
        {
            // 1. version + 1 (odd), writing...
            atomic_inc(&m_version);

            // 2. manipulate data
            compiler_barrier();
            func(m_data, data);
            compiler_barrier();

            // 3. version + 1 (even)，written done.
            atomic_inc(&m_version);
        };

        // optimistic reading
        T read()
        {
            T val;
            uint32_t v1, v2{};
            do
            {
                // 1. get version
                v1 = atomic_get(&m_version);

                // odd -> someone's writing, keep waiting...
                if (v1 & 1)
                {
                    k_yield();
                    continue;
                }

                // 2. copy data
                compiler_barrier();
                val = m_data;
                compiler_barrier();

                // 3. get version again
                v2 = atomic_get(&m_version);

                // 4. version changed(v1 != v2) means data corrupted, retry.
            }
            while (v1 != v2);

            return val;
        }

    private:
        T m_data{};
        atomic_t m_version = ATOMIC_INIT(0);
    };
};

#endif //OF_DOUBLEBUFFER_HPP

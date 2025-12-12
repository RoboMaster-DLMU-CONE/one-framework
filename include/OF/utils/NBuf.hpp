#ifndef OF_TRIPLEBUF_HPP
#define OF_TRIPLEBUF_HPP

#include <new>
#include <array>
#include <zephyr/sys/atomic.h>

#include <optional>

using std::hardware_destructive_interference_size;

namespace OF
{
    // SPMC N-Buffer Class
    template <typename T, size_t N>
        requires std::is_standard_layout_v<T> && (N >= 2)
    class NBuf
    {
    public:
        NBuf() = default;

        NBuf(const NBuf&) = delete;
        NBuf& operator =(const NBuf&) = delete;
        NBuf(NBuf&& other) = delete;
        NBuf& operator =(NBuf&& other) = delete;

        void write(const T& data) noexcept
        {
            auto next_slot = (m_next_write_idx + 1) % N;
            auto& slot = m_slots[next_slot];
            atomic_inc(slot.version);
            compiler_barrier();
            slot.data = data;
            compiler_barrier();
            atomic_inc(slot.version);

            atomic_set(&m_latest_idx, next_slot);
            m_next_write_idx = next_slot;
        }

        template <typename Func>
        void manipulate(const Func& func)
        {
            auto next_slot = (m_next_write_idx + 1) % N;
            auto& slot = m_slots[next_slot];

            atomic_inc(slot.version);
            compiler_barrier();
            func(slot.data);
            compiler_barrier();
            atomic_inc(slot.version);


            atomic_set(&m_latest_idx, next_slot);
            m_next_write_idx = next_slot;
        }

        std::optional<T> try_read() const noexcept
        {
            T out_data;
            auto& slot = m_slots[atomic_get(&m_latest_idx)];

            auto v1 = atomic_get(&slot.version);

            compiler_barrier();
            out_data = slot.data;
            compiler_barrier();

            auto v2 = atomic_get(&slot.version);
            return v1 == v2 ? out_data : std::nullopt;

        }

        T read() const noexcept
        {
            T copy;
            auto& slot = m_slots[atomic_get(&m_latest_idx)];
            atomic_val_t v1, v2{};
            do
            {
                v1 = atomic_get(&slot.version);

                if (v1 & 1)
                {
                    k_yield();
                    continue;
                }

                compiler_barrier();
                copy = slot.data;
                compiler_barrier();

                v2 = atomic_get(&slot.version);
            }
            while (v1 != v2);
            return copy;
        }

    private:
        struct alignas (hardware_destructive_interference_size) Slot
        {
            atomic_t version = ATOMIC_INIT(0);
            T data;
        };

        std::array<Slot, N> m_slots;

        atomic_t m_latest_idx = ATOMIC_INIT(0);

        atomic_val_t m_next_write_idx{0};
    };
}

#endif //OF_TRIPLEBUF_HPP

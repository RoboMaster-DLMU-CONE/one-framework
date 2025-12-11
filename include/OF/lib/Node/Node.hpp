#ifndef OF_LIB_NODE_HPP
#define OF_LIB_NODE_HPP

#include <concepts>

#include <zephyr/logging/log.h>

#include "Macro.hpp"
#include "Descriptor.hpp"

namespace OF
{

    template <typename T>
    concept NodeConcept = requires
    {
        { T::Meta::stack_size } -> std::convertible_to<size_t>;
        { T::Meta::priority } -> std::convertible_to<int>;
        { T::Meta::name } -> std::convertible_to<const char*>;
        { std::declval<T>().init() } -> std::same_as<bool>;
        { std::declval<T>().run() } -> std::same_as<void>;
        { std::declval<T>().cleanup() } -> std::same_as<void>;
    };

    template <typename Derived>
    class Node
    {
    public:
        static void zephyr_entry_point(void* p1, void*, void*)
        {
            static_assert(NodeConcept<Derived>,
                          "Your Node must define a 'struct Meta { stack_size, priority, name };'");
            LOG_MODULE_DECLARE(NodeSystem, CONFIG_NODE_LOG_LEVEL);
            auto* node = static_cast<Derived*>(p1);

            if (!node->init())
            {
                LOG_ERR("Node %s failed to init", Derived::Meta::name);
                return;
            }

            node->run();

            node->cleanup();

        }

        static void start_impl(k_thread* thread_data, k_thread_stack_t* stack_area)
        {
            k_tid_t tid = k_thread_create(thread_data, stack_area, Derived::Meta::stack_size, zephyr_entry_point,
                                          &instance(), nullptr,
                                          nullptr, Derived::Meta::priority, 0, K_NO_WAIT);
            k_thread_name_set(tid, Derived::Meta::name);
            Derived::tid_storage = tid;
        }

        static Derived& instance()
        {
            static Derived instance;
            return instance;
        }

        inline static k_tid_t tid_storage = nullptr;

    };
}

#endif //OF_LIB_NODE_HPP

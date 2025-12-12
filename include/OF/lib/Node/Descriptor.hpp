#ifndef OF_LIB_NODE_DESCRIPTOR_HPP
#define OF_LIB_NODE_DESCRIPTOR_HPP
#include "zephyr/kernel/thread.h"

namespace OF
{
    struct node_desc
    {
        const char* name;
        k_tid_t* thread_id_ptr;

        void (*start_func)();
    };

    typedef void (*print_func_t)(const struct topic_desc* desc);
    typedef size_t (*get_size_func_t)();

    struct topic_desc
    {
        const char* name;
        void* topic_instance;

        uint32_t type_size;
        print_func_t print_func;
    };
}

#endif //OF_LIB_NODE_DESCRIPTOR_HPP

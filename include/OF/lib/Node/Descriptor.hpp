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
}

#endif //OF_LIB_NODE_DESCRIPTOR_HPP

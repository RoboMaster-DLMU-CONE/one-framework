#ifndef OF_LIB_NODE_MACRO_HPP
#define OF_LIB_NODE_MACRO_HPP

#include <zephyr/sys/iterable_sections.h>

#include "Descriptor.hpp"

#define ONE_NODE_REGISTER(UserClass) \
    /* stack definition */ \
    K_THREAD_STACK_DEFINE(_stack_##UserClass, UserClass::Meta::stack_size); \
    /* thread data */ \
    static struct k_thread _thread_data_##UserClass;\
    /* launcher */ \
    static void _launcher_##UserClass(void) { \
        UserClass::start_impl(&_thread_data_##UserClass, _stack_##UserClass); \
    } \
    /* register it into global linker section */ \
    STRUCT_SECTION_ITERABLE(node_desc, _desc_##UserClass) ={ \
        .name = UserClass::Meta::name, \
        .thread_id_ptr = &UserClass::tid_storage, \
        .start_func = &_launcher_##UserClass \
        }

#endif //OF_LIB_NODE_MACRO_HPP

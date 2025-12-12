#ifndef OF_LIB_NODE_MACRO_HPP
#define OF_LIB_NODE_MACRO_HPP

#include <zephyr/sys/iterable_sections.h>
#include <OF/utils/CCM.h>

#include "Node.hpp"
#include "Descriptor.hpp"
#include "Topic.hpp"


#define ONE_NODE_REGISTER(UserClass) \
    static_assert(NodeConcept<UserClass>, \
        "Your Node must define a full Meta struct and implement init, run and cleanup function! See 'NodeConcept' for more detail. ' " \
    ); \
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

#define ONE_TOPIC_REGISTER(Type, VarName, TopicNameStr) \
    \
    /* CCM Topic instance and reference */ \
    OF_CCM_ATTR static OF::Topic<Type> _topic_instance_##VarName; \
    OF::Topic<Type>& VarName = _topic_instance_##VarName;\
    /* register it into global linker section */ \
    STRUCT_SECTION_ITERABLE(topic_desc, _topic_desc_##VarName) = { \
        .name = TopicNameStr, \
        .topic_instance = &_topic_instance_##VarName, \
        .type_size = sizeof(Type),\
        .print_func = OF::Topic<Type>::print_stub \
    };


#endif //OF_LIB_NODE_MACRO_HPP

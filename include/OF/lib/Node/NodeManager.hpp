#ifndef OF_LIB_NODEMANAGER_HPP
#define OF_LIB_NODEMANAGER_HPP

#include "Descriptor.hpp"


namespace OF
{
    extern const node_desc _node_desc_list_start[];
    extern const node_desc _node_desc_list_end[];

    void start_all_nodes()
    {
        LOG_MODULE_DECLARE(NodeSystem, CONFIG_NODE_LOG_LEVEL);
        STRUCT_SECTION_FOREACH(node_desc, desc)
        {
            LOG_INF("Starting Node: %s", desc->name);
            desc->start_func();
        }
    }
}

#endif //OF_LIB_NODEMANAGER_HPP

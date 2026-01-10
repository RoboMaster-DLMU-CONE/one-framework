#include <OF/lib/Node/Descriptor.hpp>
#include <OF/lib/Node/Node.hpp>
#include <OF/lib/Node/NodeManager.hpp>
#include <OF/lib/Node/Topic.hpp>

#include <zephyr/logging/log.h>

extern "C" {
extern OF::node_desc _node_desc_list_start[];
extern OF::node_desc _node_desc_list_end[];
}

namespace OF
{
    LOG_MODULE_REGISTER(NodeSystem, CONFIG_NODE_LOG_LEVEL);

    void start_all_nodes()
    {
        for (const node_desc* desc = _node_desc_list_start; desc < _node_desc_list_end; ++desc)
        {
            LOG_INF("Starting Node: %s", desc->name);
            if (!desc->start_func)
            {
                LOG_ERR("Node %s has null start_func", desc->name);
                continue;
            }
            desc->start_func();
        }
    }
}

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NODE
        Node.cpp
)
zephyr_linker_sources_ifdef(CONFIG_NODE DATA_SECTIONS linker/node_sections.ld)
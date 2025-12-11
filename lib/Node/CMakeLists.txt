zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NODE
        Node.cpp
)

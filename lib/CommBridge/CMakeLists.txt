zephyr_library()
zephyr_library_sources_ifdef(CONFIG_COMM_BRIDGE
        Manager.cpp
)

zephyr_library_link_libraries(rpl)
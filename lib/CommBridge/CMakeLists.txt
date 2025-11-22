zephyr_library()
zephyr_library_sources_ifdef(CONFIG_COMM_BRIDGE
        CommBridge.cpp
)

zephyr_library_link_libraries(rpl)
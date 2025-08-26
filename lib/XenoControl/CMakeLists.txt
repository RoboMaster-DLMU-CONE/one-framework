if(CONFIG_XENO_CONTROL)

zephyr_library()
zephyr_library_sources(Joints.cpp)
zephyr_library_include_directories(${ZEPHYR_BASE}/../one-framework/include)
target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE onemotor)

endif()
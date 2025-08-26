if(CONFIG_XENO_HARDWARE_INTERFACE)

zephyr_library()
zephyr_library_sources(XenoHardwareInterface.cpp)
zephyr_library_include_directories(${ZEPHYR_BASE}/../one-framework/include)

endif()
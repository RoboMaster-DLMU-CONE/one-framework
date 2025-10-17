list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

zephyr_include_directories(boards)

if (CONFIG_ONE_FRAMEWORK)
    include(modules/OF)

    zephyr_syscall_include_directories(include)

    zephyr_include_directories(include)
    zephyr_include_directories(dts)

    add_subdirectory(drivers)
    add_subdirectory(lib)

    zephyr_link_libraries_ifdef(tl::expected)
endif ()
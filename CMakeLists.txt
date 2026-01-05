list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

zephyr_include_directories(boards)

if (CONFIG_ONE_FRAMEWORK)
    include(OF)

    zephyr_syscall_include_directories(include)

    zephyr_include_directories(include)
    zephyr_include_directories(dts)

    add_subdirectory(drivers)
    add_subdirectory(lib)

    zephyr_link_libraries(tl::expected)
    zephyr_link_libraries(frozen::frozen)
    zephyr_link_libraries(unordered_dense::unordered_dense)
    zephyr_link_libraries(mp-units::mp-units)
endif ()
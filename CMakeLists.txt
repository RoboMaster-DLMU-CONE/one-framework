zephyr_syscall_include_directories(include)

zephyr_include_directories(include)
zephyr_include_directories(boards)
zephyr_include_directories(dts)

add_subdirectory(drivers)
add_subdirectory(lib)

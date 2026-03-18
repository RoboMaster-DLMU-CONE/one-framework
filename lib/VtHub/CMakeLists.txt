zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VT_HUB VtHub.cpp)

zephyr_library_compile_options(-fno-freestanding -D_GLIBCXX_HOSTED=1)

zephyr_library_link_libraries(rpl)
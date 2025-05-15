zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UNIT Unit.cpp UnitRegistry.cpp)
zephyr_linker_sources_ifdef(CONFIG_UNIT DATA_SECTIONS linker/sections.ld)
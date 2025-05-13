zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UNIT Unit.cpp UnitRegistry.cpp UnitThreadManager.cpp)
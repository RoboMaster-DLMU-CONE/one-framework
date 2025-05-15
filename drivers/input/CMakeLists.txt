zephyr_library()
zephyr_library_sources_ifdef(CONFIG_INPUT_DBUS input-dbus.c)
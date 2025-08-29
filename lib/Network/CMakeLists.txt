# Copyright (c) 2025. MoonFeather
# SPDX-License-Identifier: BSD-3-Clause

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NETWORK 
    Interface.cpp 
    Netlink.cpp 
    Daemon.cpp 
    CANInterface.cpp 
    VCANInterface.cpp
)
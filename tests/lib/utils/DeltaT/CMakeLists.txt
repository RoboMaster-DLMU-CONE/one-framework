cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OF_lib_utils_DeltaT_test)

target_sources(app PRIVATE src/main.cpp)

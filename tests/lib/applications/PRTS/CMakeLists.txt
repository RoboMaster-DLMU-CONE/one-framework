cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OF_lib_applications_PRTS_test)

target_sources(app PRIVATE src/main.cpp src/Units/Chassis.cpp)

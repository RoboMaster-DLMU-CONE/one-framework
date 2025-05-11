cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(OF_lib_ap_Unit_test)

one_framework_unit_register(TestUnit1)
one_framework_unit_register(TestUnit2)
one_framework_unit_register(ThreadTestUnit)

one_framework_finalize_unit_registration()
target_sources(app PRIVATE src/main.cpp src/Units/TestUnit1.cpp src/Units/TestUnit2.cpp src/Units/ThreadTestUnit.cpp)

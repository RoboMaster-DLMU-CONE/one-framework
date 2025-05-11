
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_UNIT Unit.cpp UnitRegistry.cpp UnitThreadManager.cpp)

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL check_and_update_unit_sources)

function(check_and_update_unit_sources)
    get_property(unit_count GLOBAL PROPERTY OF_REGISTERED_UNIT_COUNT)

    if (unit_count AND NOT unit_count EQUAL 0)
        message(STATUS "Unit: Confirmed Unit sources with ${unit_count} registered units")

    else ()
        zephyr_compile_definitions(OF_TOTAL_REGISTERED_UNITS=0)
        message(FATAL_ERROR "No units registered but got CONFIG_UNIT=y. You can: \n    - Manually disable Unit by using CONFIG_UNIT=n \n    - Check whether a 'one_framework_init()' is in your app's CMakeLists.txt \n    - Check if there's any custom Unit without 'one_framework_unit_register'")
    endif ()
endfunction()
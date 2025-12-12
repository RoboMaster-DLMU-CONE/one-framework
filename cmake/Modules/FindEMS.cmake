find_package(ems QUIET)
if (NOT ems_FOUND)
    FetchContent_Declare(ems
            GIT_REPOSITORY https://gitee.com/moonfeather/ems
            GIT_TAG main
    )
    FetchContent_MakeAvailable(ems)
endif ()
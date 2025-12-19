find_package(unordered_dense CONFIG QUIET)

if (NOT unordered_dense_FOUND)
    FetchContent_Declare(
            unordered_dense
            GIT_REPOSITORY https://gitee.com/dlmu-cone/unordered_dense.git
            GIT_TAG main
            GIT_SHALLOW true
    )
    FetchContent_MakeAvailable(unordered_dense)
endif ()
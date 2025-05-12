# 用法: one_framework_register_unit(MyUnit.hpp)
function(one_framework_register_unit UNIT_HEADER)
    # 获取头文件的绝对路径
    if (NOT IS_ABSOLUTE ${UNIT_HEADER})
        set(UNIT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${UNIT_HEADER}")
    endif ()

    if (NOT EXISTS ${UNIT_HEADER})
        message(FATAL_ERROR "Unit头文件不存在: ${UNIT_HEADER}")
        return()
    endif ()

    # 提取类名和目录
    get_filename_component(UNIT_CLASS_NAME ${UNIT_HEADER} NAME_WE)
    get_filename_component(UNIT_DIR ${UNIT_HEADER} DIRECTORY)

    # 读取头文件内容
    file(READ ${UNIT_HEADER} HEADER_CONTENT)

    # 提取Unit名称
    string(REGEX MATCH "static consteval std::string_view name\\(\\)[^{]*{[^\"]*\"([^\"]*)\"[^}]*}"
            NAME_MATCH "${HEADER_CONTENT}")
    if (CMAKE_MATCH_1)
        set(UNIT_NAME ${CMAKE_MATCH_1})
    else ()
        set(UNIT_NAME ${UNIT_CLASS_NAME})
    endif ()

    # 提取栈大小
    string(REGEX MATCH "static consteval size_t stackSize\\(\\)[^{]*{[^0-9]*([0-9]+)[^}]*}"
            STACK_MATCH "${HEADER_CONTENT}")
    if (CMAKE_MATCH_1)
        set(STACK_SIZE ${CMAKE_MATCH_1})
        # 添加安全余量

        # 累加到全局变量
        math(EXPR NEW_TOTAL "${OF_TOTAL_HEAP_SIZE} + ${STACK_SIZE}")
        set(OF_TOTAL_HEAP_SIZE ${NEW_TOTAL} CACHE INTERNAL "累计的Unit堆内存需求")

        message(STATUS "Unit ${UNIT_NAME}: 栈大小 ${STACK_SIZE} 字节")
    else ()
        message(WARNING "无法从 ${UNIT_HEADER} 提取栈大小")
    endif ()

endfunction()

function(one_framework_finalize_heap_size)
    # 添加基础堆大小
    math(EXPR FINAL_HEAP_SIZE "${OF_TOTAL_HEAP_SIZE} + 1024")

    message(STATUS "设置最终堆大小: ${FINAL_HEAP_SIZE} 字节")

    # 检查是否已经定义了 CONFIG_HEAP_MEM_POOL_SIZE
    execute_process(
            COMMAND ${CMAKE_C_COMPILER} -dM -E -x c /dev/null
            COMMAND grep -q CONFIG_HEAP_MEM_POOL_SIZE
            RESULT_VARIABLE NOT_DEFINED
            OUTPUT_QUIET
            ERROR_QUIET
    )

    if (NOT_DEFINED EQUAL 0)
        # 如果已定义，获取当前值并加上 FINAL_HEAP_SIZE
        execute_process(
                COMMAND ${CMAKE_C_COMPILER} -dM -E -x c /dev/null
                COMMAND grep CONFIG_HEAP_MEM_POOL_SIZE
                OUTPUT_VARIABLE CURRENT_DEF
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX REPLACE "^#define CONFIG_HEAP_MEM_POOL_SIZE ([0-9]+).*$" "\\1" CURRENT_SIZE "${CURRENT_DEF}")
        math(EXPR NEW_SIZE "${CURRENT_SIZE} + ${FINAL_HEAP_SIZE}")
        message(STATUS "增加现有堆大小: ${CURRENT_SIZE} + ${FINAL_HEAP_SIZE} = ${NEW_SIZE} 字节")
        zephyr_compile_definitions(CONFIG_HEAP_MEM_POOL_SIZE=${NEW_SIZE})
    else ()
        # 如果未定义，直接设置
        zephyr_compile_definitions(CONFIG_HEAP_MEM_POOL_SIZE=${FINAL_HEAP_SIZE})
    endif ()
endfunction()
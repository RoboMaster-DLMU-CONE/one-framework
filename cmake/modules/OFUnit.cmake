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
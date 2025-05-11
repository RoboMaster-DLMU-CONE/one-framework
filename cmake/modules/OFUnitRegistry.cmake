# 全局属性来存储已注册 Unit 的数量
set_property(GLOBAL PROPERTY OF_REGISTERED_UNIT_COUNT 0)
set_property(GLOBAL PROPERTY OF_REGISTERED_UNIT_NAMES "")

# CMake 函数，供各个 Unit 的 CMakeLists.txt 调用
function(one_framework_unit_register unit_target_name)
    # 获取当前的计数值
    get_property(current_count GLOBAL PROPERTY OF_REGISTERED_UNIT_COUNT)

    # 增加计数值
    math(EXPR new_count "${current_count} + 1")
    set_property(GLOBAL PROPERTY OF_REGISTERED_UNIT_COUNT ${new_count})

    get_property(current_names GLOBAL PROPERTY OF_REGISTERED_UNIT_NAMES)
    list(APPEND current_names ${unit_target_name})
    set_property(GLOBAL PROPERTY OF_REGISTERED_UNIT_NAMES ${current_names})

    message(STATUS "Registered Unit: ${unit_target_name}. Total registered: ${new_count}")
endfunction()

# CMake 函数，在所有 Unit 注册后，将总数作为编译定义添加到目标
function(one_framework_finalize_unit_registration)
    get_property(total_registered_units GLOBAL PROPERTY OF_REGISTERED_UNIT_COUNT)
    get_property(registered_unit_names GLOBAL PROPERTY OF_REGISTERED_UNIT_NAMES)

    message(STATUS "Finalizing Unit registration. Total Units: ${total_registered_units}")
    message(STATUS "Registered Unit names: ${registered_unit_names}")

    zephyr_compile_definitions(OF_TOTAL_REGISTERED_UNITS=${total_registered_units})

endfunction()
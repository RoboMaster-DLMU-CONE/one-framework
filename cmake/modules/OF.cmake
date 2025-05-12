set(OF_TOTAL_HEAP_SIZE 0 CACHE INTERNAL "累计的堆内存需求")
function(one_framework_init)
    # 添加基础堆大小(可选)
    math(EXPR FINAL_HEAP_SIZE "${OF_TOTAL_HEAP_SIZE} + 1024")

    message(STATUS "设置最终堆大小: ${FINAL_HEAP_SIZE} 字节")

    zephyr_compile_definitions(CONFIG_HEAP_MEM_POOL_SIZE=${FINAL_HEAP_SIZE})

endfunction()
include(${CMAKE_CURRENT_LIST_DIR}/OFUnit.cmake)
message(STATUS "-- One Framework build system activated --")
message(STATUS "One Framework 构建系统已激活")

if(CONFIG_CONTROLLER_CENTER)
include(Modules/FindFrozen)
endif ()
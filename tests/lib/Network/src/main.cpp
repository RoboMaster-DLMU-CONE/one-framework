// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#include <OF/lib/Network/Interface.hpp>
#include <OF/lib/Network/Netlink.hpp>
#include <OF/lib/Network/Daemon.hpp>
#include <OF/lib/Network/CANInterface.hpp>
#include <OF/lib/Network/VCANInterface.hpp>

LOG_MODULE_REGISTER(network_test, LOG_LEVEL_DBG);

using namespace OF::Network;

// 测试 Netlink 类的基本功能
ZTEST(netlink_tests, test_netlink_initialization)
{
    Netlink netlink;
    
    // 测试初始化
    zassert_true(netlink.initialize(), "Netlink应该初始化成功");
    
    // 测试重复初始化
    zassert_true(netlink.initialize(), "重复初始化应该成功");
    
    // 清理
    netlink.close();
}

// 测试 Netlink 的 is_up 功能
ZTEST(netlink_tests, test_netlink_is_up)
{
    Netlink netlink;
    zassert_true(netlink.initialize(), "Netlink应该初始化成功");
    
    // 测试 VCAN 接口状态查询
    bool vcan0_status = netlink.is_up("vcan0");
    LOG_INF("vcan0 status: %s", vcan0_status ? "up" : "down");
    
    // 测试 CAN 接口状态查询
    bool can0_status = netlink.is_up("can0");
    LOG_INF("can0 status: %s", can0_status ? "up" : "down");
    
    // 测试不存在的接口
    bool fake_status = netlink.is_up("fake123");
    zassert_false(fake_status, "不存在的接口应该返回false");
    
    netlink.close();
}

// 测试 Daemon 类的基本功能
ZTEST(daemon_tests, test_daemon_initialization)
{
    Daemon daemon;
    
    // 测试初始化
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    // 测试重复初始化
    zassert_true(daemon.initialize(), "重复初始化应该成功");
    
    // 清理
    daemon.shutdown();
}

// 测试 Daemon 的接口管理功能
ZTEST(daemon_tests, test_daemon_interface_management)
{
    Daemon daemon;
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    const char* test_interface = "vcan_test0";
    
    // 测试创建 VCAN 接口
    zassert_true(daemon.createVCANInterface(test_interface), 
                 "创建VCAN接口应该成功");
    
    // 测试接口存在性检查
    zassert_true(daemon.interfaceExists(test_interface), 
                 "创建的接口应该存在");
    
    // 测试接口类型解析
    std::string type = daemon.parseInterfaceType(test_interface);
    zassert_true(type == "vcan", "接口类型应该是vcan");
    
    // 测试删除接口
    zassert_true(daemon.deleteInterface(test_interface), 
                 "删除接口应该成功");
    
    daemon.shutdown();
}

// 测试 VCANInterface 类
ZTEST(vcan_interface_tests, test_vcan_interface_creation)
{
    Daemon daemon;
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    // 测试创建 VCAN 接口
    {
        VCANInterface vcan_if("vcan_test1", daemon);
        
        // 测试接口名称
        zassert_true(vcan_if.getName() == "vcan_test1", 
                     "接口名称应该正确");
        
        // 测试接口状态
        bool status = vcan_if.is_up();
        LOG_INF("VCAN interface status: %s", status ? "up" : "down");
        
        // 测试启用接口
        zassert_true(vcan_if.bring_up(), "启用接口应该成功");
        
        // 测试是否由此实例创建
        zassert_true(vcan_if.is_created_by_this_instance(), 
                     "接口应该由此实例创建");
    }
    // VCANInterface 析构函数应该自动清理接口
    
    daemon.shutdown();
}

// 测试 CANInterface 类（应该抛出异常，因为接口不存在）
ZTEST(can_interface_tests, test_can_interface_not_found)
{
    Daemon daemon;
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    // 测试创建不存在的 CAN 接口应该抛出异常
    bool exception_thrown = false;
    try {
        CANInterface can_if("nonexistent_can", daemon);
    } catch (const CANInterfaceNotFoundException& e) {
        exception_thrown = true;
        LOG_INF("Caught expected exception: %s", e.what());
    }
    
    zassert_true(exception_thrown, "应该抛出CANInterfaceNotFoundException");
    
    daemon.shutdown();
}

// 测试 CANInterface 类（使用存在的接口）
ZTEST(can_interface_tests, test_can_interface_existing)
{
    Daemon daemon;
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    // 测试创建存在的 CAN 接口（can0 在模拟中存在）
    bool creation_successful = false;
    try {
        CANInterface can_if("can0", daemon);
        creation_successful = true;
        
        // 测试接口名称
        zassert_true(can_if.getName() == "can0", 
                     "接口名称应该正确");
        
        // 测试接口状态
        bool status = can_if.is_up();
        LOG_INF("CAN interface status: %s", status ? "up" : "down");
        
        // 测试启用/禁用接口
        zassert_true(can_if.bring_up(), "启用接口应该成功");
        zassert_true(can_if.bring_down(), "禁用接口应该成功");
        
    } catch (const CANInterfaceNotFoundException& e) {
        LOG_ERR("Unexpected exception: %s", e.what());
    }
    
    zassert_true(creation_successful, "创建存在的CAN接口应该成功");
    
    daemon.shutdown();
}

// 测试接口配置解析
ZTEST(daemon_tests, test_interface_config_parsing)
{
    Daemon daemon;
    zassert_true(daemon.initialize(), "Daemon应该初始化成功");
    
    // 测试配置解析
    bool result = daemon.parseInterfaceConfig("can0", "bitrate 500000");
    zassert_true(result, "配置解析应该成功");
    
    daemon.shutdown();
}

static void* common_test_setup(void)
{
    LOG_INF("Starting Network module tests...");
    k_sleep(K_MSEC(100)); // 短暂延迟让日志输出
    return nullptr;
}

ZTEST_SUITE(netlink_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(daemon_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(vcan_interface_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
ZTEST_SUITE(can_interface_tests, nullptr, common_test_setup, nullptr, nullptr, nullptr);
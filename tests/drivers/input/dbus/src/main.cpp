// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <OF/drivers/utils/status_leds.h>

// 注册日志模块
LOG_MODULE_REGISTER(dbus_test, CONFIG_LOG_DEFAULT_LEVEL);

// 输入事件回调函数
static void input_callback(input_event* evt, void* user_data)
{
    ARG_UNUSED(user_data);
    // 针对不同类型的输入事件打印不同的日志
    switch (evt->type)
    {
    case INPUT_EV_ABS:
        LOG_INF("摇杆或滚轮: code=%u, value=%d", evt->code, evt->value);
        break;
    case INPUT_EV_REL:
        LOG_INF("鼠标移动: code=%u, value=%d", evt->code, evt->value);
        break;
    case INPUT_EV_KEY:
        LOG_INF("按键或开关: code=%u, state=%s", evt->code, evt->value ? "按下" : "释放");
        break;
    case INPUT_EV_MSC:
        LOG_INF("其他事件: code=%u, value=%d", evt->code, evt->value);
        break;
    default:
        LOG_INF("未知事件: type=%u, code=%u, value=%d", evt->type, evt->code, evt->value);
        break;
    }
}

int main()
{
    LOG_INF("启动设备");

    const struct device* status_led_dev = DEVICE_DT_GET(DT_NODELABEL(status_leds));
    if (!device_is_ready(status_led_dev))
    {
        LOG_ERR("状态LED设备未就绪\n");
        return -1;
    }
    const auto led_api = static_cast<const status_leds_api*>(status_led_dev->api);

    led_api->set_heartbeat(status_led_dev);

    // 获取DBUS设备
    const device* dbus_dev = DEVICE_DT_GET_ANY(dji_dbus);

    if (!device_is_ready(dbus_dev))
    {
        LOG_ERR("DBUS设备未就绪，检查硬件连接和配置");
        return -1;
    }

    LOG_INF("DBUS测试已启动");
    LOG_INF("等待遥控器数据...");

    // 注册输入回调函数
    INPUT_CALLBACK_DEFINE(dbus_dev, input_callback, nullptr);

    // 主循环等待数据
    while (true)
    {
        k_sleep(K_SECONDS(10));
        LOG_INF("系统运行中，如有数据会显示在日志中");
    }
}

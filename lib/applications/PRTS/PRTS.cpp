#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(prts, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_PRTS

// PRTS线程配置
#define PRTS_STACK_SIZE 4096
#define PRTS_PRIORITY 5
K_THREAD_STACK_DEFINE(prts_stack, PRTS_STACK_SIZE);
static k_thread prts_thread_data;

// PRTS线程函数
[[noreturn]] static void prts_thread_fn(void* arg1, void* arg2, void* arg3)
{
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  LOG_INF("PRTS线程已启动");

  // 在屏幕中央创建一个"Hello World"标签
  lv_obj_t* label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello World");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_timer_handler();

  // 主循环
  while (true)
  {
    // 在实际应用中，这里会:
    // 1. 收集各模块状态信息
    // 2. 处理日志数据
    // 3. 更新UI显示
    lv_timer_handler(); // 处理LVGL任务
    k_sleep(K_MSEC(10)); // 适当的时间间隔
  }
}

// 初始化PRTS模块
static int prts_init(void)
{
  LOG_INF("初始化PRTS模块");
  const device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("显示设备未就绪");
    return -ENODEV;
  }
  display_blanking_off(display_dev);


  // 创建PRTS线程
  const k_tid_t tid = k_thread_create(&prts_thread_data, prts_stack, PRTS_STACK_SIZE,
                                      prts_thread_fn, nullptr, nullptr, nullptr, PRTS_PRIORITY,
                                      0, K_NO_WAIT);

  if (tid == nullptr)
  {
    LOG_ERR("PRTS线程创建失败");
    return -ENOMEM;
  }

  k_thread_name_set(tid, "prts");
  LOG_INF("PRTS模块初始化成功");

  return 0;
}

SYS_INIT(prts_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_PRTS */

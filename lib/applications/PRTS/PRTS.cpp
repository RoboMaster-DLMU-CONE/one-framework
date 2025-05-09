#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(prts, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_PRTS

// PRTS线程配置
#define PRTS_STACK_SIZE 2048
#define PRTS_PRIORITY 5
K_THREAD_STACK_DEFINE(prts_stack, PRTS_STACK_SIZE);
static struct k_thread prts_thread_data;

// 用于LVGL操作的互斥锁
K_MUTEX_DEFINE(lvgl_mutex);

// PRTS线程函数
static void prts_thread_fn(void *arg1, void *arg2, void *arg3) {
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);

  LOG_INF("PRTS线程已启动");

  // 创建LVGL界面元素 - 使用互斥锁确保线程安全
  k_mutex_lock(&lvgl_mutex, K_FOREVER);

  // 在屏幕中央创建一个"Hello World"标签
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello World");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  k_mutex_unlock(&lvgl_mutex);

  // 主循环
  while (1) {
    // 在实际应用中，这里会:
    // 1. 收集各模块状态信息
    // 2. 处理日志数据
    // 3. 更新UI显示
    k_sleep(K_MSEC(1000));
  }
}

// 初始化PRTS模块
static int prts_init(void) {
  LOG_INF("初始化PRTS模块");

  // 创建PRTS线程
  k_tid_t tid = k_thread_create(&prts_thread_data, prts_stack, PRTS_STACK_SIZE,
                                prts_thread_fn, NULL, NULL, NULL, PRTS_PRIORITY,
                                0, K_NO_WAIT);

  if (tid == NULL) {
    LOG_ERR("PRTS线程创建失败");
    return -ENOMEM;
  }

  k_thread_name_set(tid, "prts");
  LOG_INF("PRTS模块初始化成功");

  return 0;
}

SYS_INIT(prts_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_PRTS */
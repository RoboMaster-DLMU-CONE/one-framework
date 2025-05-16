#ifndef PID_C_API_H
#define PID_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

// PID类型
typedef enum
{
    PID_POSITIONAL,
    PID_INCREMENTAL
} pid_type_t;

// PID特性标志位
#define PID_FEATURE_NONE                 0x00
#define PID_FEATURE_DEADBAND             0x01
#define PID_FEATURE_INTEGRAL_LIMIT       0x02
#define PID_FEATURE_OUTPUT_LIMIT         0x04
#define PID_FEATURE_OUTPUT_FILTER        0x08
#define PID_FEATURE_DERIVATIVE_FILTER    0x10

// 不透明指针类型
typedef struct pid_controller_s* pid_controller_t;

// 创建PID控制器
pid_controller_t pid_create(pid_type_t type, float kp, float ki, float kd,
                            unsigned int features,
                            float max_output, float deadband, float integral_limit);

// 计算PID输出
float pid_compute(pid_controller_t pid, float reference, float measurement);

// 重置PID控制器
void pid_reset(pid_controller_t pid);

// 释放PID控制器
void pid_destroy(pid_controller_t pid);

#ifdef __cplusplus
}
#endif

#endif // PID_C_API_H

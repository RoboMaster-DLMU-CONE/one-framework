# XenoControl 机械臂控制库

## 概述

XenoControl 是为机械臂控制设计的库，提供了对7自由度机械臂关节的统一控制接口。该库基于 OneMotor 框架构建，使用 M3508 电机进行精确的位置和速度控制。

## 特性

- **7关节支持**: 支持 Lift（升降）、Stretch（伸缩）、Shift（平移）、Arm1-3（机械臂关节）、Suck（吸盘）
- **双重控制模式**: 支持位置控制和速度控制
- **标定系统**: 为每个关节提供独立的速度和位置标定值
- **线程安全**: 使用 SpinLock 保证多线程环境下的数据安全
- **实时反馈**: 实时读取电机状态并更新关节数据

## 库结构

### XenoControl::Joints 类

核心关节控制类，负责管理7个关节的控制和状态监测。

#### 关节映射

```cpp
enum class JointType : uint8_t
{
    LIFT = 0,    // 升降关节 (电机ID: 1)
    STRETCH = 1, // 伸缩关节 (电机ID: 2)
    SHIFT = 2,   // 平移关节 (电机ID: 3)
    ARM1 = 3,    // 机械臂关节1 (电机ID: 4)
    ARM2 = 4,    // 机械臂关节2 (电机ID: 5)
    ARM3 = 5,    // 机械臂关节3 (电机ID: 6)
    SUCK = 6     // 吸盘关节 (电机ID: 7)
};
```

#### 数据结构

```cpp
struct JointData
{
    float velocity_reading = 0.0f;      // 速度读取值 (度/秒)
    float velocity_calibration = 1.0f;  // 速度标定值 (倍数)
    float position_reading = 0.0f;      // 位置读取值 (度)
    float position_calibration = 0.0f;  // 位置标定值 (度偏移)
};
```

#### 主要方法

```cpp
// 获取方法
float getVelocityReading(JointType joint) const;
float getVelocityCalibration(JointType joint) const;
float getPositionReading(JointType joint) const;
float getPositionCalibration(JointType joint) const;

// 设置方法
void setVelocityCalibration(JointType joint, float calibration);
void setPositionCalibration(JointType joint, float calibration);
void setTargetVelocity(JointType joint, float velocity);
void setTargetPosition(JointType joint, float position);

// 控制方法
void enableJoint(JointType joint);
void disableJoint(JointType joint);
void updateReadings();
```

### XenoHardwareInterface 类

高级硬件接口类，提供更便捷的机械臂控制接口。

#### 主要方法

```cpp
// 初始化和控制
bool initialize();
void enableAllJoints();
void disableAllJoints();

// 位置控制
void setLiftPosition(float position);
void setStretchPosition(float position);
void setShiftPosition(float position);
void setArmPositions(float arm1_pos, float arm2_pos, float arm3_pos);
void setSuckPosition(float position);

// 状态读取
float getLiftPosition() const;
float getStretchPosition() const;
float getShiftPosition() const;
void getArmPositions(float& arm1_pos, float& arm2_pos, float& arm3_pos) const;
float getSuckPosition() const;

// 传感器更新
void updateSensorReadings();
```

## 使用方法

### 1. 配置依赖

在 `prj.conf` 中启用必要的配置：

```
CONFIG_ONE_FRAMEWORK=y
CONFIG_ONEMOTOR=y
CONFIG_XENO_CONTROL=y
CONFIG_XENO_HARDWARE_INTERFACE=y
```

### 2. 基本使用示例

```cpp
#include <OF/lib/XenoHardwareInterface/XenoHardwareInterface.hpp>
#include <OneMotor/Can/CanDriver.hpp>

void control_robot_arm()
{
    // 初始化 CAN 驱动
    const device* can_dev = DEVICE_DT_GET(DT_CHOSEN(can1));
    OneMotor::Can::CanDriver can_driver(can_dev);
    
    auto result = can_driver.open();
    if (!result) {
        // 处理CAN初始化错误
        return;
    }
    
    // 创建硬件接口
    OF::XenoHardwareInterface::XenoHardwareInterface hardware_interface(can_driver);
    
    // 初始化硬件接口
    if (!hardware_interface.initialize()) {
        // 处理初始化错误
        return;
    }
    
    // 获取关节控制器并设置标定值
    auto& joints = hardware_interface.getJoints();
    joints.setVelocityCalibration(OF::XenoControl::JointType::LIFT, 1.2f);
    joints.setPositionCalibration(OF::XenoControl::JointType::LIFT, 0.0f);
    
    // 启用所有关节
    hardware_interface.enableAllJoints();
    
    // 控制机械臂运动
    hardware_interface.setLiftPosition(45.0f);
    hardware_interface.setStretchPosition(90.0f);
    hardware_interface.setArmPositions(30.0f, 60.0f, 90.0f);
    
    // 控制循环
    while (running) {
        // 更新传感器读数
        hardware_interface.updateSensorReadings();
        
        // 读取当前状态
        float current_lift = hardware_interface.getLiftPosition();
        
        // 根据状态进行控制决策
        // ...
        
        k_sleep(K_MSEC(10));
    }
    
    // 停止所有关节
    hardware_interface.disableAllJoints();
}
```

### 3. 标定系统

标定系统允许对每个关节进行微调：

```cpp
// 速度标定：实际输出 = 目标速度 × 标定倍数
joints.setVelocityCalibration(JointType::LIFT, 1.2f);  // 增加20%速度

// 位置标定：实际输出 = 目标位置 + 标定偏移
joints.setPositionCalibration(JointType::STRETCH, 5.0f);  // 增加5度偏移
```

## 配置选项

### Kconfig 选项

```kconfig
CONFIG_XENO_CONTROL=y                    # 启用 XenoControl 库
CONFIG_XENO_HARDWARE_INTERFACE=y         # 启用硬件接口
CONFIG_XENO_CONTROL_LOG_LEVEL=3          # 设置日志级别 (0-4)
CONFIG_XENO_HARDWARE_INTERFACE_LOG_LEVEL=3  # 设置硬件接口日志级别
```

### OneMotor 配置

```kconfig
CONFIG_ONEMOTOR=y                        # 启用 OneMotor 库
CONFIG_OM_CAN_MAX_DJI_MOTOR=8           # 设置最大电机数量
CONFIG_ONEMOTOR_LOG_LEVEL=3             # OneMotor 日志级别
```

## 线程安全

该库在设计时考虑了多线程环境：

- 所有读写操作都使用 `OneMotor::Util::SpinLock` 保护
- 可以安全地在不同线程中同时读取和写入关节数据
- 建议在专用控制线程中调用 `updateReadings()`

## 错误处理

库提供了基本的错误检查：

- 无效的关节索引会被自动忽略
- 电机通信错误会通过 OneMotor 的错误系统报告
- 初始化失败会返回相应的错误码

## 性能考虑

- 使用 SpinLock 而非互斥锁，适合短时间的数据访问
- 关节状态更新建议控制在 10-50Hz 频率
- 电机控制命令可以更高频率发送（100-1000Hz）

## 依赖项

- OneFramework 核心库
- OneMotor 电机控制库
- Zephyr RTOS
- C++20 标准库支持

## 注意事项

1. 使用前必须正确初始化 CAN 总线
2. 电机ID必须与物理连接对应（1-7）
3. 标定值需要根据实际机械结构调整
4. 建议在实际部署前进行充分的安全测试
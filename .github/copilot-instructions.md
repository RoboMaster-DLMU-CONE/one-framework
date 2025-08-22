# One Framework - Copilot 使用指南

## 项目概述

**One Framework** 是由大连民族大学 C·One 团队开发的机甲大师嵌入式框架。这是一个基于 Zephyr RTOS 构建的"一键式"
解决方案，为机器人竞赛提供硬件抽象、模块化架构和测试驱动开发。

**核心技术:**

- **编程语言**: C++20 配合 Zephyr C API
- **实时操作系统**: Zephyr (实时操作系统)
- **构建系统**: West (Zephyr 元工具) + CMake
- **测试框架**: ZTest 框架配合 QEMU 仿真
- **硬件平台**: 基于 STM32 的开发板 (DJI Board C: ARM Cortex, 192KB RAM, 1MB Flash)
- **依赖库**: tl::expected 库, OneMotor 模块

**项目规模**: 约31个 C++/头文件，模块化架构包含应用程序、驱动程序和实用工具。

## 构建说明

**重要**: 参考 `.github/workflows/build.yml` 工作流进行配置。

### 构建命令

```bash
# 导航到框架目录
cd one-framework

# 为 DJI Board C 构建步兵配置
west build -b dji_board_c app -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay

# 构建调试配置 (添加 debug.conf)
west build -b dji_board_c app -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay -DOVERLAY_CONFIG=debug.conf

# 清理构建 (如需要)
west build -t clean
```

### 测试执行

```bash
# 使用 Twister 运行所有测试
west twister -T tests -v --inline-logs --integration

# 运行特定测试
west twister -T tests/lib/Unit -v

# 在模拟器上运行测试 (更快)
west twister -T tests --platform qemu_cortex_m3 -v

# 构建应用测试 (验证构建配置)
west twister -T app -v --inline-logs --integration
```

**集成平台**:

- 主要平台: `qemu_cortex_m3` (用于单元测试)
- 硬件平台: `dji_board_c` (用于集成测试)

**重要提示**: 测试在 `qemu_cortex_m3` 虚拟平台上运行，需要约10秒的设置时间。

## 代码库结构

### 架构概览

```
one-framework/
├── app/                    # 主应用程序入口点
│   ├── src/main.cpp       # 应用程序主函数
│   ├── prj.conf          # 项目配置
│   └── boards/cone/       # 开发板特定覆盖配置
├── lib/                   # 框架核心库
│   ├── Unit/              # 单元系统 (线程、生命周期)
│   ├── PRTS/              # 打印/调试系统
│   └── ControllerCenter/  # 控制器中心模块
├── drivers/              # 硬件驱动
│   ├── input/           # 输入驱动 (DBUS 等)
│   ├── output/          # 输出驱动 (蜂鸣器、电机)
│   ├── sensor/          # 传感器驱动 (PWM 加热器等)
│   └── utils/           # 实用驱动 (状态 LED)
├── tests/               # 单元测试 (镜像 lib/ 结构)
├── include/OF/          # 公共头文件
├── boards/              # 开发板定义
├── dts/                # 设备树包含文件
└── .github/workflows/   # CI/CD 管道
```

### 关键配置文件

- **west.yml**: West 清单文件，定义依赖项 (Zephyr, OneMotor)
- **Kconfig**: 框架配置选项
- **prj.conf**: 项目级 Zephyr 配置
- **CMakeLists.txt**: 构建配置
- **.clang-format**: 代码格式化 (基于 LLVM，120 字符限制)

### 关键依赖项

- **设备树系统**: 硬件在 `.dts` 和 `.overlay` 文件中描述
    - `app/boards/cone/infantry.overlay`: 可供参考的步兵机器人配置
    - `boards/dji/dji_board_c/`: DJI Board C 定义 (CAN, PWM, GPIO, USART 支持)
- **Kconfig 系统**: 通过 `CONFIG_*` 选项进行功能选择
    - `CONFIG_ONE_FRAMEWORK=y`: 启用框架核心
    - `CONFIG_UNIT=y`, `CONFIG_PRTS=y`: 应用模块
- **West 工作空间**: 必须初始化为 Zephyr 工作空间，而非独立的 git 仓库

## 验证步骤

### 提交前检查

CI 管道运行这些步骤 - 本地复现:

1. **构建测试**: `west twister -T app -v --inline-logs --integration`
2. **单元测试**: `west twister -T tests -v --inline-logs --integration`
3. **测试平台**: 在 Ubuntu 上运行测试即可

### 代码质量

- **格式化**: 使用 `.clang-format` (LLVM 风格，4空格缩进)
- **编码标准**: 启用 C++20 特性，鼓励 constexpr 优化
- **测试**: 为新功能编写 ZTest 单元测试
- **版权信息**: 在新增的代码顶部补充以下版权信息：

```cpp
// Copyright (c) 2025. MoonFeather
// SPDX-License-Identifier: BSD-3-Clause
```

### 常见问题

- **找不到 West**: 运行任何命令前确保已激活 Zephyr 环境
- **构建超时**: 对构建命令使用 `timeout: 300`，测试可能需要 5-10 分钟
- **缺少依赖**: 如果缺少 OneMotor 或 Zephyr 模块，运行 `west update`
- **测试失败**: 检查 `qemu_cortex_m3` 平台可用性，确保 CONFIG_ZTEST=y
- **错误目录**: 命令必须在 west 工作空间内的 `one-framework/` 目录中运行
- **设备树错误**: 验证 `.overlay` 文件引用正确的开发板外设

## 框架概念

### 单元系统

- **Unit.hpp**: 线程化应用组件的基类
- **UnitRegistry.hpp**: 全局单元管理和生命周期
- **测试**: `tests/lib/Unit/` 演示用法

### 设备抽象

- **基于 DTS**: 通过设备树进行硬件配置
- **Zephyr 驱动**: 标准驱动 API (device_is_ready, sensor_sample_get)

### 配置理念

- **模块化**: 使用 `CONFIG_*` 标志启用/禁用功能
- **开发板特定**: 在开发板文件中覆盖配置
- **测试驱动**: 先编写测试，再实现最小代码

相信这些指南能够实现高效开发。只有当指南不完整或过时时才寻找其他信息。
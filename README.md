## 项目简介

> RoboMaster嵌入式框架“一键”解决方案，为你的“创意”服务。

这是大连民族大学C·One战队为RoboMaster比赛开发的嵌入式框架`one-framework`。

## 使用场景

在使用传统的电控嵌入式框架时，我们很容易遇到这样的问题：

- 换块开发板，就要重新移植一遍所有的代码
- 队伍中同时存在多套不一样的框架，维护成本爆炸
- 要加减硬件需要手动注释或加入代码，导致很多抽象问题
- 调试困难，裸机串口容易抽风，RTT又需要JLink
- FreeRTOS乃至定时器难以满足越来越多的模块间通信需求
- 代码改后压根不知道会不会对其它部分造成影响，导致抽象问题

通过在OneFramework中集成Zephyr框架，我们不仅能解决上述痛点，还可以进一步发展嵌入式框架，促进各种新功能、新技术的诞生。

## 快速开始

### 前置环境

- 能够正常连接Github的网络环境；
- Zephyr工具链。[安装教程](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)

### 克隆代码

```shell
# 先激活west环境，比如：
# cd && .\zephyrproject\.venv\Scripts\Activate.ps1
west init -m https://github.com/RoboMaster-DLMU-CONE/one-framework --mr main zephyr_workspace
# workspace名称是任意的
cd zephyr_workspace
west update
```

### 命令行构建

```shell
# 确保根目录在one_framework
# 为C·One步兵构建
west build -b dji_board_c app -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay
```

### 烧录与调试

```shell
# 烧录
west flash
## 使用 --openocd 指定用 OpenOCD + DAP-Link 烧录
# 调试
west debug
```

### 开发指南

要了解如何使用合适的方法构建、使用、开发此框架，请参阅[
Wiki](https://robomaster-dlmu-cone.github.io/one-framework/develop.html)

## 文档

使用 `Doxygen` 自动生成文档到 `Github Page`。在 `doc`目录下可以找到所有文档 。

## Todo

- [ ] PRTS界面设计
- [ ] 电机drivers
- [ ] Buzzer
- [x] application基类
- [x] CPU负载计数器

## Credit

- 感谢 [basic_framework](https://github.com/HNUYueLuRM/basic_framework) 开源的大量 `RoboMaster` 相关代码。
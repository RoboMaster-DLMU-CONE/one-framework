## 项目简介
TBD
## 使用场景
TBD
## 快速开始
### 前置环境
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

### CLion构建
#### 创建工具链
如果这是你第一次使用CLion打开Zephyr工程，请先为Zephyr创建一个工具链。步骤如下：
- 进入CLion设置菜单，找到`构建、执行、部署`->`工具链`
- 在窗口左侧选择`添加`，点击添加的新工具链的`添加环境`链接，选择`来自文件`的环境。文件选择`%HOMEPATH%\zephyrproject\.venv\Scripts\Activate.ps1`。其中，`zephyrproject`是你在安装zephyr工具链时创建的虚拟环境的绝对路径。
- 修改新工具链的名称，比如`zephyr`，然后点击右下角的`确定`进行保存。
#### 使用CLion打开Zephyr工作目录
1. 进入CLion,打开`zephyr_workspace/one-framework/app`路径。其中，`zephyr_workspace`是克隆过程中新建的工作区。
2. 在CLion下方`West`窗口中点击小齿轮`West设置`
3. 工具链选择你在CLion中创建的zephyr工具链，构建选项填入`-b dji_board_c -- -DDTC_OVERLAY_FILE=boards/cone/infantry.overlay`。
4. 点击确定，CLion会自动为你生成索引和相关文件。

### 烧录与调试
```shell
# 烧录
west flash
# 调试
west debug
```
> `One Framework`为开发板配置了`OpenOCD`和`JLink`两种烧录选项。其中，`OpenOCD`用于兼容`Dap-Link`和其它下载器，`JLink`用于兼容`JLink`下载器。使用`--runner`参数指定`openocd`或`jlink`即可更改。
## Wiki
TBD
## Todo
TBD
## Credit
TBD
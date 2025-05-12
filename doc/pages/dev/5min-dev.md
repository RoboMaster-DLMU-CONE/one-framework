\page 5min-dev 五分钟内移植框架
\tableofcontents

## 概述

这篇文章将演示如何快速为你自己的开发板移植 `One Framework` 。除去克隆工程所需的时间外，我们还需要：

+ 修改各外设的引脚配置 `（约需2分钟）`
+ 启用/禁用需要在框架中运行的任务 `（约需2分钟）`
+ 编译并烧录代码 `（约需1分钟）`

\remark 本开发指南使用 `CLion`作为开发Zephyr应用程序的IDE。你可以参考下面两篇文章进行CLion配置：
- [CLion下载与安装](https://conestudio.yuque.com/rpzak7/qd5o3y/gkt4gmk2trtg3tf0?singleDoc)
- [CLion开发Zephyr应用程序](https://conestudio.yuque.com/rpzak7/qd5o3y/owlpkde5xwhhbgn5?singleDoc)


\page develop 开发指南
\tableofcontents

## 开发指南概述

这是 `One Framework` 的开发指南。

`One Framework` 采用`Zephyr`作为其实时任务系统与硬件抽象层(HAL)框架。您可以参阅 \ref framework "框架设计" 了解更多详细内容。

对于大多数嵌入式开发初学者来说，使用Zephyr与CMake进行嵌入式开发既是一次新奇的体验，也是一次具有挑战性的旅程。本开发指南从如何快速为开发板移植框架入手，逐步深入介绍
``One Framework`` 的系统设计和构建系统，同时穿插讲解Zephyr系统的各项关键知识，力求深入浅出。希望这份指南能帮助你更快更好地完成比赛项目，并成长为更优秀的嵌入式工程师！

\remark 在本篇开发指南中，若涉及到与框架无直接联系的前置知识，我们会使用此提示块将你引导至外部网站。

\subpage 5min-dev
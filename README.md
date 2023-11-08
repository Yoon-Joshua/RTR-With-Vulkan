# Real-Time Rendering Demo with Vulkan

本项目基于中文课程GAMES 202。该课程为其课后作业提供了WebGL的框架，使学生专注于着色器的工作。我为了练习该课程的知识以及Vulkan编程，自己用Vulkan重新搭建框架，实现课程作业中的渲染效果。

GAMES 202 的作业任务有

* Percentage Closer Soft Shadow
* Precomputed Radiance Transfer
* Kulla-Conty Approximation
* Screen Space Global Illumination
* Ray Tracing and Denoising

目前已完成前4个的基本任务。第5个任务需要具备硬件光追功能的显卡，其渲染流程和一般的 Vulkan 的光栅化渲染流程不太一样，待日后完善。

前4个任务中还包含一些“加分项” ，其中环境光的旋转、屏幕空间求交的加速有一定难度，所以我没有做。如果未来有时间，会做。

现在，程序还缺少交互功能，需要开发具有轨迹球或者弧形球功能的Camera类。

## 演示

![](img/pcss.png)

![](img/prt.png)

![](img/ssr.png)

![](img/kully-conty.png)

## 依赖

* GLM
* GLFW3
* Vulkan SDK
* tiny_obj_loader


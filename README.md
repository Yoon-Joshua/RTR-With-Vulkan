# Real-Time Rendering Demo with Vulkan

任务有

* Percentage Closer Soft Shadow
* Precomputed Radiance Transfer
* Kulla-Conty Approximation
* Screen Space Global Illumination
* Ray Tracing and Denoising

目前已完成前4个的基本任务。第5个任务需要具备硬件光追功能的显卡，其渲染流程和一般的 Vulkan 的光栅化渲染流程不太一样，待日后完善。

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


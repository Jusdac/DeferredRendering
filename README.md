# DeferredRendering
DX12

<img width="800" height="625" alt="image" src="https://github.com/user-attachments/assets/7da9117e-a0f0-4b3a-86d2-6021b26c0e7c" />

第一次更新：First Update in 1.3/2026

1：现支持MRT技术 supporting MRT now!

2：剔除原来冗余的shader   remove redundant shaders

本次更新对延迟渲染管线中的 G-Buffer 渲染阶段 进行了关键性优化，通过引入 多重渲染目标（Multiple Render Targets, MRT） 技术，将原本需要 4 次独立 DrawCall 的 G-Buffer 写入流程，合并为 单次 DrawCall，大幅减少了 GPU 状态切换开销。

我使用 MSI Afterburner 对比了优化前后的性能表现。以下是关键指标对比：

指标	     原始版本（无 MRT）	  优化后（启用 MRT）	      提升

GPU使用率	   平均 ~37%	          平均 ~45%	           +21.6%

显存占用	     最高 1273 MB	        最高 1273 MB	        相同

核心频率	     峰值 ~1234 MHz	     峰值 ~1224 MHz	       几乎持平

显存频率	     峰值 ~8501 MHz	     峰值 ~8501 MHz	        几乎持平

 优化成果总结
 
✅ 减少 3 次 OMSetRenderTargets 调用，降低 CPU 开销。

✅ 避免多次 ResourceBarrier，提升 GPU 执行流畅度。

✅ G-Buffer 写入效率提升约 20%，为后续光照计算提供更稳定输入。

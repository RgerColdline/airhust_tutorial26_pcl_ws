# Airhust 26级梯队任务 之 PCL

本仓库用于阶段三 PCL 考核。各节说明提供基础接口和示例，需要独立完成的部分标在源码的 `TODO` 中。

## 目录

- **README ←**
  - [阶段三：先学会用人家的函数](docs/index.md)
    - [3.1 点云在内存里怎么存](docs/01-io.md)
    - [3.2 用 rviz 看](docs/02-rviz.md)
    - [3.3 滤波](docs/03-filter.md)
    - [3.4 找邻居](docs/04-kdtree.md)
    - [3.5 聚簇](docs/05-cluster.md)
    - [3.6 点云哪来的](docs/06-cloud-register.md)
    - [3.7 进仿真](docs/07-sim.md)
    - [3.8 扩展](docs/08-extend.md)

## 如何预览md？

**如果你已经会预览了请忽略本节**

md &rarr; markdown
上b站应该有教程

一般是使用代码编辑器，笔记软件或者是直接用某些预览md的网页打开。但是这种md一般在代码仓库里面，github可以直接预览，也可以从代码编辑器（如vscode）安装相关插件预览

## 什么是PCL？

- ~~Plain Craft Launcher~~
- **Point Cloud Library**
  翻译：**点云处理库**
  官网：[https://pointclouds.org此处可以点击](https://pointclouds.org)

    官网里面有对其解释，大家可以用浏览器的翻译插件配合观看学习

![](https://pointclouds.org/assets/images/about/pcl_dependency_graph2.png)
上图是模块架构图，但是似乎很久没有更新了，我们没有完全必要详细学习，只是个参考，让大家认识模块不是整齐的树或者是链，或者是平行的东西，而是一个有向无环图（超纲了）

和cpp标准库一样，他同样提供了很多cpp的接口函数供你调用，用来处理点云，而不需要你实现其内部细节

**下一步请来到[可点击的这个文件](./docs/index.md)来学习如何使用PCL**

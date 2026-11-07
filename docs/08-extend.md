<details>
<summary>目录</summary>

- [README](../README.md)
  - [阶段三：先学会用人家的函数](index.md)
    - [3.1 点云在内存里怎么存](01-io.md)
    - [3.2 用 rviz 看](02-rviz.md)
    - [3.3 滤波](03-filter.md)
    - [3.4 找邻居](04-kdtree.md)
    - [3.5 聚簇](05-cluster.md)
    - [3.6 点云哪来的](06-cloud-register.md)
    - [3.7 进仿真](07-sim.md)
    - **3.8 扩展 ←**

</details>

[← 3.7 进仿真](07-sim.md)　·　[目录](index.md)

# 3.8 扩展

下面各小节介绍不同的点云处理方向，可以按兴趣选择阅读。

## 3.8.1 法线

法线是每个点所在局部表面的朝向：取一小片邻居点拟合平面，垂线方向就是法线。3.4 的邻域搜索就是为它服务的。

算完有两种存法。只用 `pcl::Normal`（只有 `normal_x/y/z` 和 `curvature`）：

```cpp
pcl::PointCloud<pcl::Normal> normals;
pcl::NormalEstimationOMP<pcl::PointXYZI, pcl::Normal> ne;
ne.setInputCloud(cloud);
ne.setKSearch(15);
ne.compute(normals);
```

和坐标一起存，用 `pcl::PointXYZINormal`：先放坐标和强度，估计器只写法线字段：

```cpp
pcl::PointCloud<pcl::PointXYZINormal> normals;
pcl::copyPointCloud(*cloud, normals);
pcl::NormalEstimationOMP<pcl::PointXYZI, pcl::PointXYZINormal> ne;
ne.setInputCloud(cloud);
ne.setKSearch(15);
ne.compute(normals);
```

法线的正负没有唯一答案，PCL 默认让它朝向点云记录的传感器位置（PCD 的 `VIEWPOINT`）；要指定观察点用 `setViewPoint(x, y, z)`。

什么时候值得算：

- 按朝向筛点，比如用 `|n_z|` 判断水平面、用水平分量判断墙面；
- 用 `pcl::RegionGrowing` 做区域生长，它必须有法线；
- 用于计算 FPFH、PFH 等特征描述子，或做 point-to-plane ICP；
- 需要描述各个点附近的表面朝向。

拟合平面可以直接用 RANSAC，先算出系数 `a、b、c、d`，其中 `a、b、c` 就是法向。是否为地面，还要看方向和高度。

## 3.8.2 找平面（RANSAC）

除了按范围裁，PCL 还能把点云里的平面直接拟合出来。`pcl::SACSegmentation` 用 RANSAC：随机抽几个点定一个平面，数有多少点落在平面附近（内点），重复若干次，内点最多的那个就是结果。

```cpp
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>
#include <iostream>

using Cloud = pcl::PointCloud<pcl::PointXYZI>;

bool extractPlane(const Cloud::ConstPtr& cloud, Cloud& plane, Cloud& remaining) {
  auto inliers = pcl::make_shared<pcl::PointIndices>();
  pcl::ModelCoefficients coeffs;
  pcl::SACSegmentation<pcl::PointXYZI> seg;
  seg.setModelType(pcl::SACMODEL_PLANE);
  seg.setMethodType(pcl::SAC_RANSAC);
  seg.setDistanceThreshold(0.03);   // 点到平面的距离上限，米
  seg.setMaxIterations(500);
  seg.setOptimizeCoefficients(true);
  seg.setInputCloud(cloud);
  seg.segment(*inliers, coeffs);

  plane.clear();
  remaining.clear();
  if (inliers->indices.empty() || coeffs.values.size() < 4) return false;
  std::cout << coeffs.values[0] << " x + " << coeffs.values[1] << " y + "
            << coeffs.values[2] << " z + " << coeffs.values[3] << " = 0\n";

  pcl::ExtractIndices<pcl::PointXYZI> extract;
  extract.setInputCloud(cloud);
  extract.setIndices(inliers);
  extract.setNegative(false);       // 提取平面内点
  extract.filter(plane);
  extract.setNegative(true);        // 提取其余点
  extract.filter(remaining);
  return true;
}
```

`coeffs.values` 的前四个数依次是 `a、b、c、d`，前三个数就是平面法向：`|c|` 接近 1 说明是水平面（地面或桌面），`a、b` 为主说明是竖面（墙）。想知道拟合到的是哪个面，就看法向和位置。

和 `CropBox` 的区别：

- `CropBox` 按坐标范围裁，范围要按场景和坐标系给。点已经转到世界系时，固定场景的盒子可以保持不变；如果盒子表示飞机前方的区域，就要结合飞机位姿转换。
- RANSAC 按“哪些点共面”拟合，平面可以倾斜；需要设置距离阈值并反复采样，结果也可能是墙面或桌面。

可以先用 `CropBox` 排除范围外的点，再用 RANSAC 拟合平面，并检查方向和高度。已知地面高度时，也可以直接按高度裁剪。

## 3.8.3 栅格

有时想要一张俯视图：把 x、y 切成格子，统计每格有没有东西。

```text
ix = floor((x - x_min) / resolution)
iy = floor((y - y_min) / resolution)
```

二维格子存进一维数组，下标是 `iy * nx + ix`。每格记点数和最高点，点数超过阈值就标成占据。示例地图 8.4 m × 6.4 m、分辨率 0.1 m，就是 84×64 格。

它和“压成平面”不是一回事：

- 压成平面只是把 z 丢掉，点还是原来的点，投影后一堆点叠在一起；
- 栅格先划区域，每格只留统计值（点数、最高点、占据），输出是一张二维数组。

图上空白不代表能飞，也可能只是没扫到。用之前先按高度裁掉地面和天花板，否则地面会把整张图填满。

## 3.8.4 点云配准

从不同位置观察同一个场景，会得到不同坐标系下的点云。点云配准利用它们共同观测到的结构，估计两者之间的旋转和平移，再把点云对齐。

PCL 提供了 [`pcl::IterativeClosestPoint`（ICP）](https://pointclouds.org/documentation/classpcl_1_1_iterative_closest_point.html) 和 [`pcl::NormalDistributionsTransform`（NDT）](https://pointclouds.org/documentation/classpcl_1_1_normal_distributions_transform.html) 等类。下面是类声明的简写，省略具体实现：

```cpp
// pcl 命名空间内；PointSource、PointTarget 分别是待对齐和参考点云的点类型。
template <typename PointSource, typename PointTarget, typename Scalar = float>
class IterativeClosestPoint : public Registration<PointSource, PointTarget, Scalar> { /* ... */ };

template <typename PointSource, typename PointTarget>
class NormalDistributionsTransform : public Registration<PointSource, PointTarget> { /* ... */ };
```

3.6 中的 FAST-LIO 已经完成了位姿估计和点云转换。想进一步了解配准，可以选两份有重叠区域的点云，尝试估计它们之间的变换，观察初始位置、重叠程度和噪声对结果的影响。

## 3.8.5 GPU

前面各节的程序都在 CPU 上运行。PCL 的算法默认单线程或 OMP 多线程，法线那节用的就是 OMP 版。

如果考虑 GPU，先确认显卡、CUDA 和 PCL 对应模块的编译配置。GPU 模块的接口与 CPU 版可能不同，现有代码需要适配。

先用自己的点云分别测量各步耗时，确认时间主要花在哪里，再决定是否换实现。比较时同时记录点数、参数、线程数和设备；使用 GPU 时也要算上数据传输时间。

## 3.8.6 点云与神经网络

点云处理还有物体识别、语义分割、点云补全等方向，也可以引入 AI 和神经网络。例如，给每个点判断它属于地面、墙面还是车辆，或根据已有观测补全物体的形状。

我们目前对这些方向的探索还比较少。有兴趣的同学可以找公开的数据集和开源项目，先运行一个例子，再尝试换成自己采集的点云，看看效果如何。欢迎大家继续探索，把尝试的方法和结果分享出来。

---

[← 3.7 进仿真](07-sim.md)　·　[目录](index.md)

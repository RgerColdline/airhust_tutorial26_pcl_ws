<details>
<summary>目录</summary>

- [README](../README.md)
  - [阶段三：先学会用人家的函数](index.md)
    - [3.1 点云在内存里怎么存](01-io.md)
    - [3.2 用 rviz 看](02-rviz.md)
    - [3.3 滤波](03-filter.md)
    - **3.4 找邻居 ←**
    - [3.5 聚簇](05-cluster.md)
    - [3.6 点云哪来的](06-cloud-register.md)
    - [3.7 进仿真](07-sim.md)
    - [3.8 扩展](08-extend.md)

</details>

[← 3.3 滤波](03-filter.md)　·　[目录](index.md)　·　[3.5 聚簇 →](05-cluster.md)

# 3.4 找邻居

## 为什么要找邻居

一个点只有坐标。要判断它附近是平面还是墙角，需要同时看周围的点；要把同一个物体上的点分到一起，也要从一个点继续寻找相邻的点。后面的法线估计和欧式聚类都会用到邻域搜索。

邻域有两种常见定义：

- 最近的 `k` 个点：固定数量，范围随点云密度变化。
- 距离不超过 `r` 的点：固定空间范围，数量随点云密度变化。

例如，同样取 15 个邻居，桌面上点密集，范围可能很小；远处柱子上的点稀疏，范围就可能超过柱子的宽度。这会影响计算结果。

## KdTree 是什么

每次查询都遍历整张点云，可以找到邻居，但重复查询时会做大量距离计算。KdTree 先根据坐标建立空间索引，查询时排除与目标距离过远的区域，减少需要比较的点。

`pcl::search::KdTree` 是 PCL 提供的搜索类，[查看官网 API](https://pointclouds.org/documentation/classpcl_1_1search_1_1_kd_tree.html)。头文件是 `<pcl/search/kdtree.h>`。类声明简写如下：

```cpp
// pcl::search 命名空间内，省略继承和实现
template <typename PointT, class Tree = pcl::KdTreeFLANN<PointT>>
class KdTree;
```

`PointT` 是点类型，`Tree` 使用默认值即可。下面是按 `PointXYZI` 使用时的主要接口：

```cpp
pcl::search::KdTree<pcl::PointXYZI> tree;
tree.setInputCloud(cloud);

std::vector<int> indices;
std::vector<float> squared_distances;
int found = tree.nearestKSearch(query, 15, indices, squared_distances);
// 或：tree.radiusSearch(query, 0.10, indices, squared_distances);
```

`query` 是一个点。返回值 `found` 是实际找到的数量，`indices` 是这些点在输入点云中的下标，`squared_distances` 保存的是距离的平方。查询点本身若也在输入点云中，搜索结果通常包含它。

## 建不建树，差在哪

设点数为 N。不建树时，每次查询都要和全部点算一遍距离：

- 单次 k 近邻：O(N log k)，扫一遍、维护 k 个最近点
- 单次半径查询：O(N)
- 每个点都查一次（法线、去噪、聚类都是这种）：O(N²)

KdTree 先用 O(N log N) 的时间和 O(N) 的内存把点按空间分开，查询时只搜索可能包含邻居的分支：

- 单次 k 近邻：平均 O(log N + k)
- 单次半径查询：平均 O(log N + m)，m 是结果数
- 每个点都查一次：平均 O(N log N)

建树需要额外的时间和内存。查询较少时，直接遍历可能更合适；需要反复查询时，空间索引通常能减少距离计算。查询范围和点的分布也会影响效率，需要访问大量点时，查询耗时可能接近遍历整张点云。

实际耗时和内存占用要用自己的数据测，记录点数、邻域参数和查询次数；比较时把建树与查询分别计时。

## 3.3 的去噪建树了吗

本节的无序点云会用到 KdTree。`pcl::StatisticalOutlierRemoval` 在执行 `filter` 时准备搜索器、设置它的输入，再逐点查邻居；只调用滤波器的 `setInputCloud` 还没开始建树。有序点云可能使用 `OrganizedNeighbor`，具体看输入和实现。

3.3 里另外三个滤波器不建树：`VoxelGrid` 按格子归并，`PassThrough` 和 `CropBox` 逐点比较坐标。

## 什么情况会用到

法线估计（3.8 扩展）里用 `setKSearch(15)` 表示取最近的 15 个点；也可以改成 `setRadiusSearch(r)`，按半径取邻域。

3.5 的 `setClusterTolerance(0.12)` 表示聚类时按 0.12 m 的邻域继续搜索相连的点。它控制连接距离，不限制整个聚类的直径；那里的 `setSearchMethod` 传的就是这节讲的搜索对象，见 [3.5 聚簇](05-cluster.md)。

---

[← 3.3 滤波](03-filter.md)　·　[目录](index.md)　·　[3.5 聚簇 →](05-cluster.md)

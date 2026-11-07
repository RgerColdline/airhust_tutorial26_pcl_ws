<details>
<summary>目录</summary>

- [README](../README.md)
  - [阶段三：先学会用人家的函数](index.md)
    - [3.1 点云在内存里怎么存](01-io.md)
    - [3.2 用 rviz 看](02-rviz.md)
    - [3.3 滤波](03-filter.md)
    - [3.4 找邻居](04-kdtree.md)
    - [3.5 聚簇](05-cluster.md)
    - **3.6 点云哪来的 ←**
    - [3.7 进仿真](07-sim.md)
    - [3.8 扩展](08-extend.md)

</details>

[← 3.5 聚簇](05-cluster.md)　·　[目录](index.md)　·　[3.7 进仿真 →](07-sim.md)

# 3.6 点云哪来的

前面从 PCD 文件读取点云，实际运行时，雷达会持续扫描，由驱动程序把点云发布到 ROS 话题，例如 `/livox/lidar`。

我们实际处理时，直接订阅已经处理过的 `/cloud_registered`，消息类型是 `sensor_msgs/PointCloud2`。下面分别介绍它在实机和仿真中的来历。

## 实机：FAST-LIO / FAST-LIO2

实机上，FAST-LIO 或 FAST-LIO2 接收雷达点云和 IMU 数据，估计雷达的位姿、补偿扫描期间的运动影响，再将当前帧的点转换到参考坐标系，发布到 `/cloud_registered`。

需要手动执行实机对应的启动脚本或 launch 文件，由它们启动 FAST-LIO / FAST-LIO2。等这个话题开始发布后，后续程序就可以订阅它。

<details>
<summary>我之前尝试过自己做配准</summary>

我之前尝试用 PCL 的 [ICP](https://pointclouds.org/documentation/classpcl_1_1_iterative_closest_point.html) 和 [NDT](https://pointclouds.org/documentation/classpcl_1_1_normal_distributions_transform.html) 自己处理原始点云数据，但计算耗时较长，稀疏、带噪声的点云也经常对不齐，后来改用 FAST-LIO，由它处理原始数据，再订阅 `/cloud_registered` 做后续处理。

</details>

## 仿真：以 tutorial_gazebo 的真值方案为例

这里介绍的是本地 `catkin_ws/src/tutorials/tutorial_gazebo` 仿真包的实现：`cloud_register_mapper` 利用 Gazebo 提供的模型真值处理雷达点云，发布 `/cloud_registered`。

这个包的 `sim-new.launch` 已经包含该节点，启动后可以直接订阅 `/cloud_registered`。3.7 使用的是学生镜像中的 `simulation` 包，属于另一套仿真环境；它的发布节点以对应启动文件和 `rostopic info /cloud_registered` 的输出为准。

<details>
<summary>tutorial_gazebo 中的简化处理</summary>

`tutorial_gazebo` 的这个节点使用真值中的位置给点坐标加上平移偏移，并将消息的 `frame_id` 设为 `world`。完整的坐标转换还包含旋转；飞机转向后，需要结合雷达输出的坐标方向检查这一步。

</details>

这里介绍的 FAST-LIO 和真值节点都发布 `/cloud_registered`。本地 FAST-LIO 使用 `camera_init` 坐标系，`tutorial_gazebo` 的真值节点使用 `world`。使用学生镜像时，rviz 的 Fixed Frame 按实际消息头中的 `frame_id` 设置。

## 一帧点云和多帧累计

上述本地实现发布的 `/cloud_registered` 都只包含当前帧处理后的点。飞机换一个位置，下一帧包含的是新位置上观测到的点。

如果想把不同位置看到的场景合成一份点云，还需要把这些帧中的点合并起来。转换到同一个坐标系，是合并前的准备；把多帧点保存在一起，才是累计。位姿和时间对应准确时，同一个静止物体在各帧中的点才能相互对齐。移动物体则可能在累计结果中保留多个时刻的位置。

下一节订阅连续点云，把累计、滤波和聚类串成一个处理节点。

## 任务

仿真启动后，检查 `/cloud_registered`：

```shell
rostopic info /cloud_registered
rostopic type /cloud_registered
rostopic echo -n 1 /cloud_registered/header
rostopic hz /cloud_registered
```

依次查看发布节点、消息类型、坐标系和发布频率，再在 rviz 中添加 PointCloud2，选择这个话题，把 Fixed Frame 设为消息头中的 `frame_id`。如果使用其他 Fixed Frame，需要提供它与点云坐标系之间的 TF 变换。

这一节先认识点云的来源，下一节再接收仿真点云并连续处理。


---

[← 3.5 聚簇](05-cluster.md)　·　[目录](index.md)　·　[3.7 进仿真 →](07-sim.md)

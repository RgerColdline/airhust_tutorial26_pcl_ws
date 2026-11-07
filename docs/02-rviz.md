<details>
<summary>目录</summary>

- [README](../README.md)
  - [阶段三：先学会用人家的函数](index.md)
    - [3.1 点云在内存里怎么存](01-io.md)
    - **3.2 用 rviz 看 ←**
    - [3.3 滤波](03-filter.md)
    - [3.4 找邻居](04-kdtree.md)
    - [3.5 聚簇](05-cluster.md)
    - [3.6 点云哪来的](06-cloud-register.md)
    - [3.7 进仿真](07-sim.md)
    - [3.8 扩展](08-extend.md)

</details>

[目录](index.md)　·　[3.3 滤波 →](03-filter.md)

# 3.2 用 rviz 看

要在 rviz 中将点云和其他 ROS 数据叠加显示，需要先把 PCL 点云转换成 ROS 消息 `sensor_msgs::PointCloud2`。

## 任务

[`src/02_publish.cpp`](../src/tutorial_pcl_basic/src/02_publish.cpp) 读 pcd，把点云转成 `sensor_msgs::PointCloud2` 发到 `/demo_room`。转换部分需要你在 `publish` 函数里完成。

### 要用的东西

```cpp
template <typename PointT>
void toROSMsg (const pcl::PointCloud<PointT> &cloud_in, sensor_msgs::PointCloud2 &msg);
```

`msg.header.frame_id` 要和 rviz 的 Fixed Frame 一致，两边都填 `world`：

```cpp
msg.header.frame_id = "world";
```

不设的话，rviz 会提示 `Message ... has no frame id`，点云显示不出来。

`advertise` 最后一个参数 `true` 是 latched：静态点云只发一次，晚连上来的订阅者也能收到。

## 封装成 publish 函数

把转换和发布放在同一个函数里：

```cpp
using Cloud = pcl::PointCloud<pcl::PointXYZI>;

void publish(const Cloud& cloud, sensor_msgs::PointCloud2& msg,
             const ros::Publisher& pub);
```

`cloud` 是输入，`msg` 用来保存转换后的消息，`pub` 负责发送。点云、消息容器和发布器都由调用方创建：

```cpp
auto cloud = pcl::make_shared<Cloud>();
sensor_msgs::PointCloud2 msg;
auto pub = nh.advertise<sensor_msgs::PointCloud2>("/demo_room", 1, true);

// 读取点云后，转换并发布。
publish(*cloud, msg, pub);
ros::spin();
```

`publish` 内部补全消息转换和 `frame_id`，发送部分已经写好。连续接收点云时，可以在每帧处理完成后调用它，复用同一个 `msg` 和发布器。这里仍只读取、发布一份 PCD 文件。

## 启动 roscore 和 rviz

完成代码后，在两个终端分别启动：

```shell
# 终端 1
roscore

# 终端 2
rviz
```

rviz 里：**Fixed Frame** 填 `world`。**Add** → **PointCloud2**，Topic 填 `/demo_room`。

这两个只要还开着，就不用重复启动。`roscore`、`rviz` 用的是 ROS 本体，`.bashrc` 里已经 source 过 `/opt/ros/noetic`。

## 运行

```shell
./run 2
```

程序会持续运行（Ctrl-C 退出），这样 rviz 什么时候连上来都能收到这条消息。

## 运行结果

终端：

```
发了 112400 个点，话题 /demo_room
```

rviz 里能看到整个房间。

看不到的话先查两处：topic 名字对不对，Fixed Frame 和 `msg.header.frame_id` 是不是都是 `world`。

如果消息的 `frame_id` 已是 `world`，rviz 的 Fixed Frame 也设为 `world`，显示这份点云无需额外的 TF。需要与其他坐标系的数据叠加时，再提供实际存在的坐标变换。坐标变换应与传感器实际安装位置和坐标定义一致。

---

[← 3.1 点云怎么存](01-io.md)　·　[目录](index.md)　·　[3.3 滤波 →](03-filter.md)

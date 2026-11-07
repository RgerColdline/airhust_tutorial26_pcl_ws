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
    - **3.7 进仿真 ←**
    - [3.8 扩展](08-extend.md)

</details>

[← 3.6 点云哪来的](06-cloud-register.md)　·　[目录](index.md)　·　[3.8 扩展 →](08-extend.md)

# 3.7 进仿真

## 目标

把前面几步串成一条在线流水线：订阅 `/cloud_registered`，每收到一帧就累积、裁区域、降采样、去噪、聚类，然后把染好色的结果发布到 `/pipeline_result`。程序持续运行并发布结果，rviz 里可以实时看到分簇。

四个处理函数需要补全，其余部分已经写好。

## 程序结构

[`07_pipeline.cpp`](../src/tutorial_pcl_basic/src/07_pipeline.cpp) 里，点云和参数放在文件顶部，`cloudCallback` 是 `/cloud_registered` 的订阅回调：每来一帧，依次调用下面四个函数，染色后发布到 `/pipeline_result`。`main` 里创建发布器、订阅话题，最后调用 `ros::spin()`。

`crop` 用 `CropBox` 按盒子裁掉地面和范围外的点，盒子范围在文件顶部，更换仿真场景时按点云坐标调整这些值。要补的四个函数在回调前面：

```cpp
void accumulate(const Cloud::ConstPtr& frame, Cloud& map);                     // TODO 1
void downsample(const Cloud::ConstPtr& cloud, Cloud& output, float leaf);      // TODO 2
void denoise(const Cloud::ConstPtr& cloud, Cloud& output, int mean_k, double stddev);  // TODO 3
void cluster(const Cloud::ConstPtr& cloud, std::vector<pcl::PointIndices>& clusters,
             double tolerance, int min_size);                                  // TODO 4
```

处理参数在源码顶部设置，修改后重新编译：

| 参数 | 默认 | 含义 |
| --- | --- | --- |
| `kLeaf` | 0.05 | 体素边长，米 |
| `kMeanK` / `kStddev` | 20 / 1.0 | 统计去噪 |
| `kTolerance` | 0.15 | 聚类容差，米 |
| `kMinCluster` | 20 | 每簇最少点数 |

参数没有通用值。换一张地图、换一个场景，体素、去噪和聚类容差都要重新试：先看各步点数的变化，再看该保留的物体还在不在。

## 任务

1. 补全四个函数。
2. 先启动 `roscore`，用包里的假发布者验证链路：
   ```shell
   # 新终端先加载本工作空间环境。
   source ~/airhust_tutorial26_pcl_ws/devel/setup.bash
   rosrun tutorial_pcl_basic publish_demo.py
   ```
   假发布者只支持 `x/y/z/intensity` 四个 float32 字段的未压缩 binary PCD，默认房间数据符合这个格式；其他格式会明确报错。

   另开终端在本工作空间根目录执行 `./run 7`，再在 rviz 中添加 PointCloud2，订阅 `/pipeline_result`，Fixed Frame 设为 `world`。

   分色结果用来看聚类，其他几步看各自的话题：降采样后点数是否减少，去噪后离群点是否减少。假发布者一直发送同一份点云；累计效果要到仿真里换几个位置，看看之前扫描到的区域是否还在。
3. 确认各步处理结果后，停止假发布者，再启动仿真。

## 启动仿真

默认场景中可用于聚类的物体较少，本节使用仓库中的 [`worlds/modified_task.world`](../worlds/modified_task.world)。在已加载仿真工作空间环境的终端，进入本工作空间根目录，把场景文件复制到仿真包：

```shell
simulation_dir="$(rospack find simulation)"
mkdir -p "$simulation_dir/worlds"
cp worlds/modified_task.world "$simulation_dir/worlds/"
```

再打开仿真包中的 `sim.launch`，把 `world` 这一行的默认值改为这份文件：

```xml
<arg name="world" default="$(find simulation)/worlds/modified_task.world"/>
```

只改 `world` 这一行的默认值，其它字段不动。然后启动：

```shell
roslaunch simulation sim.launch
```

场景中有围墙、圆柱、方柱和门框，可以用来观察聚类结果。场景还引用了 `sun`、`ground_plane`，H 标志使用 `h_marker` 的材质；`h_marker` 位于仿真包的 `models/` 下，该目录需要包含在 Gazebo 的模型搜索路径 `GAZEBO_MODEL_PATH` 中。

## 运行

```shell
./run 7
```

程序每 10 帧打印一次点数、簇数和前 10 个簇的中心。“累计”是追加当前帧后、裁剪前的点数。处理完成后，用裁剪并降采样的结果更新地图，下一帧继续在这份地图上追加点，限制保留地图的范围和密度。

每一步的结果都单独发布，便于逐步排查：

| 话题 | 内容 |
| --- | --- |
| `/pipeline_accumulated` | 累积后的点云 |
| `/pipeline_cropped` | 裁区域后 |
| `/pipeline_filtered` | 降采样后 |
| `/pipeline_denoised` | 去噪后 |
| `/pipeline_result` | 聚类、染色后的结果 |

多发布会增加少量开销，调试结束后可以只保留 `/pipeline_result`。

## 运行结果

更换仿真场景或参数后，点数和簇数都会变；飞行路径和收帧时机不同，每次跑的结果也会有出入。下面是用假发布者反复发送 `demo_room.pcd`、采用默认参数时的一组输出：

```text
第 10 帧: 累计 145493 点, 降采样 33093, 去噪 32693, 4 个簇
簇 0: 25102 点, 中心 (4.00126, 2.48216, 1.14791)
簇 1: 4006 点, 中心 (2.61144, 3.11048, 0.726783)
簇 2: 2282 点, 中心 (5.50054, 4.30247, 1.15435)
簇 3: 1303 点, 中心 (5.49903, 1.50053, 1.15861)
第 20 帧: 累计 145493 点, 降采样 33093, 去噪 32693, 4 个簇
...
```

## 扩展

源码提供的是初始参数，需要结合场景调整。

### 去噪后表面边缘缺失

`StatisticalOutlierRemoval` 按“到最近 k 个邻居的平均距离”判断离群点。平面边缘的邻居分布在一侧，到最近 k 个邻居的平均距离可能偏大，去噪时容易把部分边缘点一起删掉。

在 rviz 里对比 `/pipeline_filtered` 和 `/pipeline_denoised`，观察边缘是否有变化。`kMeanK` 和 `kStddev` 在源码顶部，改完重新编译；也可以比较把这步去掉之后的效果。

### 裁剪范围不合适

`CropBox` 按坐标范围筛点。范围太大时，会包含任务范围外的围墙或地面；范围太小时，目标物体可能被截断。

在 rviz 里看 `/pipeline_cropped`，对照场景调整 `kCropMin*` 和 `kCropMax*` 六个值。点云坐标系的原点和朝向可能与场景文件使用的坐标系不同，调整时以话题中点的坐标为准。

### 其它参数

体素、聚类容差、最少点数也一样，改一组跑一次，比较簇的数量、大小和位置。

---

[← 3.6 点云哪来的](06-cloud-register.md)　·　[目录](index.md)　·　[3.8 扩展 →](08-extend.md)

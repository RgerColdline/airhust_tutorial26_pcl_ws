# 阶段三：先学会用人家的函数

本阶段考查点云基本操作和 PCL 接口的使用。请根据各节说明，独立补全源码中的 `TODO`。文档提供基础用法、API 说明和调用示例。

## 目录

- [README](../README.md)
  - **阶段三：先学会用人家的函数 ←**
    - [3.1 点云在内存里怎么存](01-io.md)
    - [3.2 用 rviz 看](02-rviz.md)
    - [3.3 滤波](03-filter.md)
    - [3.4 找邻居](04-kdtree.md)
    - [3.5 聚簇](05-cluster.md)
    - [3.6 点云哪来的](06-cloud-register.md)
    - [3.7 进仿真](07-sim.md)
    - [3.8 扩展](08-extend.md)

## 准备工作

### 代码在哪

```text
airhust_tutorial26_pcl_ws/
├── run                            # 启动脚本：./run 3
├── data/
│   ├── demo_room.pcd              # 模拟房间，112400 个点
│   ├── demo_room_noisy.pcd        # 加入重复点和噪声后的房间，3.3 使用
│   ├── table_scene_lms400.pcd     # 另一份点云，可用于比较不同数据
│   ├── make_demo_scene.py         # 生成房间数据
│   └── make_noisy.py              # 生成重复点和噪声
├── out/                           # 程序输出
├── worlds/                        # 3.7 使用的仿真场景
├── docs/                          # 各节说明
└── src/
    └── tutorial_pcl_basic/
        ├── src/                   # 需要补全的源码
        └── scripts/               # 3.7 的假发布者
```

### 怎么编译

在工作空间根目录执行：

```shell
cd ~/airhust_tutorial26_pcl_ws
catkin_make --pkg tutorial_pcl_basic
```

修改源码后，需要重新编译。

### 怎么运行

`run` 脚本会加载工作空间环境，再启动对应程序：

```shell
./run 3
./run 3 data/table_scene_lms400.pcd
./run 7
```

1、2、3、5 接收一个可选的点云文件路径；换数据时，要按数据的坐标范围调整裁剪参数。7 订阅 ROS 话题，连续处理接收到的点云。不带参数运行 `./run` 会打印用法。

如果在终端直接使用 `rosrun`，先按所用 shell 加载一次环境。

bash：

```bash
source devel/setup.bash
```

zsh：

```zsh
source devel/setup.zsh
```

工作空间较多时，把这些命令写入 shell 配置会使环境混在一起。运行本阶段程序用 `./run` 即可；直接使用 `rosrun` 时，在对应终端加载环境。

### 关于数据

PCD 是 Point Cloud Data 的缩写，是 PCL 使用的点云文件格式。

`data/demo_room.pcd` 由脚本生成，模拟一个 8m × 6m × 2.6m 的房间：地面、四面墙、一张 0.75m 高的桌子、桌上三个零件，以及两根半径为 0.22m 和 0.35m 的柱子。

程序读入文件后，在内存中处理点云；实时运行时，也可以直接接收 ROS 话题中的点云。保存 PCD 主要用于记录数据、检查中间结果或离线处理。

### 如何检查结果

各节的输出示例用来说明需要观察的内容。参数、输入或采集过程不同，点数和分簇结果也可能不同。

源码中的占位代码用于展示处理流程。补全后，需要同时检查终端输出和实际点云，确认各步处理是否符合要求。

### 小节和程序编号

| 小节 | 内容 | 运行命令 |
| --- | --- | --- |
| 3.1 点云在内存里怎么存 | `01_io.cpp` | `./run 1` |
| 3.2 用 rviz 看 | `02_publish.cpp` | `./run 2` |
| 3.3 滤波 | `03_filter.cpp` | `./run 3` |
| 3.4 找邻居 | 阅读与参数理解 | 无独立程序 |
| 3.5 聚簇 | `05_cluster.cpp` | `./run 5` |
| 3.6 点云哪来的 | 查看 ROS 话题及点云来源 | 无独立 C++ 程序 |
| 3.7 进仿真 | `07_pipeline.cpp`，在线综合考核 | `./run 7` |
| 3.8 扩展 | 选读内容 | 无独立程序 |

**下一步：[3.1 点云在内存里怎么存](01-io.md)**

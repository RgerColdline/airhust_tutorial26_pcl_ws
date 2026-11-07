#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>

#include <iostream>
#include <string>

#include "pcl_basic/path.hpp"

using Cloud = pcl::PointCloud<pcl::PointXYZI>;

// 消息容器由调用方创建，每次调用更新内容并发布。
void publish(const Cloud& cloud, sensor_msgs::PointCloud2& msg,
             const ros::Publisher& pub) {
  // TODO：把点云转换为 msg，并将 frame_id 设为 world。
  // 转换函数和坐标系说明见 3.2。
  // 目标输出：发了 112400 个点，话题 /demo_room。
  std::cout << "把点云转换为 ROS 消息\n";
  msg.header.stamp = ros::Time::now();
  pub.publish(msg);
  std::cout << "发了 " << msg.width * msg.height << " 个点，话题 /demo_room\n";
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "s02_publish");
  ros::NodeHandle nh;

  // 创建点云和消息容器。
  auto cloud = pcl::make_shared<Cloud>();
  sensor_msgs::PointCloud2 msg;
  // 发布器在节点运行期间保持有效；true 表示锁存最后一帧。
  auto pub = nh.advertise<sensor_msgs::PointCloud2>("/demo_room", 1, true);

  // 读取点云。
  const std::string path = argc > 1 ? argv[1] : dataPath("demo_room.pcd");
  if (pcl::io::loadPCDFile(path, *cloud) < 0) {
    std::cerr << "读取失败: " << path << "\n";
    return 1;
  }
  // 转换并发布。
  publish(*cloud, msg, pub);
  // 等待订阅者，Ctrl-C 退出。
  ros::spin();
  return 0;
}

#include <pcl/common/centroid.h>
#include <pcl/filters/crop_box.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl_conversions/pcl_conversions.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <std_msgs/Header.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using Cloud = pcl::PointCloud<pcl::PointXYZI>;
using ColoredCloud = pcl::PointCloud<pcl::PointXYZRGB>;

//////////////////////// 点云和参数 ////////////////////////

// 修改处理参数后重新编译。
constexpr double kLeaf = 0.05;
constexpr int kMeanK = 20;
constexpr double kStddev = 1.0;
constexpr double kTolerance = 0.15;
constexpr int kMinCluster = 20;

// 裁剪范围按点云坐标系和任务调整。
constexpr float kCropMinX = -3.0f;
constexpr float kCropMinY = -6.0f;
constexpr float kCropMinZ = 0.1f;
constexpr float kCropMaxX = 9.0f;
constexpr float kCropMaxY = 6.0f;
constexpr float kCropMaxZ = 2.2f;

// 回调中复用的点云。
Cloud::Ptr map = pcl::make_shared<Cloud>();
Cloud::Ptr cropped = pcl::make_shared<Cloud>();
Cloud::Ptr filtered = pcl::make_shared<Cloud>();
Cloud::Ptr denoised = pcl::make_shared<Cloud>();
int frame_count = 0;
ros::Publisher pub_accumulated;
ros::Publisher pub_cropped;
ros::Publisher pub_filtered;
ros::Publisher pub_denoised;
ros::Publisher pub_result;

//////////////////////// 处理函数 ////////////////////////

// 把新来的一帧并进累计点云。
void accumulate(const Cloud::ConstPtr& frame, Cloud& map) {
  // ---- TODO 1 -------------------------------------------------------------
  // 把 frame 里的点追加到 map 末尾，再更新 width / height。
  // -------------------------------------------------------------------------
}

void crop(const Cloud::ConstPtr& cloud, Cloud& output) {
  output.clear();
  pcl::CropBox<pcl::PointXYZI> box;
  box.setInputCloud(cloud);
  box.setMin(Eigen::Vector4f(kCropMinX, kCropMinY, kCropMinZ, 1.0f));
  box.setMax(Eigen::Vector4f(kCropMaxX, kCropMaxY, kCropMaxZ, 1.0f));
  box.filter(output);
}

void downsample(const Cloud::ConstPtr& cloud, Cloud& output, float leaf) {
  output = *cloud;  // 占位：先原样拷贝
  // ---- TODO 2 -------------------------------------------------------------
  // 用 pcl::VoxelGrid 按 leaf 降采样，见 3.3。
  // -------------------------------------------------------------------------
}

void denoise(const Cloud::ConstPtr& cloud, Cloud& output, int mean_k, double stddev) {
  output = *cloud;  // 占位：先原样拷贝
  // ---- TODO 3 -------------------------------------------------------------
  // 用 pcl::StatisticalOutlierRemoval 去噪，见 3.3。
  // -------------------------------------------------------------------------
}

void cluster(const Cloud::ConstPtr& cloud, std::vector<pcl::PointIndices>& clusters,
             double tolerance, int min_size) {
  clusters.clear();
  // ---- TODO 4 -------------------------------------------------------------
  // 用 pcl::EuclideanClusterExtraction 聚类，容差和最少点数用参数，见 3.5。
  // -------------------------------------------------------------------------
}

void describeClusters(const Cloud& cloud, const std::vector<pcl::PointIndices>& clusters) {
  const std::size_t shown = std::min<std::size_t>(clusters.size(), 10);
  for (std::size_t c = 0; c < shown; ++c) {
    Eigen::Vector4f centroid;
    pcl::compute3DCentroid(cloud, clusters[c].indices, centroid);
    std::cout << "簇 " << c << ": " << clusters[c].indices.size() << " 点, 中心 ("
              << centroid[0] << ", " << centroid[1] << ", " << centroid[2] << ")\n";
  }
}

void colorClusters(const Cloud& cloud, const std::vector<pcl::PointIndices>& clusters,
                   ColoredCloud& colored) {
  colored.clear();
  colored.resize(cloud.size());
  for (std::size_t i = 0; i < cloud.size(); ++i) {
    colored[i].x = cloud.points[i].x;
    colored[i].y = cloud.points[i].y;
    colored[i].z = cloud.points[i].z;
    colored[i].r = 255;
    colored[i].g = 255;
    colored[i].b = 255;
  }
  const std::array<std::array<std::uint8_t, 3>, 8> palette{{
      {{255, 0, 0}},   {{0, 255, 0}},   {{0, 0, 255}},   {{255, 255, 0}},
      {{0, 255, 255}}, {{255, 0, 255}}, {{255, 128, 0}}, {{128, 0, 255}},
  }};
  for (std::size_t c = 0; c < clusters.size(); ++c) {
    const auto& color = palette[c % palette.size()];
    for (int idx : clusters[c].indices) {
      colored[idx].r = color[0];
      colored[idx].g = color[1];
      colored[idx].b = color[2];
    }
  }
}

//////////////////////// 回调函数 ////////////////////////

template <typename CloudT>
void publishCloud(const CloudT& cloud, const ros::Publisher& pub, const std_msgs::Header& header) {
  sensor_msgs::PointCloud2 msg;
  pcl::toROSMsg(cloud, msg);
  msg.header = header;
  pub.publish(msg);
}

void cloudCallback(const sensor_msgs::PointCloud2::ConstPtr& msg) {
  auto frame = pcl::make_shared<Cloud>();
  pcl::fromROSMsg(*msg, *frame);

  accumulate(frame, *map);
  const auto accumulated_size = map->size();
  crop(map, *cropped);
  downsample(cropped, *filtered, kLeaf);
  denoise(filtered, *denoised, kMeanK, kStddev);

  std::vector<pcl::PointIndices> clusters;
  cluster(denoised, clusters, kTolerance, kMinCluster);

  ColoredCloud colored;
  colorClusters(*denoised, clusters, colored);

  // 每步都发一份，调试时可以单独看哪一步不对；多发几份会略增加开销。
  publishCloud(*map, pub_accumulated, msg->header);
  publishCloud(*cropped, pub_cropped, msg->header);
  publishCloud(*filtered, pub_filtered, msg->header);
  publishCloud(*denoised, pub_denoised, msg->header);

  publishCloud(colored, pub_result, msg->header);

  // 地图保持降采样后的规模，下一帧在上面继续叠加。
  *map = *filtered;

  if (++frame_count % 10 == 0) {
    std::cout << "第 " << frame_count << " 帧: 累计 " << accumulated_size << " 点, 降采样 "
              << filtered->size() << ", 去噪 " << denoised->size() << ", "
              << clusters.size() << " 个簇\n";
    describeClusters(*denoised, clusters);
  }
}

//////////////////////// 主函数 ////////////////////////

int main(int argc, char** argv) {
  ros::init(argc, argv, "s07_pipeline");
  ros::NodeHandle nh;

  pub_accumulated = nh.advertise<sensor_msgs::PointCloud2>("/pipeline_accumulated", 1, true);
  pub_cropped = nh.advertise<sensor_msgs::PointCloud2>("/pipeline_cropped", 1, true);
  pub_filtered = nh.advertise<sensor_msgs::PointCloud2>("/pipeline_filtered", 1, true);
  pub_denoised = nh.advertise<sensor_msgs::PointCloud2>("/pipeline_denoised", 1, true);
  pub_result = nh.advertise<sensor_msgs::PointCloud2>("/pipeline_result", 1, true);
  auto sub = nh.subscribe("/cloud_registered", 1, cloudCallback);
  ROS_INFO("s07_pipeline: subscribed /cloud_registered");
  ros::spin();
  return 0;
}

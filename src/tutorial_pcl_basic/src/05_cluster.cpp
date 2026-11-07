#include <pcl/common/centroid.h>
#include <pcl/filters/crop_box.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pcl_basic/path.hpp"

using Cloud = pcl::PointCloud<pcl::PointXYZI>;

void readCloud(const std::string& path, Cloud& cloud) {
  if (pcl::io::loadPCDFile(path, cloud) < 0 || cloud.empty())
    throw std::runtime_error("点云读取失败或没有点: " + path);
  const auto original_size = cloud.size();
  std::size_t valid = 0;
  for (std::size_t i = 0; i < cloud.size(); ++i) {
    const auto& point = cloud.points[i];
    if (std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z))
      cloud.points[valid++] = point;
  }
  cloud.resize(valid);
  cloud.width = static_cast<std::uint32_t>(valid);
  cloud.height = 1;
  cloud.is_dense = true;
  if (cloud.empty()) throw std::runtime_error("点云没有有限坐标: " + path);
  std::cout << "输入点数: " << cloud.size() << "，排除无效坐标 " << original_size - cloud.size() << " 点\n";
}

void downsample(const Cloud::ConstPtr& cloud, Cloud& output, float leaf) {
  output.clear();
  pcl::VoxelGrid<pcl::PointXYZI> grid;
  grid.setInputCloud(cloud);
  grid.setLeafSize(leaf, leaf, leaf);
  grid.filter(output);
  std::cout << "降采样后点数: " << output.size() << "\n";
}

void region(const Cloud::ConstPtr& cloud, Cloud& output) {
  output.clear();
  pcl::CropBox<pcl::PointXYZI> crop;
  crop.setInputCloud(cloud);
  crop.setMin(Eigen::Vector4f(1.6f, 1.0f, 0.2f, 1.0f));
  crop.setMax(Eigen::Vector4f(6.6f, 5.0f, 2.4f, 1.0f));
  crop.filter(output);
  std::cout << "裁剪后点数: " << output.size() << "\n";
}

void cluster(const Cloud::ConstPtr& cloud, std::vector<pcl::PointIndices>& clusters) {
  clusters.clear();
  // ---- TODO 1 -------------------------------------------------------------
  // 用 pcl::EuclideanClusterExtraction 把点按距离分组。
  // 距离阈值、点数上下限见 3.5 的「调用聚类类」，可以自己调。
  // 结果写进 clusters，每堆一组下标。
  std::cout << "按距离把点分组\n";
  // -------------------------------------------------------------------------
}

// 算一堆的点数和中心。
std::size_t clusterCenter(const Cloud::ConstPtr& cloud, const pcl::PointIndices& one,
                          Eigen::Vector4f& centroid) {
  // ---- TODO 2 -------------------------------------------------------------
  // 补两处：count 换成这一堆的点数；用 pcl::compute3DCentroid 把中心算进 centroid。
  // 参数和例子见 3.5 的「算中心」。打印由 describeClusters 负责。
  std::size_t count = 0;  // ← 换成这一堆的点数
  // ← 这里算中心，写进 centroid
  return count;
  // -------------------------------------------------------------------------
}

void describeClusters(const Cloud::ConstPtr& cloud,
                      const std::vector<pcl::PointIndices>& clusters) {
  std::cout << "分出 " << clusters.size() << " 堆\n";
  const std::size_t shown = std::min<std::size_t>(clusters.size(), 10);
  if (clusters.size() > shown) std::cout << "只列前 10 堆\n";
  for (std::size_t c = 0; c < shown; ++c) {
    Eigen::Vector4f centroid = Eigen::Vector4f::Zero();
    const std::size_t count = clusterCenter(cloud, clusters[c], centroid);
    if (count == 0) continue;
    std::cout << "堆 " << c << ": " << count << " 点, 中心 (" << centroid[0] << ", "
              << centroid[1] << ", " << centroid[2] << ")\n";
  }
}

// 每堆一种颜色，没进任何一堆的点用白色。
void colorClusters(const Cloud::ConstPtr& cloud,
                   const std::vector<pcl::PointIndices>& clusters,
                   pcl::PointCloud<pcl::PointXYZRGB>& colored) {
  colored.clear();
  colored.resize(cloud->size());
  for (std::size_t i = 0; i < cloud->size(); ++i) {
    colored[i].x = cloud->points[i].x;
    colored[i].y = cloud->points[i].y;
    colored[i].z = cloud->points[i].z;
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

int main(int argc, char** argv) try {
  auto cloud = pcl::make_shared<Cloud>();
  auto sampled = pcl::make_shared<Cloud>();
  auto cropped = pcl::make_shared<Cloud>();
  // 读取房间点云，也可以从命令行指定文件。
  readCloud(argc > 1 ? argv[1] : dataPath("demo_room.pcd"), *cloud);
  // 先降采样，点少一些，聚类快一些。
  downsample(cloud, *sampled, 0.05f);
  // 裁掉地面和不关心的区域。
  region(sampled, *cropped);
  // 按点之间的距离聚类。
  std::vector<pcl::PointIndices> clusters;
  cluster(cropped, clusters);
  // 输出数量和中心，给每堆染色。
  describeClusters(cropped, clusters);
  pcl::PointCloud<pcl::PointXYZRGB> colored;
  colorClusters(cropped, clusters, colored);
  // 保存彩色点云。
  if (pcl::io::savePCDFileBinary(outPath("05_clusters.pcd"), colored) < 0)
    throw std::runtime_error("聚类点云保存失败");
  std::cout << "存到 " << outPath("05_clusters.pcd") << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << e.what() << "\n";
  return 1;
}

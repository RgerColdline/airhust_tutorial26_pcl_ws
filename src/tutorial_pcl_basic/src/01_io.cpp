#include <pcl/common/common.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <iostream>
#include <string>

#include "pcl_basic/path.hpp"

int main(int argc, char** argv) {
  const std::string path = (argc > 1) ? argv[1] : dataPath("demo_room.pcd");

  // 读文件。pcl::io::loadPCDFile 返回负数就是没读成功，
  // 最常见的原因是路径不对，或者 pcd 里存的点类型不是 PointXYZI。
  pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);
  if (pcl::io::loadPCDFile<pcl::PointXYZI>(path, *cloud) == -1) {
    std::cerr << "读不了 " << path << "，看看文件在不在\n";
    return 1;
  }

  // ---- TODO 1 -------------------------------------------------------------
  // 打印点云的基本信息：点数、width、height、is_dense。
  //
  // 目标输出类似这样：
  //   点数     : 112400
  //   width    : 112400
  //   height   : 1
  //   is_dense : 1
  std::cout << "打印点云的基本信息：点数、width、height、is_dense\n";
  // -------------------------------------------------------------------------

  // ---- TODO 2 -------------------------------------------------------------
  // 求三个轴上的最小 / 最大坐标，打印包围盒。
  //
  // 目标输出类似这样：
  //   包围盒 x [-0.015, 8.015]  y [-0.015, 6.017]  z [-0.015, 2.611]
  std::cout << "打印包围盒：三个轴的最小值和最大值\n";
  // -------------------------------------------------------------------------

  // ---- TODO 3 -------------------------------------------------------------
  // 打印第 0 个点的 x、y、z、intensity。
  //
  // 目标输出类似这样：
  //   第 0 个点 (0.008, -0.004, 0.00006)  intensity 33.3
  std::cout << "打印第 0 个点的 x、y、z、intensity\n";
  // -------------------------------------------------------------------------

  // ---- TODO 4 -------------------------------------------------------------
  // 把点云原样存成 out/01_copy.pcd，二进制格式。
  //
  // 目标输出类似这样：
  //   存到 /.../out/01_copy.pcd
  std::cout << "把点云存成 out/01_copy.pcd\n";
  // -------------------------------------------------------------------------

  return 0;
}

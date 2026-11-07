// 各个程序共用的小工具：把 data/ 和 out/ 的路径拼出来。
// 路径在 CMakeLists.txt 里写死成编译期宏，所以程序在哪跑都能找到数据。
#pragma once

#include <string>
#include <filesystem>

#ifndef DATA_DIR
#define DATA_DIR "data"
#endif

#ifndef OUT_DIR
#define OUT_DIR "out"
#endif

inline std::string dataPath(const std::string& file) {
  return std::string(DATA_DIR) + "/" + file;
}

inline std::string outPath(const std::string& file) {
  std::filesystem::create_directories(OUT_DIR);
  return std::string(OUT_DIR) + "/" + file;
}

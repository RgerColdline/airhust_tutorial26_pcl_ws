#!/usr/bin/env python3
"""把一份 PCD 反复发到 /cloud_registered，用来在没有仿真时验证 3.7 的链路。

用法：
  rosrun tutorial_pcl_basic publish_demo.py [pcd 路径]
默认发 data/demo_room.pcd，10 Hz，一直发到 Ctrl-C。
"""
import struct
import sys
from pathlib import Path

import rospy
import sensor_msgs.point_cloud2 as pc2
from std_msgs.msg import Header


DEFAULT_PCD = Path(__file__).resolve().parents[3] / "data" / "demo_room.pcd"


def read_pcd_binary(path):
    """只读取本考核使用的四字段 float32、未压缩 binary PCD。"""
    with open(path, "rb") as f:
        header = {}
        header_bytes = 0
        while True:
            raw_line = f.readline(4097)
            header_bytes += len(raw_line)
            if not raw_line:
                raise ValueError("PCD 头不完整：缺少 DATA 行")
            if len(raw_line) > 4096 or header_bytes > 65536:
                raise ValueError("PCD 头过长或缺少 DATA 行")
            try:
                line = raw_line.decode("ascii").strip()
            except UnicodeDecodeError as exc:
                raise ValueError("PCD 头必须为 ASCII 文本") from exc
            if not line or line.startswith("#"):
                continue
            key, *values = line.split()
            if key in header:
                raise ValueError("PCD 头字段重复：" + key)
            header[key] = values
            if key == "DATA":
                break

        if header["DATA"] != ["binary"]:
            raise ValueError("仅支持 DATA binary；不支持 ASCII 或 binary_compressed")
        expected = {
            "FIELDS": ["x", "y", "z", "intensity"],
            "SIZE": ["4", "4", "4", "4"],
            "TYPE": ["F", "F", "F", "F"],
            "COUNT": ["1", "1", "1", "1"],
        }
        for key, values in expected.items():
            if header.get(key) != values:
                raise ValueError("不支持的 PCD 布局：需要 " + key + " " + " ".join(values))

        dimensions = {}
        for key in ("WIDTH", "HEIGHT", "POINTS"):
            values = header.get(key, [])
            if len(values) != 1 or not values[0].isascii() or not values[0].isdigit():
                raise ValueError("PCD 头需要一个正整数：" + key)
            dimensions[key] = int(values[0])
            if dimensions[key] <= 0:
                raise ValueError("PCD 点云不能为空：" + key)
        if dimensions["WIDTH"] * dimensions["HEIGHT"] != dimensions["POINTS"]:
            raise ValueError("PCD 的 WIDTH × HEIGHT 与 POINTS 不一致")

        payload_start = f.tell()
        f.seek(0, 2)
        payload_size = f.tell() - payload_start
        if payload_size != dimensions["POINTS"] * 16:
            raise ValueError("PCD 载荷长度与 POINTS 不一致，需要每点 16 字节")
        f.seek(payload_start)
        raw = f.read()
    return list(struct.iter_unpack("<ffff", raw))


def main():
    args = rospy.myargv(argv=sys.argv)
    if len(args) > 2:
        print("用法：publish_demo.py [pcd 路径]", file=sys.stderr)
        return 1
    path = args[1] if len(args) > 1 else DEFAULT_PCD
    try:
        points = read_pcd_binary(path)
    except (OSError, ValueError) as exc:
        print("读取 PCD 失败：{}：{}".format(path, exc), file=sys.stderr)
        return 1
    rospy.init_node("publish_demo")
    pub = rospy.Publisher("/cloud_registered", pc2.PointCloud2, queue_size=1)
    rospy.sleep(1.0)
    fields = [
        pc2.PointField("x", 0, pc2.PointField.FLOAT32, 1),
        pc2.PointField("y", 4, pc2.PointField.FLOAT32, 1),
        pc2.PointField("z", 8, pc2.PointField.FLOAT32, 1),
        pc2.PointField("intensity", 12, pc2.PointField.FLOAT32, 1),
    ]
    header = Header()
    header.frame_id = "world"
    rate = rospy.Rate(10)
    rospy.loginfo("发布 %s（%d 点）到 /cloud_registered", path, len(points))
    while not rospy.is_shutdown():
        header.stamp = rospy.Time.now()
        pub.publish(pc2.create_cloud(header, fields, points))
        rate.sleep()


if __name__ == "__main__":
    sys.exit(main())

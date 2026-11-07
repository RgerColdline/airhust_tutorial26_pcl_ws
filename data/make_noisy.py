#!/usr/bin/env python3
"""把 demo_room.pcd 弄"脏"：加冗余、加噪声，生成 data/demo_room_noisy.pcd。

- 冗余：每个点复制 3 份，各加 2mm 抖动 —— 模拟同一块墙面被反复扫到；
- 噪声：几千个随机点 + 一串从雷达位置往外散的离群点 —— 模拟打到灰尘、桨叶、远处的东西。
"""
import pathlib
import struct

import numpy as np

src = pathlib.Path(__file__).resolve().parent / "demo_room.pcd"
dst = pathlib.Path(__file__).resolve().parent / "demo_room_noisy.pcd"

with open(src, "rb") as f:
    hdr = {}
    while True:
        line = f.readline().decode().strip()
        k, *v = line.split()
        hdr[k] = v
        if k == "DATA":
            break
    raw = f.read()

n = int(hdr["POINTS"][0])
dt = np.dtype([("x", "<f4"), ("y", "<f4"), ("z", "<f4"), ("i", "<f4")])
a = np.frombuffer(raw, dtype=dt, count=n)
xyz = np.stack([a["x"], a["y"], a["z"]], 1).astype(np.float32)
inten = a["i"].astype(np.float32)

rng = np.random.default_rng(7)

# 冗余：复制 3 份，各加 2mm 抖动
p = np.repeat(xyz, 3, axis=0) + rng.normal(0, 0.002, (len(xyz) * 3, 3)).astype(np.float32)
it = np.repeat(inten, 3)

# 噪声 1：房间里随机撒点
k1 = 4000
noise1 = np.stack([
    rng.uniform(-0.5, 8.5, k1),
    rng.uniform(-0.5, 6.5, k1),
    rng.uniform(-0.5, 3.0, k1),
], 1).astype(np.float32)

# 噪声 2：从雷达位置 (0.4,0.3,1.2) 往外散的离群点
k2 = 800
dirs = rng.normal(size=(k2, 3))
dirs /= np.linalg.norm(dirs, axis=1, keepdims=True)
r = rng.uniform(1.0, 25.0, k2)[:, None]
noise2 = np.array([0.4, 0.3, 1.2], np.float32) + (dirs * r).astype(np.float32)

xyz_all = np.vstack([p, noise1, noise2]).astype(np.float32)
inten_all = np.concatenate([it, np.full(k1, 15.0, np.float32), np.full(k2, 10.0, np.float32)])

total = len(xyz_all)
header = (
    "# .PCD v0.7 - Point Cloud Data file format\nVERSION 0.7\n"
    "FIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n"
    f"WIDTH {total}\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS {total}\nDATA binary\n"
)
blob = b"".join(struct.pack("<ffff", float(x), float(y), float(z), float(i))
                for (x, y, z), i in zip(xyz_all, inten_all))
with open(dst, "wb") as f:
    f.write(header.encode("ascii"))
    f.write(blob)

print(f"写了 {dst}：{total} 点（原始 {n} 点）")

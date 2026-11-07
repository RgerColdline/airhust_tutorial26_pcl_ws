#!/usr/bin/env python3
"""生成 data/demo_room.pcd —— 一个假的室内场景，用来练 PCL。

为什么不用真实点云：真实 pcd 动辄几十上百 MB，来回传不方便，
而且里面有啥东西我们自己也不知道，出错了分不清是你写错了还是数据就那样。
这个场景是我们自己搭的，每个物体在哪、多大，脚本里写得清清楚楚。

只依赖 numpy。用法：
    python3 data/make_demo_scene.py
会覆盖 data/demo_room.pcd。

场景：8m x 6m x 2.6m 的房间（没画天花板），
      一张 0.75m 高的桌子，桌上三个零件，房间中间立两根柱子。
"""

import struct
from pathlib import Path

import numpy as np

rng = np.random.default_rng(20261031)

ROOM_X, ROOM_Y, ROOM_Z = 8.0, 6.0, 2.6

# 每个物体给一个“反射强度”，模拟不同材质的回波。
INTENSITY = {
    "floor": 35,
    "wall": 55,
    "table": 110,
    "object": 190,
    "pillar": 150,
}


def grid(origin, u, v, nu, nv):
    """在 origin + s*u + t*v 张成的矩形上按网格撒点。"""
    s = np.linspace(0.0, 1.0, nu)
    t = np.linspace(0.0, 1.0, nv)
    s, t = np.meshgrid(s, t, indexing="ij")
    pts = np.asarray(origin) + s[..., None] * np.asarray(u) + t[..., None] * np.asarray(v)
    return pts.reshape(-1, 3)


def box(x0, x1, y0, y1, z0, z1, step):
    """一个长方体的六个面（只撒外表面，点云本来也只能打到表面）。"""
    nx = max(2, int(round((x1 - x0) / step)) + 1)
    ny = max(2, int(round((y1 - y0) / step)) + 1)
    nz = max(2, int(round((z1 - z0) / step)) + 1)

    parts = [
        # 上下
        grid((x0, y0, z0), (x1 - x0, 0, 0), (0, y1 - y0, 0), nx, ny),
        grid((x0, y0, z1), (x1 - x0, 0, 0), (0, y1 - y0, 0), nx, ny),
        # 前后
        grid((x0, y0, z0), (x1 - x0, 0, 0), (0, 0, z1 - z0), nx, nz),
        grid((x0, y1, z0), (x1 - x0, 0, 0), (0, 0, z1 - z0), nx, nz),
        # 左右
        grid((x0, y0, z0), (0, y1 - y0, 0), (0, 0, z1 - z0), ny, nz),
        grid((x1, y0, z0), (0, y1 - y0, 0), (0, 0, z1 - z0), ny, nz),
    ]
    return np.vstack(parts)


def cylinder(cx, cy, r, z0, z1, step):
    """竖直圆柱的侧面 + 顶面。"""
    n_theta = max(12, int(round(2 * np.pi * r / step)))
    n_z = max(2, int(round((z1 - z0) / step)) + 1)
    theta = np.linspace(0.0, 2 * np.pi, n_theta, endpoint=False)
    z = np.linspace(z0, z1, n_z)
    t, zz = np.meshgrid(theta, z, indexing="ij")
    side = np.stack([cx + r * np.cos(t), cy + r * np.sin(t), zz], axis=-1).reshape(-1, 3)

    n_r = max(2, int(round(r / step)) + 1)
    rr = np.linspace(0.0, r, n_r)
    t2, r2 = np.meshgrid(theta, rr, indexing="ij")
    top = np.stack([cx + r2 * np.cos(t2), cy + r2 * np.sin(t2), np.full_like(r2, z1)], axis=-1).reshape(-1, 3)
    return np.vstack([side, top])


def main():
    clouds = []

    # 地面和四面墙
    clouds.append((grid((0, 0, 0), (ROOM_X, 0, 0), (0, ROOM_Y, 0), 201, 151), "floor"))
    clouds.append((grid((0, 0, 0), (ROOM_X, 0, 0), (0, 0, ROOM_Z), 201, 66), "wall"))
    clouds.append((grid((0, ROOM_Y, 0), (ROOM_X, 0, 0), (0, 0, ROOM_Z), 201, 66), "wall"))
    clouds.append((grid((0, 0, 0), (0, ROOM_Y, 0), (0, 0, ROOM_Z), 151, 66), "wall"))
    clouds.append((grid((ROOM_X, 0, 0), (0, ROOM_Y, 0), (0, 0, ROOM_Z), 151, 66), "wall"))

    # 桌子：桌面 + 四条腿（桌腿细，先用 1cm 的步长撒）
    clouds.append((box(2.0, 3.2, 2.0, 4.2, 0.70, 0.75, 0.02), "table"))
    for lx in (2.05, 3.15):
        for ly in (2.05, 4.15):
            clouds.append((box(lx - 0.02, lx + 0.02, ly - 0.02, ly + 0.02, 0.0, 0.70, 0.02), "table"))

    # 桌上的三个零件
    clouds.append((box(2.20, 2.35, 2.60, 2.75, 0.75, 0.90, 0.01), "object"))
    clouds.append((box(2.70, 2.90, 3.10, 3.25, 0.75, 1.00, 0.01), "object"))
    clouds.append((cylinder(2.90, 3.60, 0.10, 0.75, 1.05, 0.01), "object"))

    # 房间中间两根柱子，半径不一样，高到天花板
    clouds.append((cylinder(5.5, 1.5, 0.22, 0.0, ROOM_Z, 0.03), "pillar"))
    clouds.append((cylinder(5.5, 4.3, 0.35, 0.0, ROOM_Z, 0.03), "pillar"))

    pts = []
    inten = []
    for p, kind in clouds:
        pts.append(p)
        inten.append(np.full(len(p), INTENSITY[kind], dtype=np.float32))
    pts = np.vstack(pts)
    inten = np.concatenate(inten)

    # 传感器噪声：真实点云不会精确落在几何面上
    pts = pts + rng.normal(0.0, 0.004, pts.shape).astype(np.float32)
    inten = inten + rng.normal(0.0, 5.0, inten.shape).astype(np.float32)

    # 少量离群点：真实点云里总有那么几个“鬼”
    n_outlier = 300
    outliers = np.stack(
        [
            rng.uniform(0.0, ROOM_X, n_outlier),
            rng.uniform(0.0, ROOM_Y, n_outlier),
            rng.uniform(0.0, ROOM_Z, n_outlier),
        ],
        axis=1,
    )
    pts = np.vstack([pts, outliers]).astype(np.float32)
    inten = np.concatenate([inten, np.full(n_outlier, 80.0, dtype=np.float32)])

    n = len(pts)
    out = Path(__file__).resolve().parent / "demo_room.pcd"
    header = (
        "# .PCD v0.7 - Point Cloud Data file format\n"
        "VERSION 0.7\n"
        "FIELDS x y z intensity\n"
        "SIZE 4 4 4 4\n"
        "TYPE F F F F\n"
        "COUNT 1 1 1 1\n"
        f"WIDTH {n}\n"
        "HEIGHT 1\n"
        "VIEWPOINT 0 0 0 1 0 0 0\n"
        f"POINTS {n}\n"
        "DATA binary\n"
    )

    blob = bytearray()
    for (x, y, z), i in zip(pts, inten):
        blob += struct.pack("<ffff", x, y, z, i)

    with open(out, "wb") as f:
        f.write(header.encode("ascii"))
        f.write(blob)

    print(f"写了 {out}")
    print(f"点数 {n}")
    print(f"x [{pts[:, 0].min():.2f}, {pts[:, 0].max():.2f}]  "
          f"y [{pts[:, 1].min():.2f}, {pts[:, 1].max():.2f}]  "
          f"z [{pts[:, 2].min():.2f}, {pts[:, 2].max():.2f}]")


if __name__ == "__main__":
    main()

# GitHub 场景候选整理（2026-04-15）

本次整理目标：从 GitHub 上筛出的像素风景 / 科幻湖面 / 城市夜景方向里，挑出适合 `64×64` Glowxel 板子的可保留候选，并落到仓库里，避免后续再次重复检索与重复裁切。

## 保留目录

- 主推候选：`static/github-scene-finalists/priority/`
- 备选候选：`static/github-scene-finalists/backup/`
- 总览图：`static/github-scene-finalists/sheets/`
- 机器可读清单：`static/github-scene-finalists/manifest.json`

## 本轮主推

### 1. Sunset Ridge C

- 文件：`static/github-scene-finalists/priority/sunset_ridge_c.png`
- 来源：`hZambrzycki/aseprite-pixel-assets / 08-LandScape.aseprite`
- 保留原因：
  - 晚霞层次在 `64×64` 下仍然清楚
  - 下方树脊剪影足够稳，不会糊成一片
  - 适合先作为静态风景入库

### 2. SciFi Lake A

- 文件：`static/github-scene-finalists/priority/scifi_lake_a.png`
- 来源：`hZambrzycki/aseprite-pixel-assets / 07-SciFi.aseprite`
- 保留原因：
  - 大行星和倒影主体都还清楚
  - 科幻感强，板子上容易一眼看出题材
  - 适合后续做科幻风景分支

### 3. City Evening B

- 文件：`static/github-scene-finalists/priority/city_evening_b.png`
- 来源：`xliii/cityscape / evening`
- 保留原因：
  - 楼体和窗灯分层最稳
  - 加少量霓虹点后仍然不乱
  - 适合继续走赛博城市方向

## 备选说明

- `sunset_ridge_a.png`
  - 更保守的风景版本，可作为低风险替换项
- `scifi_lake_b.png`
  - 月亮和天空信息更多，适合想要更“外星夜景”的分支
- `city_night_d.png`
  - 更偏纯夜景 + 点状霓虹，适合更暗更赛博的方向
- `city_evening_f.png`
  - 窗灯信息更多，适合后续尝试城市动态化

## 当前结论

如果只保留 3 张作为下一步继续推进的候选，优先顺序如下：

1. `Sunset Ridge C`
2. `SciFi Lake A`
3. `City Evening B`

## 后续建议

- 若继续做 Glowxel 入库，优先把这 3 张接成统一命名的本地素材包
- 若继续做设备端表现优化，优先先验证：
  - 晚霞高亮区在实际板子上是否会过曝
  - 科幻湖面的青色高光在实机是否会丢层次
  - 城市夜景的暗部是否需要再抬一点亮度

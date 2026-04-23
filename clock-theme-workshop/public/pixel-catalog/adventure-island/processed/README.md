# Adventure Island 像素数据整理

这次已经把 `adventure-island` 里当前最直接能复用的几类素材统一转成像素数据了。

统一像素格式：

```json
["x", "y", "r", "g", "b"]
```

## 当前输出

- `master_higgins.json`
  - 来源：`character_01__master_higgins.png`
  - 内容：25 个主角切片
  - 处理方式：透明导出
  - 推荐直接使用：`recommendedAnimations.primary_run_cycle`

- `tina.json`
  - 来源：`character_06__tina.png`
  - 内容：7 个 Tina 角色帧
  - 处理方式：透明导出
  - 推荐直接使用：`recommendedAnimations.sheet_order`

- `items_and_objects.json`
  - 来源：`item_01__items_and_objects.png`
  - 内容：63 个组件
  - 处理方式：
    - `recommendedGroups.upper_section`：透明导出
    - `recommendedGroups.lower_pattern_rows`：不透明导出

- `beach_mountains_metatiles.json`
  - 来源：`character_04__beach_and_mountains_metatiles.png`
  - 内容：63 个非空 `32x32` 场景块
  - 处理方式：不透明导出
  - 推荐直接使用：`recommendedGroups.ground`、`recommendedGroups.clouds`

- `beach_mountains_chunks.json`
  - 来源：`character_03__beach_and_mountains_chunks.png`
  - 内容：17 个 `128x352` 整块场景
  - 处理方式：不透明导出
  - 推荐直接使用：`recommendedGroups.sheet_row_01`

- `forest_and_boss_rooms_chunks.json`
  - 来源：`map_01__forest_and_boss_rooms_chunks.png`
  - 内容：17 个 `128x352` 整块场景
  - 处理方式：不透明导出
  - 推荐直接使用：`recommendedGroups.sheet_row_01`

- `manifest.json`
  - 用途：把原图来源、输出文件和推荐组合统一整理起来

## 推荐组合

如果你现在只想先做一个最小可用的横版演示，建议直接这么拿：

- 主角动作：`master_higgins.json` -> `recommendedAnimations.primary_run_cycle`
- 地面/平台：`beach_mountains_metatiles.json` -> `recommendedGroups.ground`
- 云层变化：`beach_mountains_metatiles.json` -> `recommendedGroups.clouds`
- 场景装饰：`beach_mountains_metatiles.json` -> `recommendedGroups.props`
- 大块背景切换：`beach_mountains_chunks.json` 或 `forest_and_boss_rooms_chunks.json`
- 道具：`items_and_objects.json` -> `recommendedGroups.upper_section`
- NPC：`tina.json` -> `recommendedAnimations.sheet_order`

## 转换规则

- 主角和 NPC：按切片边框底色 + 整张图深蓝外底色做透明抠除
- 道具上半区：透明导出
- 道具下半区花纹条：不透明导出
- metatile 和 chunk：全部保留原始满块像素，不做透明抠除
- 底部版权说明、调色板说明、文字图例：不参与导出

## 原图分类

- 主角动作源图：`character_01__master_higgins.png`
- NPC：`character_06__tina.png`
- 道具和物件：`item_01__items_and_objects.png`
- 海滩场景 metatile：`character_04__beach_and_mountains_metatiles.png`
- 海滩整块参考图：`character_03__beach_and_mountains_chunks.png`
- 森林/Boss 房替换场景：`map_01__forest_and_boss_rooms_chunks.png`

## 生成方式

在仓库根目录运行：

```bash
python3 scripts/build_adventure_island_pixels.py
```

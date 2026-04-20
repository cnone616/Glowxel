# Battle City 素材落地包

更新日期：2026-04-20

## 当前结果

这套目录当前按 `Battle City / 坦克大战` 整理，已经把首批可直接用的正式画面、包装扫描、坦克/道具素材、关卡图和音效包落到本地。

当前已落地：

- `95` 个文件
- 总体积约 `173.26 MB`
- `01-official-reference`：`9` 个正式参考文件
- `02-official-screenshots`：`1` 张正式游戏内画面
- `03-sprites-tanks-items`：`2` 张核心 sprite 素材表
- `04-stage-tiles-maps/spriters-resource-stage-sheets`：`38` 张关卡素材表
- `04-stage-tiles-maps/vgmaps-stage-maps`：`35` 张关卡地图
- `05-audio`：`1` 个音效包
- `06-tech-notes`：`5` 个资料入口链接

## 关键入口

- [local-inventory.csv](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/local-inventory.csv)
  文件级总清单，记录本地路径、来源链接、文件大小。
- [source-manifest.csv](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/source-manifest.csv)
  高层来源表，方便后续继续补料。
- [download-checklist.md](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/download-checklist.md)
  当前完成状态与剩余缺口。

## 推荐先看

- 正式画面与包装参考：
  [battle-city-box-front.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/01-official-reference/battle-city-box-front.png)
  [battle-city-box-back.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/01-official-reference/battle-city-box-back.png)
  [battle-city-cart-front.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/01-official-reference/battle-city-cart-front.png)
  [battle-city-ingame-gamesdatabase.jpg](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/02-official-screenshots/battle-city-ingame-gamesdatabase.jpg)
- 坦克 / 基地 / 道具核心 sprite：
  [battle-city-general-sprites.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/03-sprites-tanks-items/battle-city-general-sprites.png)
  [battle-city-misc-sprites.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/03-sprites-tanks-items/battle-city-misc-sprites.png)
- 关卡地形和布局：
  [battle-city-stage-01-sheet.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/04-stage-tiles-maps/spriters-resource-stage-sheets/battle-city-stage-01-sheet.png)
  [battle-city-stage-01-map.png](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/04-stage-tiles-maps/vgmaps-stage-maps/battle-city-stage-01-map.png)
- 音效：
  [battle-city-sfx.zip](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/05-audio/battle-city-sfx.zip)

## 目录说明

- `01-official-reference`
  官方说明书、盒面、卡带、标题画面等正式资料。
- `02-official-screenshots`
  正式游戏内画面。
- `03-sprites-tanks-items`
  坦克、基地、道具、通用 sprite 核心表。
- `04-stage-tiles-maps/spriters-resource-stage-sheets`
  Spriters Resource 整套关卡素材表，适合拆地形 tile。
- `04-stage-tiles-maps/vgmaps-stage-maps`
  VGMaps 的单关地图，适合按关卡比对布局。
- `05-audio`
  音效压缩包。
- `06-tech-notes`
  机制、RAM、隐藏内容等后续分析入口。

## 当前限制

- 这次整理默认目标游戏是 `Battle City`。如果你后面给出的截图证实不是它，保留这套目录结构，替换来源即可重建一份。
- `GameFAQs` 图片页当前直抓会被 Cloudflare 挡住，所以这次没有把它作为主下载源。
- `VGMaps` 当前已落地 `Stage 01` 到 `Stage 35`，索引中未见与 `Stage 71` 对应的地图文件。

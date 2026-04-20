# Battle City 资料索引

更新日期：2026-04-20

## 识别结论

当前我没有直接拿到用户截图文件，但结合“坦克素材”描述和仓库内已有线索，当前高概率目标游戏是 `Battle City / 坦克大战 / バトルシティー`。

本地线索：

- 项目素材库已直接收录 `坦克大战 Battle City` 条目：
  [led-simulator-material-library.js](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/led-simulator-material-library.js)
- 项目内已有对应标题画面：
  [battle-city.png](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/material-library/nes-title-screens/battle-city.png)

如果后续用户补充的截图不是这款游戏，应按同一模板重新检索。

## 正式画面与官方扫描

优先级最高，适合做“正式画面”参考：

- Gaming Alexandria：
  <https://www.gamingalexandria.com/wp/2022/12/battle-city/>
  提供 Battle City 的条目页，并给出手册 PDF 与高分辨率原始扫描入口。
- Battle City 日版说明书 PDF：
  <https://www.gamingalexandria.com/highquality/fc/Battle%20City/Battle%20City%20-%20Manual.pdf>
  12 页，可直接看到玩法说明、角色说明、强化星星、敌方坦克分类、Construction 模式说明。
- Internet Archive 高精度扫描：
  <https://archive.org/details/battle-city-fc-hiresscans>
  包含 `Box Front/Back`、`Cart Front/Back/Top`、`Manual` 等原始扫描，是最接近“正式包装素材”的来源之一。
- GameFAQs 媒体页：
  <https://gamefaqs.gamespot.com/nes/562966-battle-city/images>
  汇总了 Box Shots、截图和视频，并且明确列出 `NES 66 screens`、`GB 15 screens`、`X1 8 screens`。
- Games Database 游戏内画面：
  <https://www.gamesdatabase.org/media/nintendo-nes/artwork-in-game/battle-city>
  可作为补充的“游戏内正式画面”索引页。

## 提取素材与坦克素材

最适合做还原和资源拆分：

- The Spriters Resource 游戏总页：
  <https://www.spriters-resource.com/nes/batcity/index.html>
  当前收录 `40` 张素材表，包含 `38` 张关卡背景和 `2` 张杂项素材。
- General Sprites：
  <https://www.spriters-resource.com/nes/batcity/sheet/60016/>
  这是最关键的一张，主坦克、敌方坦克、核心通用游戏 sprite 基本都应从这里取。
- Miscellaneous：
  <https://www.spriters-resource.com/nes/batcity/sheet/11756>
  这里有补充项，但页面评论也说明它不是完整主素材表，主 sprite 仍应以 `General Sprites` 为准。
- Stage 01 示例：
  <https://www.spriters-resource.com/nes/batcity/sheet/120304/>
  可直接看到单关卡背景拼图格式，适合验证砖墙、钢墙、水面、森林、冰面的 tile 布局。
- The Sounds Resource 音效包：
  <https://www.sounds-resource.com/nes/battlecity/sound/3710/>
  当前页面列出 `17` 个 `wav` 音效，适合补齐射击、爆炸、拾取等声音参考。

## 地图与关卡资料

适合做关卡复刻、正式画面对照和 tile 规则分析：

- VGMaps NES Atlas：
  <https://www.vgmaps.com/atlas/NES/index.htm>
  搜索结果已明确列出 `Battle City (J) Maps`，并显示 `Stage 1` 到 `Stage 35` 的关卡地图。
- GameFAQs 单关地图示例：
  <https://gamefaqs.gamespot.com/nes/562966-battle-city/map/6311-level-01-map>
  适合快速核对某一关的正式布局。

## 技术与补充资料

适合做玩法机制和隐藏素材补充，不属于“正式画面”，但很有用：

- Data Crystal 主页：
  <https://datacrystal.tcrf.net/wiki/Battle_City>
  收录 ROM 基本信息、关卡编辑器、Hack、转译资料等。
- Data Crystal RAM Map：
  <https://datacrystal.tcrf.net/wiki/Battle_City:RAM_map>
  适合做状态、敌人、关卡数据分析。
- VGFacts 未使用手枪强化说明：
  <https://www.vgfacts.com/game/battlecity/trivia-11553/>
  提到 NES 版本 sprite 数据里存在未使用的 `pistol power-up`。

## 直接可用的素材分类建议

如果你的目标是“把这游戏的正式画面和素材尽量找齐”，建议按下面顺序取：

1. 正式标题与包装：
   Gaming Alexandria + Internet Archive
2. 正式游戏截图：
   GameFAQs + Games Database
3. 坦克/基地/道具素材：
   Spriters Resource `General Sprites`
4. 关卡 tile 与地形：
   Spriters Resource 各 `Stage XX` + VGMaps
5. 音效：
   Sounds Resource
6. 机制补遗：
   Data Crystal + VGFacts

## 当前结论

如果你要的是 `Battle City`，这批来源已经足够支撑下面几类工作：

- 还原标题页和正式 UI
- 抽坦克 sprite、基地 sprite、道具 sprite
- 抽砖墙/钢墙/水/冰/树 tile
- 对照原版关卡布局
- 查说明书里的官方设定和玩法文案

本地落地整理目录：

- [Battle City 素材落地包](/Users/aflylong/Desktop/project/Glowxel/uniapp/output/battle-city-materials/README.md)

如果你下一步要我继续，我建议直接做两件事之一：

- 基于这些来源再帮你“筛一份最值得下载的精简包”
- 或者直接把“坦克、道具、地形、正式画面”分文件夹整理成下载清单

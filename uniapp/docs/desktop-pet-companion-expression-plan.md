# Plan: 桌面宠物陪伴感表情规划

**Generated**: 2026-04-25  
**Estimated Complexity**: High

## Overview
这份规划的目标不是单纯“让表情更多”，而是把桌面宠物从“会切表情的像素播放器”收口成“有呼吸感、会陪伴、偶尔轻微回应”的存在。

当前仓库已经有：
- 22 个现有眼神
- 时段池、特殊时间窗口、`sleepyAfterMs` 困倦偏置
- `behavior.expressionRhythm = slow / standard / lively`
- 本地嘴形映射和本地嘴位置调整

当前最需要补的不是新字段，而是“行为气质”：
- 眼神不能频繁变化
- 嘴形不能比眼神更乱
- 随机感要像人，不像抽卡机
- 嘴巴要比眼神更克制，跟随但不抢戏
- 一个眼神或情绪簇只能有少量候选嘴形，不能把整个嘴库都拿来随机
- 眨眼和左右看也必须一起收口，不能表情变慢了但视线还像扫描器
- 整体要偏轻松、愉悦、陪伴，而不是表演型角色

## Assumptions
- 本期仍坚持“眼神主导，嘴形辅助”。
- 正式设备合同仍只使用现有 `behavior.expressionRhythm`，不新增设备端嘴字段。
- 现有 7 个时段、2 个特殊窗口和 22 个眼神继续保留。
- 这份规划优先定义体验基线和调度结构，后续再分阶段落入 `uniapp` 预览和设备运行态。

## Success Criteria
- 连续看 3-5 分钟，不会产生“刚看清就又换脸”的急躁感。
- 嘴形不会独立乱跳，不会出现“眼神没变，嘴一直换”的违和感。
- 嘴巴左右跟随存在，但幅度很小，像轻微呼吸或轻点头，不像摆件晃动。
- 白天偏温和清醒，夜间偏收敛放松，整体情绪分布能让人感到“陪着我”，而不是“在给我演”。
- 强情绪不是常驻主旋律，只作为短暂点缀。

## Design Thesis

### 1. 稳定停留大于频繁切换
表情的第一职责是建立陪伴感，不是提高单位时间内的信息密度。  
如果一个眼神刚出现 2-4 秒就切走，用户会把它感知成“系统在播预设”，而不是“角色有状态”。

### 2. 随机不是乱跳，而是有段落
建议把“随机”拆成两层：
- 情绪段落层：一段时间内维持同一种大方向气质
- 段落内微变化层：只在相近眼神之间轻微游走

这样角色会显得像“有一阵子比较专注”“有一阵子有点开心”，而不是每次都重新掷骰子。

### 3. 嘴巴只补语气，不主导戏份
嘴的职责是轻轻补上语气：
- 平静时帮助脸更完整
- 开心时轻轻抬一下
- 惊讶时给一个小提示

嘴不负责独立讲故事，也不负责制造活跃感。

### 4. 微动作要像呼吸，不像动画
拟人感通常来自：
- 小幅
- 不完全同步
- 有轻微停顿
- 偶尔不动

而不是持续匀速来回摆。

## Recommended Runtime Model

### Layer 1: 情绪段落层
引入“情绪段落”概念，每个段落持续一段完整陪伴时间，再决定下一段气质。

建议起始值：

| 档位 | 单段落建议时长 |
| --- | --- |
| `slow` | `70-120s` |
| `standard` | `45-90s` |
| `lively` | `28-55s` |

段落切换时才允许跨大类情绪移动。  
段落内部只做小范围变化，不跨很远的情绪极性。

### Layer 2: 段落内微变化层
段落内部允许少量微变化，但必须是“同簇相邻变化”。

建议起始值：

| 档位 | 单个眼神最短停留 |
| --- | --- |
| `slow` | `12-18s` |
| `standard` | `8-14s` |
| `lively` | `6-10s` |

额外约束：
- 任一档位都不建议再低于 `6s`
- 同一眼神连续保留是允许的，不必强行换
- 强情绪眼神默认不连续停留超过 1 次

### Layer 3: 嘴形层
嘴形不单独调度，跟随当前段落气质和眼神簇。

建议规则：
- 嘴形只在眼神切换时评估
- 若新眼神仍在同一气质簇内，优先保留当前嘴形
- 只有跨簇时才切到新的主嘴形
- 嘴形切换频率必须小于等于眼神切换频率
- 每个簇的候选嘴形建议不超过 `2` 个

### Layer 4: 眨眼与看向层
眨眼和左右看也必须服务于“陪伴感”，不能再像高频扫描动画。

建议规则：
- 眨眼默认是稀疏事件，不是装饰性频闪
- 左右看是“偶尔看一眼”，不是持续巡逻
- 眨眼、看向、表情切换三者不能在短时间内连续叠加

建议起始值：

| 动作 | `slow` | `standard` | `lively` |
| --- | --- | --- | --- |
| 眨眼间隔 | `5.5-9s` | `4.5-7s` | `3.8-6s` |
| 单次看向保持 | `1.8-3.5s` | `1.4-2.8s` | `1.2-2.2s` |
| 两次看向最小间隔 | `7-12s` | `5-9s` | `4-7s` |

额外限制：
- 眼神刚切换后的 `2-3s` 内，尽量不要再触发明显看向
- 嘴巴不跟每次看向都动，只有超过位移阈值才轻跟

## Expression Cluster Plan

### A. 安静陪伴簇
**目标**：桌面宠物的默认基线，最像“陪着你”  
**眼神池**：`Normal / Focused / Determined / Unimpressed / Skeptic`  
**主嘴**：`flat`  
**次嘴**：`shortDash` 风格的更短平嘴，仅作为低概率细变体  
**出现占比建议**：
- 白天主力
- 全天总占比建议 `35%-45%`

### B. 温和开心簇
**目标**：轻松、亲近、心情不错，但不亢奋  
**眼神池**：`Happy / Glee / Awe`  
**主嘴**：`softSmile`  
**次嘴**：`flat`
**出现占比建议**：
- 上午到傍晚逐渐上升
- 全天总占比建议 `20%-28%`

### C. 轻困放松簇
**目标**：有点松弛、有点懒，但不消沉  
**眼神池**：`Sleepy / Confused / Squint`  
**主嘴**：`microOpen`  
**次嘴**：`none`
**出现占比建议**：
- 深夜、清晨、久无交互时明显增加

### D. 轻警觉簇
**目标**：让角色显得活着，但不能常驻紧张  
**眼神池**：`Suspicious / Annoyed`  
**主嘴**：`flat`
**次嘴**：`none`
**出现占比建议**：
- 只做短段落点缀
- 全天总占比建议 `8%-14%`

### E. 低落收敛簇
**目标**：保留情绪层次，但避免苦相常驻  
**眼神池**：`Sad / Worried / Frustrated`  
**主嘴**：`downArc`
**次嘴**：`flat`
**出现占比建议**：
- 深夜可少量出现
- 白天应严格限频

### F. 强瞬时反应簇
**目标**：偶尔让角色有神，但绝不常驻  
**眼神池**：`Excited / Heart / Surprised / Scared / Angry / Furious`  
**主嘴**：
- `Excited / Heart` -> `omega`
- `Surprised / Scared` -> `triangle`
- `Angry / Furious` -> `none`
**出现策略**：
- 只作为短瞬时段落
- 结束后优先回落到 `安静陪伴簇` 或 `温和开心簇`
- 全天总占比建议不超过 `5%-8%`

## Mouth Strategy Recommendation

### 运行时推荐策略：固定组合为主，稀疏变体为辅
正式运行不建议做“完全拆分拼接随机嘴”，因为那会很快变成机械轮播。  
建议使用：
- `固定主嘴映射`
- `同簇低概率次嘴变体`
- `跨簇才改主嘴`

### 生产嘴库建议
实验页可以继续保留很多样本，但正式运行建议只保留一套“生产嘴库”。

建议生产嘴库先收口为 6 个：
- `none`
- `flat`
- `softSmile`
- `microOpen`
- `downArc`
- `triangle`

保留为候补、不默认进运行时的：
- `omega`
- 更短平嘴变体

暂不建议进入正式随机的：
- 四方嘴
- 撇嘴
- 斜笑嘴
- 过强符号化 kaomoji 嘴

这意味着：
- `Normal` 今天仍然主要是 `flat`
- 但在安静簇长时间停留时，偶尔可切到更短更淡的平嘴
- `Happy` 仍然主要是 `softSmile`
- 但不会每次开心都强行变成完全一样的笑口

### 建议组合方向

| 眼神簇 | 主嘴 | 次嘴 | 说明 |
| --- | --- | --- | --- |
| 安静陪伴 | `flat` | 更短平嘴 | 最稳，最适合常驻 |
| 温和开心 | `softSmile` | `flat` | 开心但不过甜 |
| 轻困放松 | `microOpen` | `none` | 更像轻呼吸，不像张嘴 |
| 轻警觉 | `flat` | `downArc` | 只给一点点态度 |
| 低落收敛 | `downArc` | `flat` | 避免苦相过重 |
| 强瞬时反应 | `triangle / none` | `omega` | `omega` 只保留给极少数高兴瞬间 |

### 眼神到嘴形的数量约束
建议正式运行直接遵守这条硬限制：
- 每个眼神只允许 `1` 个主嘴
- 少数簇允许 `1` 个次嘴
- 单个眼神绝不超过 `2` 个候选嘴形

这样嘴库才是“真正在用”，不是“名字很多但运行没意义”。

## 正式运行收口版（v1 建议）

### 生产嘴库最终名单

正式运行建议分成两层：

- 常驻主库：`flat / softSmile / microOpen / downArc / triangle / none`
- 稀有候补：`omega`

实验页可保留但不建议进正式运行自动调度的：

- `shortDash` 或其他更短平嘴变体
- 四方嘴
- 撇嘴
- 斜笑嘴
- 过强符号化 kaomoji 嘴
- 任何需要超过 `3-5` 个像素主体块面才能看清的复杂嘴

### 生产库使用率目标

正式运行不只是“名单少”，还要“真正在用”。  
建议把运行期嘴形分布先收口到下面这个量级：

| 嘴形 | 建议运行占比 | 说明 |
| --- | --- | --- |
| `flat` | `34%-42%` | 常驻基线，负责大多数平静与轻专注状态 |
| `softSmile` | `18%-24%` | 温和开心主力，不做甜腻大笑 |
| `microOpen` | `12%-18%` | 困倦、放松、刚醒的呼吸感主力 |
| `none` | `8%-12%` | 眯眼、强压迫眼神时留白用 |
| `downArc` | `6%-10%` | 低落、担忧、挫败时少量出现 |
| `triangle` | `2%-4%` | 只留给惊讶、受惊这类短瞬时反应 |
| `omega` | `0%-2%` | 候补级，只在极少数兴奋或爱心时刻点一下 |

这意味着正式运行的目标不是“嘴越多越好”，而是：

- 大多数时间只看到 `flat / softSmile / microOpen`
- 少量时候看到 `none / downArc`
- 极少数瞬时才看到 `triangle / omega`

### 22 个眼神正式对应表

| 眼神 | 情绪簇 | 主嘴 | 次嘴 | 正式运行说明 |
| --- | --- | --- | --- | --- |
| `Normal` | 安静陪伴 | `flat` | 无 | 默认基线，最常驻 |
| `Focused` | 安静陪伴 | `flat` | 无 | 比 `Normal` 更利落，但嘴不需要变复杂 |
| `Determined` | 安静陪伴 | `flat` | 无 | 靠眼神立住，不靠嘴抢戏 |
| `Skeptic` | 安静陪伴 | `flat` | `none` | 怀疑感主要靠眼型，次嘴只做更收口版本 |
| `Unimpressed` | 安静陪伴 | `flat` | `none` | 常态仍以平嘴为主，避免太苦相 |
| `Happy` | 温和开心 | `softSmile` | `flat` | 优先轻笑，不要每次都笑得很满 |
| `Glee` | 温和开心 | `softSmile` | `flat` | 比 `Happy` 更亮，但仍然不进大嘴体系 |
| `Awe` | 温和开心 | `softSmile` | `triangle` | 主体仍温和，少量时刻可短暂偏惊叹 |
| `Sleepy` | 轻困放松 | `microOpen` | `none` | 像轻呼吸，不像张嘴打哈欠 |
| `Confused` | 轻困放松 | `microOpen` | `flat` | 迷糊感靠眼神，嘴只保留轻张口或收回 |
| `Squint` | 轻困放松 | `none` | `microOpen` | 眯眼时优先留白，次嘴只少量出现 |
| `Worried` | 低落收敛 | `downArc` | `flat` | 允许担忧，但不要常驻苦脸 |
| `Sad` | 低落收敛 | `downArc` | `flat` | 夜间可少量增加，白天应限频 |
| `Frustrated` | 低落收敛 | `downArc` | `none` | 挫败感更收着，避免拖成长时间苦相 |
| `Annoyed` | 轻警觉 | `flat` | `none` | 不耐烦主要靠眼神，嘴尽量克制 |
| `Suspicious` | 轻警觉 | `none` | `flat` | 先靠留白显得更盯视，再少量回到平嘴 |
| `Angry` | 强瞬时反应 | `none` | 无 | 生气不要用嘴加戏，靠眼神完成表达 |
| `Furious` | 强瞬时反应 | `none` | 无 | 只短时出现，结束后尽快回落 |
| `Surprised` | 强瞬时反应 | `triangle` | 无 | 只做短瞬时提示，不常驻 |
| `Scared` | 强瞬时反应 | `triangle` | `none` | 先三角，随后优先收回留白 |
| `Excited` | 强瞬时反应 | `omega` | `softSmile` | `omega` 只点一下，随后应回到轻笑 |
| `Heart` | 强瞬时反应 | `omega` | `softSmile` | 爱心态可以用 `omega`，但不应常驻刷屏 |

### 正式运行禁忌

- 禁止给全部眼神开放全嘴库随机
- 禁止让嘴形比眼神切换更快
- 禁止同一情绪段落里频繁从 `flat` 切到 `triangle / omega`
- 禁止为了“看起来丰富”保留大量几乎不会命中的实验嘴

### 嘴形切换节奏建议

嘴形不应该每次跟着眼神一起切。  
正式运行建议采用下面这套分层节奏：

| 层级 | `slow` | `standard` | `lively` |
| --- | --- | --- | --- |
| 情绪段落时长 | `70-120s` | `45-90s` | `28-55s` |
| 单眼神最短停留 | `12-18s` | `8-14s` | `6-10s` |
| 单嘴形稳定段 | `70-120s` | `45-90s` | `28-55s` |
| 段内次嘴评估 | 可不触发 | 最多 `0-1` 次 | 最多 `0-1` 次 |

解释如下：

- 先有情绪段落，再有眼神微变化
- 嘴形通常跟段落走，不跟每次眼神变化走
- 同一段里即使眼神轻微变化，嘴也大多数时间保持不变
- 少数簇允许在长段落中低概率切一次次嘴，但不建议超过一次

这比“每换一次眼神就评估一次嘴形”更接近陪伴感。

### 眨眼、左右看、微位移统一节奏

| 动作 | `slow` | `standard` | `lively` | 备注 |
| --- | --- | --- | --- | --- |
| 眨眼间隔 | `5.5-9s` | `4.5-7s` | `3.8-6s` | 连续两次眨眼之间避免再叠明显眼神切换 |
| 单次看向保持 | `1.8-3.5s` | `1.4-2.8s` | `1.2-2.2s` | 看一眼后要有停顿，不要扫来扫去 |
| 两次看向最小间隔 | `7-12s` | `5-9s` | `4-7s` | 保持“偶尔看一下”的感觉 |
| 眼神水平微移 | 常态 `0-1px`，少量 `2px` | 常态 `0-1px`，少量 `2px` | 常态 `0-1px`，少量 `2px` | 静止留白不低于总时长 `35%` |
| 嘴巴水平跟随 | 大多数 `0px`，少量 `1px` | 大多数 `0px`，少量 `1px` | 大多数 `0px`，少量 `1px` | 比眼神更慢、更小、更晚 |

统一原则：

- 眼神刚切换后的 `2-3s` 内，尽量不要再触发明显左右看
- 嘴巴只在眼神累计位移超过阈值时才同向跟 `1px`
- 嘴巴跟随建议带 `120-260ms` 轻延迟
- 眨眼、看向、换眼神不要连续叠在一两秒内一起发生

## Motion Plan

### 眼神位移
建议把眼神位移从“持续匀速摆动”改成“带停顿的小幅游走”。

建议起始值：
- 大多数时间：`0-1px`
- 少量时刻：`2px`
- 单次方向保持 `1.5-4s`
- 保持静止的时间不低于总时长的 `35%`
- 左右看默认优先短距离偏移，少用跨脸大幅扫视

### 嘴巴位移
嘴巴不应逐帧紧贴眼神，而应采用“阈值触发式跟随”：
- 眼神位移未超过阈值时，嘴巴保持不动
- 眼神累计位移达到阈值后，嘴巴才跟同方向挪 `1px`
- 嘴巴跟随应带 `120-260ms` 轻延迟

建议起始值：
- 嘴巴左右位移常态 `0px`
- 大多数跟随只到 `1px`
- 不建议出现连续来回摆 `2px+`
- 嘴巴纵向保持稳定，仅在特定嘴型上做极少量微调

### 关键限制
- 嘴巴摆幅必须明显小于眼神
- 嘴巴不独立做匀速摆动
- 嘴巴不因为“想更生动”而持续左右扫

## Time-of-Day Mood Guidance

### 深夜 `deepNight`
目标：安静、放松、低刺激  
建议主簇：`轻困放松 + 安静陪伴`  
禁忌：高频 `Excited / omega / triangle`

### 清晨 `earlyMorning`
目标：刚醒、轻松、慢慢进入状态  
建议主簇：`轻困放松 + 温和开心`

### 上午 `morning`
目标：清醒、稳、稍带正向能量  
建议主簇：`安静陪伴 + 温和开心 + 少量专注`

### 午间 `noon`
目标：微疲惫、收一点  
建议主簇：`安静陪伴 + 轻困放松`

### 下午 `afternoon`
目标：专注、轻回应  
建议主簇：`安静陪伴 + 轻警觉 + 少量温和开心`

### 傍晚 `evening`
目标：轻松、亲近、最有陪伴感  
建议主簇：`温和开心 + 安静陪伴`

### 夜间 `night`
目标：安静回落，避免过度表演  
建议主簇：`轻困放松 + 安静陪伴 + 少量低落收敛`

## Rhythm Recommendation

### 建议三档语义重定义
- `slow`：沉静陪伴，不主动打扰
- `standard`：自然陪伴，默认推荐
- `lively`：更有反应，但仍像角色，不像轮播

### 建议底线
- `lively` 也不要再低于 `6s` 一次切换
- `standard` 建议把主时段稳定在 `8-12s`
- `slow` 应明显进入“长停留”观感

### 建议段落切换逻辑
- 段落开始：选一个情绪簇
- 段落中间：只在该簇内做少量变化
- 段落结束：按时段和历史选择下一簇
- 若出现互动临时态：短暂插入，再缓慢回到原段落附近，而不是暴力切回

## Sprint 1: 体验合同与组合基线
**Goal**: 把“陪伴感”的体验目标从主观感觉收口成可实现的规则。  
**Demo/Validation**:
- 产出表情簇、嘴形簇、强弱情绪占比、位移约束表
- 能明确回答“什么叫像陪伴，什么叫像播放器”

### Task 1.1: 定义表情簇与主副嘴策略
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 将 22 个眼神重组为 5-6 个情绪簇，并为每个簇定义主嘴和低概率次嘴。
- **Dependencies**: 无
- **Acceptance Criteria**:
  - 每个眼神都有明确簇归属
  - 每个簇都有“主嘴 / 次嘴 / 禁忌嘴”
  - 每个眼神最终候选嘴形不超过 `2` 个
  - 强情绪默认不进入常驻簇
- **Validation**:
  - 对照当前 `EXPRESSION_MOUTH_MAP`，输出差异清单

### Task 1.2: 定义段落式调度模型
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 在现有时段池之上增加“情绪段落层”的概念，限制跨簇跳转频率。
- **Dependencies**: Task 1.1
- **Acceptance Criteria**:
  - 能区分“段落切换”和“段落内微变化”
  - 任意时段都有清晰主簇与稀有簇
- **Validation**:
  - 给出 1 分钟、3 分钟、10 分钟观察下的预期行为样例

### Task 1.3: 定义眨眼、看向与嘴部微动作约束
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 收口眼神漂移、眨眼频率、看向频率、嘴巴漂移、停顿与延迟规则。
- **Dependencies**: Task 1.1
- **Acceptance Criteria**:
  - 有明确“常态不动 / 小幅跟随 / 阈值触发”的规则
  - 嘴摆幅严格小于眼摆幅
  - 眨眼、看向不再高频抢戏
- **Validation**:
  - 用 64×64 预览逐帧录像观察是否仍像均匀来回摆

## Sprint 2: 调度表与概率系统收口
**Goal**: 把当前“时段池 + 去重惩罚”升级成“时段池 + 情绪段落 + 同簇微随机”。  
**Demo/Validation**:
- 同一时段连续观察 3 分钟，表情有生命感但不乱跳
- `standard` 档位不再出现 5 秒内频繁换脸

### Task 2.1: 调整最短停留与段落时长
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 重设三档最短停留时长，并新增段落级停留时间。
- **Dependencies**: Sprint 1
- **Acceptance Criteria**:
  - `lively` 仍有活力，但不机械
  - `standard` 成为默认推荐观感
- **Validation**:
  - 录屏检查 30 秒内切脸次数是否明显下降

### Task 2.2: 将随机改成簇内微随机
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 当前基于整个池的权重选择改为“先选簇，再选簇内眼神”。
- **Dependencies**: Task 2.1
- **Acceptance Criteria**:
  - 同段落内眼神变化具有连贯性
  - 不再频繁从开心直接跳到愤怒或惊吓
- **Validation**:
  - 对比当前实现日志和新实现日志的连续序列

### Task 2.3: 让困倦偏置更像“慢慢没精神”
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 保留现有 `sleepyAfterMs`，但将其影响扩展为“簇倾斜 + 停留变长 + 微动作减弱”。
- **Dependencies**: Task 2.2
- **Acceptance Criteria**:
  - 超时后不会绝对锁死
  - 观感是逐渐安静，而不是突然睡着
- **Validation**:
  - 模拟长时间无交互，观察 5 分钟状态变化

## Sprint 3: 嘴形联动与拟人微动作
**Goal**: 让嘴形真正变成“语气层”，而不是附着件。  
**Demo/Validation**:
- 嘴形和眼神一起显得自然
- 嘴不会像独立 UI 元件一样飘

### Task 3.1: 嘴形改为簇级主副映射
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 将当前单眼神固定映射提升为“簇级主嘴 + 稀有次嘴”的选择策略。
- **Dependencies**: Sprint 2
- **Acceptance Criteria**:
  - 同簇眼神切换时，嘴不一定跟着切
  - 嘴形总体变化次数明显少于眼神
- **Validation**:
  - 统计 1 分钟内眼神变化次数和嘴形变化次数

### Task 3.2: 引入阈值式小摆幅嘴部跟随
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 嘴巴跟随眼神，但只在跨过阈值时轻挪一步，并带小延迟。
- **Dependencies**: Task 3.1
- **Acceptance Criteria**:
  - 肉眼能感知“跟着脸走”
  - 但不会感知到“嘴在主动晃”
- **Validation**:
  - 录制慢速视频检查是否出现来回摆件感

### Task 3.3: 强情绪嘴形限频
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue)
- **Description**: 对 `omega / triangle / downArc / none` 设置稀有度与停留上限。
- **Dependencies**: Task 3.1
- **Acceptance Criteria**:
  - `omega / triangle` 只在少数情绪窗口出现
  - 夜间不再高频出现强符号嘴
- **Validation**:
  - 观察晚间时段 3 分钟回放

## Sprint 4: 预览与设备一致性验收
**Goal**: 让“陪伴感”不是只存在于页面 demo，而是设备端也一致成立。  
**Demo/Validation**:
- 同一时间段、同一节奏档位下，预览与设备有同向观感
- 手动模式、自动模式、长时间无交互都能成立

### Task 4.1: 对齐预览与设备调度层级
- **Location**: [pages/spirit-screen/spirit-screen.vue](/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/spirit-screen/spirit-screen.vue), `esp32-firmware/` 对应 eyes 调度链
- **Description**: 将段落层、簇内微随机和嘴部联动规则同步到设备真值。
- **Dependencies**: Sprint 3
- **Acceptance Criteria**:
  - 页面和设备不再出现一快一慢
  - 页面和设备情绪方向一致
- **Validation**:
  - 同时录预览与真机画面做并排比对

### Task 4.2: 建立陪伴感验收脚本
- **Location**: `docs/` 验收记录
- **Description**: 给出固定观察清单，而不是只靠主观感觉。
- **Dependencies**: Task 4.1
- **Acceptance Criteria**:
  - 至少覆盖 `白天 / 夜间 / 久无交互 / 手动切回自动`
  - 每项都有“通过 / 不通过”的具体标准
- **Validation**:
  - 用 3 分钟和 10 分钟双时长观察

## Testing Strategy
- 录制 30 秒、3 分钟、10 分钟三种长度的预览和真机视频。
- 重点检查“切换频率、强情绪占比、嘴巴摆幅、夜间是否安静、是否像播放器”。
- 单独验证：
  - `slow` 是否进入长停留
  - `standard` 是否最自然
  - `lively` 是否仍克制
  - `sleepyAfterMs` 是否是渐变偏置而非锁死

## Potential Risks & Gotchas
- 过度收紧后可能显得太呆。  
  处理：保留段落内微变化，不把角色做成静态头像。

- 开心嘴和惊讶嘴过多时，会立刻破坏陪伴感。  
  处理：限制强符号嘴的出现占比和连续停留次数。

- 眼神和嘴巴如果同步过头，会像同一个 UI 模块整体平移。  
  处理：嘴巴采用阈值式、滞后式跟随，不做逐帧同步。

- 只修预览，不修设备，会再次出现双真值。  
  处理：调度模型必须先定义“单一真源”。

- 只做固定映射会太死，只做完全拆分会太乱。  
  处理：正式运行建议采用“固定主嘴 + 稀有次嘴”的中间策略。

## Rollback Plan
- 若段落层实现后角色变得过于安静，先回退“段落时长”和“停留时长”，不回退整个簇模型。
- 若嘴部阈值跟随后观感更差，先退回“只跟方向、不跟每次位移”的最低耦合版本。
- 若设备端一时无法同步，可先在预览侧完成策略评审，但不得宣称已完成最终陪伴感收口。

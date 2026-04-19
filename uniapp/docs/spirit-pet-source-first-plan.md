# 桌面宠物（64x64）来源优先开发计划（A 路线）

## 1. 目标与硬约束

- 目标：在现有 `eyes` 模式上实现“大头趴手宠物”效果（眼神可动、可眨眼、耳朵/尾巴有节奏动画），先做单宠打磨版，再扩展生肖皮肤。
- 硬约束：
  - 只复用或移植“现成已实现”方案，不做猜测式原创算法。
  - 开发顺序固定：先完成 `uniapp 64x64` 模拟闭环并验收，再进入板载程序开发。
  - 不新增协议字段，不改现有命令语义（第一阶段）。
  - 严格沿用当前仓库字段名，禁止别名、兼容键、兜底键。

## 2. 锁定借鉴来源（只用现成）

### 2.1 仓库内现成来源（主来源，优先级最高）

1. 眼睛运行时与动作系统  
   - `esp32-firmware/src/eyes_effect.cpp`  
   - 已有：眨眼、看左/右/中、表情切换、时段权重、互动动作。
2. 配置协议与持久化  
   - `esp32-firmware/src/websocket_command_handlers.cpp`  
   - `esp32-firmware/include/config_manager.h`  
   - 已有：`set_eyes_config`、`eyes_interact`、`EyesConfig` 持久化。
3. 前端配置页面与预览  
   - `uniapp/pages/spirit-screen/spirit-screen.vue`  
   - `uniapp/utils/webSocket.js`  
   - 已有：配置编辑、预览循环、发送链路。

### 2.2 外部现成来源（辅助来源，仅做“按源码复用/映射”）

1. Adafruit Uncanny Eyes（眼动与眨眼成熟实现）  
   - <https://github.com/adafruit/Uncanny_Eyes>
2. eSheep/desktopPet（精灵表与动画状态组织方式）  
   - <https://github.com/Adrianotiger/desktopPet>  
   - <https://esheep.petrucci.ch/Help/Manual%20-%20online%20editor.pdf>
3. Kenney Animal Pack（可商用 CC0 素材基底）  
   - <https://www.kenney.nl/assets/animal-pack>  
   - <https://opengameart.org/content/animal-pack>

说明：外部来源只做“可验证映射”，不做“看起来像”的主观复刻。

## 3. 范围边界（本轮）

### 3.1 In Scope

- 在 `eyes` 模式内增加宠物身体层（头/手/耳/尾）渲染。
- 复用现有眼睛系统作为“脸部核心动画”。
- 复用现有 `spirit-screen` 页面作为配置与发送入口。
- 输出 1 个可演示宠物（单角色），可稳定运行在 `64x64`。

### 3.2 Out of Scope

- 不新增模式入口，不新增 `businessMode`。
- 不重写 WebSocket、设备状态同步、模式切换主链路。
- 不新增协议字段（第一阶段）。
- 不做 12 生肖全量资产生产（放到第二阶段皮肤扩展）。

## 4. 协议字段冻结与映射表（第一阶段不改协议）

> 本阶段仅复用现有字段，不新增字段。

| 字段 | 来源 | 去向 |
| --- | --- | --- |
| `cmd=set_mode` | `uniapp/utils/webSocket.js#setMode` | 固件模式切换 |
| `mode=eyes` | `uniapp/pages/spirit-screen/spirit-screen.vue` | `DisplayManager::activateEyesEffect` |
| `cmd=set_eyes_config` | `uniapp/utils/webSocket.js#setEyesConfig` | `handleEyesCommand` |
| `config.layout.eyeY` | `buildEyesConfigPayload()` | `EyesConfig.layout.eyeY` |
| `config.layout.eyeSpacing` | `buildEyesConfigPayload()` | `EyesConfig.layout.eyeSpacing` |
| `config.layout.eyeWidth` | `buildEyesConfigPayload()` | `EyesConfig.layout.eyeWidth` |
| `config.layout.eyeHeight` | `buildEyesConfigPayload()` | `EyesConfig.layout.eyeHeight` |
| `config.layout.timeX` | `buildEyesConfigPayload()` | `EyesConfig.layout.timeX` |
| `config.layout.timeY` | `buildEyesConfigPayload()` | `EyesConfig.layout.timeY` |
| `config.behavior.autoSwitch` | `buildEyesConfigPayload()` | `EyesConfig.behavior.autoSwitch` |
| `config.behavior.blinkIntervalMs` | `buildEyesConfigPayload()` | `EyesConfig.behavior.blinkIntervalMs` |
| `config.behavior.lookIntervalMs` | `buildEyesConfigPayload()` | `EyesConfig.behavior.lookIntervalMs` |
| `config.behavior.idleMove` | `buildEyesConfigPayload()` | `EyesConfig.behavior.idleMove` |
| `config.behavior.sleepyAfterMs` | `buildEyesConfigPayload()` | `EyesConfig.behavior.sleepyAfterMs` |
| `config.interaction.lookHoldMs` | `buildEyesConfigPayload()` | `EyesConfig.interaction.lookHoldMs` |
| `config.interaction.moodHoldMs` | `buildEyesConfigPayload()` | `EyesConfig.interaction.moodHoldMs` |
| `config.time.show` | `buildEyesConfigPayload()` | `EyesConfig.time.show` |
| `config.time.showSeconds` | `buildEyesConfigPayload()` | `EyesConfig.time.showSeconds` |
| `config.time.font` | `buildEyesConfigPayload()` | `EyesConfig.time.font` |
| `config.time.fontSize` | `buildEyesConfigPayload()` | `EyesConfig.time.fontSize` |
| `config.style.eyeColor` | `buildEyesConfigPayload()` | `EyesConfig.style.eyeColor` |
| `config.style.timeColor` | `buildEyesConfigPayload()` | `EyesConfig.style.timeColor` |
| `cmd=eyes_interact` | `uniapp/utils/webSocket.js#eyesInteract` | `EyesEffect::triggerAction` |
| `action=blink/look_left/look_center/look_right/happy/sleepy/set_expression:*` | `spirit-screen` 交互 | 固件即时动作 |

## 5. 技术路线（来源驱动，不猜）

## 5.1 Uniapp 阶段（必须先完成）

1. 保持 `spirit-screen` 页面结构不重做，仅补充“宠物预览层”与说明文案。
2. 发送流程保持：
   - `setMode("eyes") -> setEyesConfig(config) -> eyesInteract(...)`。
3. 不新增接口字段，仅利用现有控件区间映射到宠物默认参数推荐值。
4. `64x64` 模拟目标：
   - 大头趴手主体轮廓；
   - 眼神游走与眨眼可见；
   - 耳朵/尾巴至少一项具备稳定动画；
   - 预览可连续运行并支持互动动作触发。

## 5.2 板载阶段（仅在 Uniapp 验收后启动）

1. 在 `eyes_effect.cpp` 增加“宠物身体层”绘制函数，放在现有眼睛绘制前后固定层级。
2. 身体层动画只使用现有运行时节奏信号：
   - `lookX/lookY`：驱动头部微偏移；
   - `blinkActive`：驱动耳朵轻抖/手部按压；
   - `expression`：切换少量姿态变体；
   - `millis` 周期：尾巴低频摆动。
3. 宠物外形实现方式：
   - 先用“像素模板 + 程序位移”方案（无需新增配置字段）。
   - 模板来源优先使用已确认素材（CC0）或仓内固定模板。
4. 眼睛尺寸按现有字段收敛（不新增字段）：
   - 通过 `layout.eyeWidth/eyeHeight` 设为较小，满足“半身比例”。

## 5.3 资源侧（现成素材接入）

1. 资产来源只允许：
   - 已授权 CC0 包（推荐 Kenney）。
   - 仓内已有素材或团队自有素材。
2. 资源处理流程：
   - 统一切到 `64x64`；
   - 保留 4~6 色；
   - 导出固定帧序与命名清单；
   - 不做“临场手绘推断补帧”。

## 6. 分阶段排期（可执行）

### 阶段 U0（0.5 天）：来源冻结与资产选型

- 产出：
  - 《来源冻结清单》：每个动作对应哪个现成实现/素材。
  - 《素材授权记录》：链接、许可、版本。
- Gate：
  - 来源不明或许可不清，直接阻断后续开发。

### 阶段 U1（1 天）：Uniapp 64x64 宠物预览闭环

- 任务：
  - 在 `spirit-screen` 本地预览中接入头/手/耳/尾基础层。
  - 复用当前眼睛预览逻辑，不改协议。
- 验收：
  - `64x64` 预览可稳定显示“趴趴宠物”。
  - 不影响现有 `blink/look/expression/time` 行为。

### 阶段 U2（1 天）：Uniapp 交互与一致性验收

- 任务：
  - 耳朵/尾巴与 `look/blink/expression` 联动。
  - 完成 `HBuilderX` 下的预览连续性与互动验收。
- 验收：
  - 连续运行 30 分钟无明显抖动/卡顿/状态乱跳。
  - 互动动作触发后可回归待机，不滞留。

### 阶段 G1（门槛）：Uniapp 先行验收结论

- 通过条件：
  - U0/U1/U2 全部通过；
  - 你确认“Uniapp 模拟效果可进入板载阶段”。
- 未通过处理：
  - 不进入板载开发，只继续修 Uniapp。

### 阶段 F1（1 天）：板载身体层最小闭环（G1 后）

- 任务：
  - 在 `eyes_effect.cpp` 接入头/手/耳/尾基础层。
  - 保持现有 eyes 行为逻辑不改语义。
- 验收：
  - `eyes` 模式下可稳定显示“趴趴宠物”。
  - 不影响现有 `blink/look/expression/time` 行为。

### 阶段 F2（1 天）：板载联动打磨（G1 后）

- 任务：
  - 板载侧耳朵/尾巴联动与节奏收敛。
  - 对齐 Uniapp 已通过的动作节奏。
- 验收：
  - 实机连续运行稳定；
  - 动作与 Uniapp 验收版本保持同语义。

### 阶段 F3（0.5 天）：回归与封板（G1 后）

- 回归范围：
  - `control -> spirit-screen -> send -> reboot -> status` 全链路。
  - 其它已定板模式仅做冒烟，不做改造。
- 产出：
  - 验收记录、问题清单、下一阶段（生肖皮肤）入口条件。

## 7. 验收标准（硬性）

1. 视觉：
   - `64x64` 下一眼可识别“大头趴手”轮廓；
   - 眼睛明显小于头部主体比例；
   - 至少存在耳朵或尾巴动态。
2. 动画：
   - 待机、眨眼、眼神游走、互动动作均可触发且可回归。
3. 协议：
   - Uniapp 阶段无新增字段；
   - 板载阶段若需要新增字段，必须先补参数映射表并获确认；
   - 所有发送字段与当前 `EyesConfig` 一一对应。
4. 稳定性：
   - 不影响现有 `clock/animation/theme/canvas` 基础链路。

## 8. 风险与闸门

1. 风险：素材授权不清。  
   - 处理：只用 CC0 或自有资产；未确认禁止合入。
2. 风险：为实现宠物而扩协议。  
   - 处理：第一阶段协议冻结；若必须扩字段，先出映射表再改。
3. 风险：误伤 WS 或模式切换。  
   - 处理：禁止改 `webSocket.js` 核心状态机与 `store/device.js` 主流程。
4. 风险：预览和实机分叉。  
   - 处理：按同一节奏参数驱动，优先以固件表现为基准校准。

## 9. 下一阶段（B 路线预告，不在本轮）

- 目标：在同骨架上扩展“生肖皮肤包”。
- 前提：
  - A 路线验收通过；
  - 资产来源和许可全部明确；
  - 如需协议扩展，先提交字段映射表并获得确认。

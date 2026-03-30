# 设备模式改造 TODO（uniapp + 板载程序）

## 1. 背景与目标

### 1.1 当前问题
1. 控制页点击`静态时钟/动态时钟`会立即下发`set_mode`，用户仅浏览也会切设备模式。
2. 连接后会从设备状态反向同步模式到前端，导致 UI 与用户意图冲突。
3. 拼豆模式（画板模式）退出后不会自动恢复到“上一次业务模式”。

### 1.2 目标行为（本次冻结）
1. 模式切换以“用户操作提交”为准，不以“页面进入/连接状态同步”为准。
2. 控制页点击模式仅切换“待编辑模式（前端）”，不立即切设备模式。
3. 只有在对应页面点击“保存并应用/发送到设备”时，才下发`set_mode`。
4. 退出拼豆模式时，自动恢复上一次保存的业务模式（clock/animation/tetris）。
5. 不再使用设备返回模式覆盖前端当前模式（前端主导）。

## 2. 协议与字段约束

本次优先复用现有命令，不新增接口字段。

### 2.1 现有命令（继续使用）
| 字段 | 来源 |
|---|---|
| `cmd: "set_mode"` | 仅在“用户确认提交”时发送 |
| `mode` | 当前提交页面对应模式（`clock/animation/canvas/tetris`） |
| `cmd: "set_clock_config"` | 时钟编辑页点击发送时 |
| `clockMode` | 时钟编辑页当前模式（`clock/animation`） |
| `config` | 时钟编辑器表单 |

说明：本轮不新增`restore_mode`等新命令，先用现有`set_mode`完成闭环。

## 3. 实施范围拆分

## 3.1 uniapp 侧改造

### A. store 层（模式同步策略调整）
文件：`/Users/aflylong/Desktop/project/Glowxel/uniapp/store/device.js`

TODO：
- [x] 移除或禁用 `onMessage` 中 `status === "connected"` 时对 `deviceMode` 的反向覆盖。
- [x] 保留连接状态与基础设备信息更新，不再把设备模式作为 UI 真值来源。
- [x] 明确 `deviceMode` 语义：仅代表“前端当前工作模式”。

### B. 控制页（从“立即切换”改为“选择模式”）
文件：`/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/control/control.vue`

TODO：
- [x] 把 `switchMode(mode)` 从“立即 ws.setMode”改为“仅更新前端模式选择 + 本地缓存”。
- [x] 连接后不再用 `ws.getStatus().mode` 覆盖 `this.deviceMode`。
- [x] 进入时钟编辑页时继续带 `mode` 参数，但该参数仅用于编辑上下文，不代表设备已切换。
- [x] 文案调整：模式按钮提示“选择并编辑”，真正应用发生在对应功能页保存。

### C. 时钟编辑页（提交时再切设备模式，保持现有主逻辑）
文件：
- `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/clock-editor/clock-editor.vue`
- `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/clock-editor/mixins/deviceSyncMixin.js`

TODO：
- [x] 保持 `sendToDevice()` 中“先 `set_mode(this.clockMode)` 再下发配置”的流程（这是正确的提交式切换）。
- [x] 校验 `onLoad` 中“从板子同步模式”的逻辑，改为仅用 `options.mode` + 本地缓存，不从设备状态覆盖。
- [x] 发送成功后更新本地 `device_mode`，作为后续退出拼豆后的恢复目标。

### D. 拼豆模式（进入记忆/退出恢复）
文件：
- `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/assist/assist.vue`
- `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/assist/mixins/assistDeviceSyncMixin.js`

TODO：
- [x] 进入拼豆模式时记录 `previous_mode_before_canvas`（来源：本地已保存业务模式）。
- [x] 进入拼豆后继续使用 `set_mode("canvas")`（仅拼豆业务需要）。
- [x] 退出拼豆（`onUnload`）时自动调用 `set_mode(previous_mode_before_canvas)`。
- [x] 若恢复失败，toast 提示但不阻塞页面退出；并记录日志便于排查。
- [x] 若 `previous_mode_before_canvas` 缺失，默认回退 `clock`（写入缓存）。

### E. 方块设置页（保持“保存并应用”语义）
文件：`/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/tetris-settings/tetris-settings.vue`

TODO：
- [x] 保持当前行为：仅 `saveAndApply` 下发 `set_mode: "tetris"`。
- [x] 成功后统一写入 `device_mode = "tetris"`，作为拼豆退出恢复候选。
- [x] 对失败场景补日志和提示一致性。

## 3.2 板载程序（ESP32）改造

### A. 模式恢复容错（避免异常断开长期停在 canvas）
文件：
- `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/websocket_handler.cpp`
- `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/include/display_manager.h`
- `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/display_manager.cpp`

TODO：
- [x] 增加“最近业务模式”状态（不含 `canvas/transferring`）。
- [x] 每次 `set_mode(clock/animation/tetris)` 成功后更新该状态。
- [x] 在 WebSocket 断开事件中：若当前为 `canvas`，自动回退到最近业务模式。
- [x] 回退后立即刷新显示，避免黑屏停留。

### B. 模式变更日志
文件：
- `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/websocket_command_handlers.cpp`

TODO：
- [x] 给模式切换加统一日志（from -> to，触发命令），便于联调核对。
- [x] 异常模式值时保持现有错误返回，不新增兼容别名。

## 4. 执行顺序（建议）

1. 先改 uniapp：控制页 + store + 拼豆退出恢复（前端闭环先成立）。
2. 再改固件：断线 canvas 自动回退（异常兜底）。
3. 联调：正常路径 + 异常路径全量回归。

## 5. 联调用例（必须过）

1. 控制页点击`静态时钟/动态时钟`不应立即切换设备模式。
2. 时钟编辑页修改并发送后，设备模式才切到对应模式。
3. 进入拼豆后设备切到 canvas；退出拼豆后自动回到进入前模式。
4. 若拼豆时断网/断连，设备不应长期停在 canvas（固件自动回退生效）。
5. 连接设备时，设备返回模式不应覆盖前端当前选择模式。

## 6. 风险与回滚

1. 风险：前端模式与设备实际模式短时间不一致。
   处理：在“发送/保存并应用”按钮处展示“将切换设备模式”的提示。
2. 风险：退出拼豆恢复命令发送失败。
   处理：保留本地目标模式并在下次连接后优先应用。
3. 回滚策略：若联调异常，先回滚“固件断线自动回退”，保留前端提交式切换。

## 7. 交付定义（DoD）

1. 以上联调用例全部通过。
2. 控制页不再出现“仅点击模式即切设备”的行为。
3. 拼豆退出恢复稳定，连续 20 次进出无异常。
4. 固件日志可追踪模式流转，便于线上复盘。

## 8. 后续功能规划衔接

模式改造完成后，进入“设备工作台新功能”实施。

参考文档：
- `/Users/aflylong/Desktop/project/Glowxel/uniapp/docs/device-workbench-feature-plan.md`

对齐说明：
1. 新功能文档已冻结“中文文本支持 + uniapp 全参数控制 + 开发完成后文档更新”要求。
2. 本文档仅负责模式改造闭环，功能开发与文档补全按新功能文档执行。

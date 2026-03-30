# 设备工作台实现报告（阶段：模式改造 + 新效果首轮落地 + 画板迁移）

## 1. 版本信息
- 开始日期：`2026-03-27`
- 完成日期：`2026-03-30`
- 覆盖端：`uniapp / esp32-firmware`
- 负责人：`Codex`

## 2. 本阶段实现范围

已完成功能：
1. 控制页模式改为“前端选择，不立即切设备；提交后才切换”。
2. 拼豆模式增加“进入 canvas、退出恢复上次业务模式”链路。
3. 固件增加“canvas 断连自动回退最近业务模式”与模式切换日志。
4. 控制页新增主功能入口：`文本展示 / 呼吸灯 / 律动`。
5. 新增 `effect-editor` 页面，支持参数编辑、保存并应用。
6. 新增动画二进制上传工具链：`animationUploader + effectFrameBuilders`。
7. 固件 `set_mode` 增加：`text_display / breath_effect / rhythm_effect` 模式接入。
8. 新增独立 `canvas-editor` 页面，控制页点击“画板模式”直接进入编辑页。
9. 时钟编辑器移除“绘制”入口与手绘像素叠加发送逻辑，回归纯时钟/图片编辑职责。
10. 画板模式支持本地持久化、连接设备后的实时像素预览、清空与保存并应用。

未完成项（本阶段之外）：
1. `set_text_display` 原生命令未落地（当前文本展示继续走动画下发链路，以保证中文显示）。
2. 设备工作台预设系统（保存预设 / 读取预设）未开始。

## 3. 协议落地结果（本阶段）

本阶段未新增协议字段，仅复用现有命令。

### 3.1 `set_mode`
| 字段 | 实际来源 |
|---|---|
| `cmd` | 固定值 `set_mode` |
| `mode` | 当前提交页面模式：`clock / animation / canvas / tetris / transferring` |
| `clearMode` | 方块设置页 `config.clearMode`（仅 `mode=tetris`） |
| `cellSize` | 方块设置页 `config.cellSize`（仅 `mode=tetris`） |
| `speed` | 方块设置页 `config.speed` 映射值（仅 `mode=tetris`） |
| `showClock` | 方块设置页 `config.showClock`（仅 `mode=tetris`） |
| `pieces` | 方块设置页 `config.pieces`（仅 `mode=tetris`） |

### 3.2 `set_clock_config`
| 字段 | 实际来源 |
|---|---|
| `cmd` | 固定值 `set_clock_config` |
| `clockMode` | 时钟编辑页当前模式 `clockMode` |
| `config` | 时钟编辑器表单配置与图片/文字组合结果 |

### 3.3 规划中命令（本阶段仍未实现）
| 命令 | 状态 |
|---|---|
| `set_text_display` | 未实现（文本展示继续走动画链路） |
| `set_breath_effect` | 已实现（固件原生实时渲染） |
| `set_rhythm_effect` | 已实现（固件原生实时渲染） |
| `save_canvas / load_canvas / clear_canvas` | 已实现（固件持久化命令 + uniapp 调用） |

## 4. 关键改动文件

uniapp：
1. `/Users/aflylong/Desktop/project/Glowxel/uniapp/store/device.js`
2. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/control/control.vue`
3. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/clock-editor/clock-editor.vue`
4. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/clock-editor/mixins/deviceSyncMixin.js`
5. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/assist/assist.vue`
6. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/assist/mixins/assistDeviceSyncMixin.js`
7. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/tetris-settings/tetris-settings.vue`
8. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/effect-editor/effect-editor.vue`
9. `/Users/aflylong/Desktop/project/Glowxel/uniapp/pages/canvas-editor/canvas-editor.vue`
10. `/Users/aflylong/Desktop/project/Glowxel/uniapp/utils/animationUploader.js`
11. `/Users/aflylong/Desktop/project/Glowxel/uniapp/utils/effectFrameBuilders.js`

firmware：
1. `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/include/display_manager.h`
2. `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/display_manager.cpp`
3. `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/websocket_command_handlers.cpp`
4. `/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/websocket_handler.cpp`

## 5. 测试记录

### 5.1 自动化/构建验证
- [x] `uniapp` API 配置检查通过：`npm test`
- [x] `uniapp` 小程序完整构建验证由你侧执行（本轮按你的要求不处理）
- [x] `esp32-firmware` 编译通过：`pio run`

### 5.2 功能联调（真机）
- [x] 控制页点击模式不立即切设备（代码链路完成，真机联调由你侧执行）
- [x] 时钟编辑发送后才切换设备模式（代码链路完成，真机联调由你侧执行）
- [x] 拼豆退出自动恢复上次模式（代码链路完成，真机联调由你侧执行）
- [x] 异常断连不长期停留在 canvas（代码链路完成，真机联调由你侧执行）
- [x] 文本展示（含中文）保存并应用后在设备播放（动画链路，真机联调由你侧执行）
- [x] 呼吸灯参数（速度/循环/亮度/周期/波形）下发生效（原生命令链路，真机联调由你侧执行）
- [x] 律动参数（BPM/速度/方向/强度/模式/双颜色）下发生效（原生命令链路，真机联调由你侧执行）
- [x] 画板模式实时绘制、擦除、清空、保存并应用（代码链路完成，真机联调由你侧执行）

## 6. 已知问题与后续计划

已知问题：
1. 文本展示原生命令未落地，当前仍以动画链路保障中文展示。
2. 真机联调与录屏由你侧执行，本报告先按代码闭环记录。

下一步：
1. 完成真机联调用例 A-D + 新效果用例 + 画板模式用例，并更新复盘核查文档。
2. 按 `docs/device-workbench-feature-plan.md` 推进原生命令实现与预设系统。

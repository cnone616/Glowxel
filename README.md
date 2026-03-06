## MATRIX 项目导览（仓库级）

本仓库是一个面向“拼豆/LED 点阵像素画”的创作工具集合，包含：

- **移动端/多端创作工具**：`matrix-uniapp/`（uni-app + Vue3 + Pinia）
- **ESP32 固件**：`esp32-firmware/`（Arduino + PlatformIO，驱动 HUB75 64×64）
- **官网/在线设计站点**：`pixel-matrix-website/`（Vue3 + Vite）

根目录的`项目说明.md`是产品与技术背景文档；本 README 更偏向“如何跑起来 + 模块在哪里 + 三端如何交互”。

---

## 目录结构

```
matrix/
├── esp32-firmware/          # ESP32 固件 + 本地LED模拟器
├── matrix-uniapp/           # uni-app 多端应用（小程序/H5/App）
├── pixel-matrix-website/    # 官网/在线设计站点（Vite）
└── 项目说明.md              # 整体技术与规划说明
```

---

## 快速上手

### 1) `pixel-matrix-website/`（官网 / 在线编辑）

```bash
cd pixel-matrix-website
npm install
npm run dev
```

说明：
- 当前站点主要用于在线设计、预览与导出（项目内有 Mock 数据）；与 ESP32 的 WebSocket 直连在代码中尚未看到完整落地实现（文档有提及）。

---

### 2) `matrix-uniapp/`（小程序 / H5 / App）

推荐方式（官方工作流）：
- 使用 **HBuilderX** 导入 `matrix-uniapp/` 运行到微信开发者工具 / 浏览器 / 模拟器。

也提供 npm scripts（依赖于 uni-app CLI 环境）：

```bash
cd matrix-uniapp
npm install
npm run dev:h5
# 或：npm run dev:mp-weixin
```

关键实现入口：
- **像素画布**：`matrix-uniapp/components/PixelCanvas.vue`
- **设备控制页**：`matrix-uniapp/pages/control/control.vue`
- **闹钟编辑器**：`matrix-uniapp/pages/clock-editor/clock-editor.vue`
- **设备连接状态（Pinia）**：`matrix-uniapp/store/device.js`
- **与 ESP32 的 WebSocket 通信封装**：`matrix-uniapp/utils/webSocket.js`

---

### 3) `esp32-firmware/`（ESP32 固件：编译/烧录）

```bash
cd esp32-firmware
python3 -m platformio run --target upload
```

更多细节见：`esp32-firmware/开发环境说明.md`。

---

## ESP32 设备行为与网络模式

固件入口：`esp32-firmware/src/main.cpp`。

### WiFi 模式

- **优先 STA 模式**：若曾保存 WiFi（`Preferences` 命名空间 `"wifi"`），则尝试连接并通过 NTP 同步时间。
- **否则 AP 配网模式**：
  - **热点名（SSID）**：`LED-Matrix-Setup`
  - **热点密码**：`12345678`
  - **配置页**：连接热点后访问 `http://192.168.4.1/`
  - **DNS 劫持**：固件会启动 DNS 服务将任意域名解析到配网页（便于手机系统弹出“需要登录”式页面）。

### 显示模式

固件有两个模式（WebSocket 可切换）：
- **clock**：闹钟模式（默认）；支持时间/日期/星期的布局与颜色配置，支持背景像素图
- **canvas**：画板模式；接收并绘制像素（并在内存中维护 64×64 缓冲）

---

## 与 ESP32 通信（HTTP + WebSocket）

### HTTP（端口 80）

固件提供了一组 HTTP 路由（用于健康检查、状态查询、配网、简单控制）：
- `GET /status`：JSON 状态（ip、width/height、brightness、heap、uptime 等）
- `GET /test`：简单测试字符串
- `GET /`：配网页面（AP 模式下用于填 SSID/Password）
- `POST /save`：保存 WiFi 并重启
- `GET /clear-wifi`：清除已保存 WiFi
- 其他：清屏/亮度/文字等（详见 `src/main.cpp` 的 `setupServer()`）

### WebSocket（路径 `/ws`）

连接地址：
- `ws://<设备IP>/ws`

连接后，固件会发送一条文本消息，包含当前模式：
- `{"status":"connected","device":"LED-Matrix","mode":"clock|canvas"}`

#### 1) JSON 命令（文本帧）

常用命令形态：

```json
{ "cmd": "status" }
```

固件支持的命令（以 `src/main.cpp` 为准，下面列出最核心的）：
- `ping`：心跳
- `status`：查询设备状态（包含当前 mode）
- `set_mode`：切换模式
  - `{"cmd":"set_mode","mode":"clock"}` 或 `{"cmd":"set_mode","mode":"canvas"}`
- `set_clock_config`：设置闹钟布局（并持久化到 Flash）
- `clear`：清屏（canvas 模式还会清空画布缓冲）
- `brightness`：设置亮度（0-255）
- `text`：显示文字
- `pixel`：设置单个像素
- `image` / `image_sparse` / `image_chunk`：图片/像素绘制的几种格式（历史兼容与不同传输策略）

#### 2) 二进制像素流（Binary 帧，高效）

固件支持在 WebSocket 上接收二进制像素数据，格式：

- **每个像素 5 字节**：`x, y, r, g, b`
- `x/y/r/g/b` 均为 `uint8`（0-255）
- 一帧可携带多个像素，固件按 5 字节步进解析

uni-app 侧的实现参考：
- `matrix-uniapp/utils/webSocket.js` 的 `showImage()` / `sendPartialUpdate()`
- `matrix-uniapp/pages/control/control.vue` 中的“发送像素并等待确认”逻辑

固件确认机制（闹钟模式背景图场景）：
- 若接收像素后超过约 500ms 没再继续接收，会将像素数据保存到 `Preferences`，并广播：
  - `{"status":"ok","message":"pixels_received","count":<实际接收像素数>}`

---

## 本地 LED 模拟器（可选）

在 `esp32-firmware/` 下有一个本地模拟器页面：
- `led-simulator.html`
- `simulator-script.js`
- `simulator-style.css`

主要用途：
- 在电脑上模拟 64×64 LED 面板，做闹钟布局/图片预览的交互验证（不依赖真实硬件）。

---

## 备忘：常见开发关注点

- **像素画布数据结构**：在整体设计文档中倾向稀疏存储（例如 `Map<"x,y", "#rrggbb">`），以优化大画布性能。
- **小程序 Canvas 2D 差异**：需要关注 DPR、canvas 原生尺寸与逻辑尺寸对齐等问题（见根目录`项目说明.md`“当前问题”）。
- **ESP32 写 Flash 频率**：闹钟配置/像素持久化使用 `Preferences`；频繁写入会影响 Flash 寿命，建议在上层做节流/批量提交。


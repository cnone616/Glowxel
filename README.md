# Glowxel / RenLight 仓库导览

这是一个围绕像素创作、社区发布、设备联动和 ESP32 灯板显示的多端仓库，当前主要包含 5 个可运行模块：

- `website/`：官网与在线创作站点，已接入真实后端接口
- `uniapp/`：小程序/H5/App 端，包含创作、社区、设备控制、BLE 配网
- `server/`：Express + MySQL 后端
- `admin/`：运营后台
- `esp32-firmware/`：ESP32 固件，驱动 64x64 HUB75 面板

## 目录结构

```text
Glowxel/
├── admin/                  # 后台管理
├── docs/                   # 审核与说明文档
├── esp32-firmware/         # ESP32 固件
├── server/                 # 后端服务
├── uniapp/                 # 小程序 / H5 / App
├── website/                # 官网 / 在线编辑
├── README.md
└── DEPLOY.md
```

## 当前状态

- `website`：主链可构建
- `admin`：主链可构建
- `server`：核心入口与路由可做语法检查
- `esp32-firmware`：已可用 `pio run` 编译
- `uniapp`：以 HBuilderX / uni 生态为主，本仓当前没有内置 `uni` CLI

详细收口说明见：[当前仓库状态与收口说明](/Users/aflylong/Desktop/project/Glowxel/docs/当前仓库状态与收口说明.md)

## 快速开始

### `website/`

```bash
cd website
npm install
npm run dev
```

### `admin/`

```bash
cd admin
npm install
npm run dev
```

### `server/`

```bash
cd server
npm install
npm run dev
```

### `uniapp/`

推荐使用 HBuilderX 导入 `uniapp/` 运行。

如果本机已安装 `uni` CLI，也可以使用：

```bash
cd uniapp
npm install
npm run dev:h5
# 或 npm run dev:mp-weixin
```

### `esp32-firmware/`

当前环境已可直接使用 `pio`：

```bash
cd esp32-firmware
pio run
pio run --target upload
pio device monitor
```

更多细节见：[开发环境说明](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/开发环境说明.md)

## ESP32 联动现状

固件入口是 [main.cpp](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/main.cpp)。

### 配网方式

- 已保存 WiFi 时走 STA 模式
- 未保存 WiFi 时进入 BLE 配网模式
- BLE 设备名：`RenLight-Setup`
- 小程序配网页面：`uniapp/pages/ble-config/ble-config.vue`
- 小程序仍保留 `GET /clear-wifi` 作为“重置网络并重启”的设备复位入口

### 设备模式

当前模式为：

- `clock`
- `animation`
- `canvas`
- `transferring`

### HTTP / WebSocket

- HTTP 基础接口：`/status`、`/test`、`/upload`、`/brightness`、`/text`、`/clear-wifi`
- WebSocket 路径：`ws://<设备IP>/ws`
- 连接后返回当前模式，例如：

```json
{ "status": "connected", "device": "RenLight", "mode": "clock" }
```

## 文档说明

当前建议优先看这些文档：

- [当前仓库状态与收口说明](/Users/aflylong/Desktop/project/Glowxel/docs/当前仓库状态与收口说明.md)
- [开发环境说明](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/开发环境说明.md)
- [部署文档](/Users/aflylong/Desktop/project/Glowxel/DEPLOY.md)

`docs/` 下其余审计文档保留为历史过程记录，可能与当前代码不完全同步。

# Glowxel / RenLight 仓库导览

这是一个围绕像素创作、社区发布、设备联动和 ESP32 灯板显示的多端仓库，当前包含 5 个主模块：

- `website/`：官网与在线创作站
- `admin/`：后台管理
- `server/`：Express + MySQL 后端
- `uniapp/`：小程序 / H5 / App 客户端
- `esp32-firmware/`：ESP32 固件，驱动 64x64 HUB75 面板

## 目录结构

```text
Glowxel/
├── admin/
├── docs/
├── esp32-firmware/
├── server/
├── uniapp/
├── website/
├── DEPLOY.md
├── README.md
└── task.md
```

## 当前状态

当前已完成并验证的主链：

- `website`
  - `npm test` 通过
- `admin`
  - `npm test` 通过
- `server`
  - `npm test` 通过
  - 已补最小接口测试、管理员密码 `bcrypt` 迁移、数据库迁移脚本
- `esp32-firmware`
  - `platformio run` 通过
- `uniapp`
  - 代码主链完整
  - `npm test` 已补代码级 API 配置检查
  - 当前仓库未内置 `uni` CLI，本机未做微信小程序 / H5 编译验证

详细状态见 [当前仓库状态与收口说明](/Users/aflylong/Desktop/project/Glowxel/docs/当前仓库状态与收口说明.md)。

## 快速开始

### `website`

```bash
cd website
npm install
npm run dev
```

环境变量：

- `VITE_API_BASE_URL`
- `VITE_API_PROXY_TARGET`

### `admin`

```bash
cd admin
npm install
npm run dev
```

环境变量：

- `VITE_API_BASE_URL`
- `VITE_API_PROXY_TARGET`

### `server`

```bash
cd server
npm install
npm run dev
```

常用命令：

```bash
npm run migrate
npm run test
```

### `uniapp`

推荐使用 HBuilderX 导入 `uniapp/` 运行到微信开发者工具。

如果本机已安装 `uni` CLI，也可以使用：

```bash
cd uniapp
npm install
npm run dev:h5
# 或 npm run dev:mp-weixin
```

当前统一接口入口位于 [api.js](/Users/aflylong/Desktop/project/Glowxel/uniapp/config/api.js)。

### `esp32-firmware`

```bash
cd esp32-firmware
platformio run
platformio run --target upload
platformio device monitor
```

## 设备联动现状

固件入口是 [main.cpp](/Users/aflylong/Desktop/project/Glowxel/esp32-firmware/src/main.cpp)。

### 配网方式

- 已保存 WiFi 时走 STA 模式
- 未保存 WiFi 时进入 BLE 配网模式
- BLE 设备名：`RenLight-Setup`
- 小程序配网页面：`pages/ble-config/ble-config.vue`
- 设备仍保留 `GET /clear-wifi` 作为重置网络入口

### 设备模式

- `clock`
- `animation`
- `canvas`
- `transferring`

### WebSocket

- 路径：`ws://<设备IP>/ws`
- 连接后会返回当前模式
- 当前命令处理已按类别拆分，不再全部堆在单一大文件中

### 时钟编辑能力

- 完整时钟配置主链在 `uniapp`
- `website` 当前仅保留时钟能力说明页，不提供完整设备时钟编辑

## 工程化现状

已落地：

- 敏感信息已从仓库中清理
- `server` 管理员密码已改为 `bcrypt`
- `server` 已有最小测试集
- 已新增基础 CI 工作流
- `server` 已新增数据库迁移机制
- 多端接口地址入口已统一收口

## 推荐先看

- [当前仓库状态与收口说明](/Users/aflylong/Desktop/project/Glowxel/docs/当前仓库状态与收口说明.md)
- [环境配置约定](/Users/aflylong/Desktop/project/Glowxel/docs/env-config.md)
- [部署文档](/Users/aflylong/Desktop/project/Glowxel/DEPLOY.md)
- [项目架构与接口文档](/Users/aflylong/Desktop/project/Glowxel/docs/项目架构与接口文档.md)

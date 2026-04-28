# Glowxel 仓库导览

`Glowxel` 是当前仓库与工程代号，对外命名基线统一如下：

- 公司中文全称：`仁光数字科技有限公司`
- 公司英文名：`RenLight`
- 官网 / 网站端 / 小程序：`光格像素工坊`
- 设备正式名：`Glowxel PixelBoard`

本仓库围绕像素创作、社区发布、设备控制和 ESP32 灯板显示提供多端实现。

## 主模块

- `website/`
- `admin/`
- `server/`
- `uniapp/`
- `esp32-firmware/`

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
└── 硬件指南/
```

## 当前主线

当前仓库主线分为 4 个方向：

1. `website`
   - 官网、社区、在线创作、图纸工作台
2. `uniapp`
   - 用户主操作端，承接设备控制、编辑与同步
3. `esp32-firmware`
   - 设备真实显示、主题、效果与设备网页
4. `server + admin`
   - 数据、管理、上传与交付支撑

## 快速开始

### `website`

```bash
cd website
npm install
npm run dev
```

### `admin`

```bash
cd admin
npm install
npm run dev
```

### `server`

```bash
cd server
npm install
npm run dev
```

### `uniapp`

推荐使用 HBuilderX 导入 `uniapp/` 后运行到微信开发者工具或 H5。

### `esp32-firmware`

```bash
cd esp32-firmware
platformio run
platformio run --target upload
platformio device monitor
```

## 文档入口

优先查看：

1. [文档与页面索引.md](/Users/aflylong/Desktop/project/Glowxel/docs/文档与页面索引.md)
2. [项目架构与接口文档.md](/Users/aflylong/Desktop/project/Glowxel/docs/项目架构与接口文档.md)
3. [开发路线图.md](/Users/aflylong/Desktop/project/Glowxel/docs/开发路线图.md)
4. [env-config.md](/Users/aflylong/Desktop/project/Glowxel/docs/env-config.md)
5. [DEPLOY.md](/Users/aflylong/Desktop/project/Glowxel/DEPLOY.md)

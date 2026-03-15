# Glowxel 项目部署文档

## 服务器信息

| 项目 | 值 |
|------|-----|
| 服务器 IP | 175.178.153.146 |
| SSH 用户 | ubuntu |
| SSH 连接 | `ssh ubuntu@175.178.153.146` |
| 操作系统 | Ubuntu 22.04.5 LTS |
| 内网 IP | 10.1.0.5 |
| 磁盘 | 58.94GB（已用 12%） |

## 项目路径

| 模块 | 本地路径 | 服务器路径 |
|------|---------|-----------|
| 项目根目录 | `/Users/aflylong/Desktop/project/matrix` | `/home/ubuntu/glowxel-server` |
| 后端服务 | `server/` | `/home/ubuntu/glowxel-server` |
| 小程序 | `uniapp/` | 本地开发 |
| ESP32 固件 | `esp32-firmware/` | 本地编译烧录 |

## 数据库

| 项目 | 值 |
|------|-----|
| 类型 | MySQL |
| 端口 | 3306 |
| 用户 | root |
| 密码 | matrix123 |
| 数据库名 | matrix |

## 后端服务

| 项目 | 值 |
|------|-----|
| 端口 | 3000 |
| JWT 密钥 | glowxel_jwt_secret_2024 |
| 框架 | Express.js |

## 常用命令

```bash
# SSH 连接服务器
ssh ubuntu@175.178.153.146

# 更新代码并重启（最常用）
cd ~/glowxel-repo && git pull && cd ~/glowxel-server && pm2 restart glowxel-server

# 初始化数据库
cd ~/glowxel-server && node src/config/seed.js

# 查看服务状态
pm2 status
pm2 logs glowxel-server

# 重启服务
pm2 restart glowxel-server

# 停止服务
pm2 stop glowxel-server

# ESP32 固件编译
cd esp32-firmware && pio run

# ESP32 烧录
pio run --target upload
```

## 服务器目录结构

```
/home/ubuntu/
├── glowxel-repo/       # git 仓库（完整项目）
└── glowxel-server -> glowxel-repo/server/  # 软链接到 server 目录
```

## 本地推送流程

```bash
# 1. 本地提交
cd /Users/aflylong/Desktop/project/matrix
git add -A && git commit -m "描述改动"

# 2. 推送到 GitHub
git push

# 3. SSH 到服务器更新
ssh ubuntu@175.178.153.146
cd ~/glowxel-repo && git pull && cd ~/glowxel-server && pm2 restart glowxel-server
```

## API 地址

| 环境 | 地址 |
|------|-----|
| 服务器 | http://175.178.153.146:3000 |
| 本地开发 | http://localhost:3000 |
| 健康检查 | GET /api/health |
| 固件检查 | GET /api/firmware/check |

## 微信小程序

| 项目 | 值 |
|------|-----|
| AppID | 待配置 |
| 开发工具 | 微信开发者工具 |
| 框架 | uni-app + Vue 2 |

## ESP32 设备

| 项目 | 值 |
|------|-----|
| 芯片 | ESP32 |
| 屏幕 | 64x64 HUB75 LED 矩阵 |
| 固件版本 | 1.0.0 |
| BLE 配网名称 | Glowxel-Setup |
| BLE 配网密码 | 12345678 |
| WebSocket | ws://设备IP/ws |


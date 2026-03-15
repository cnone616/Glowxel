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

# 找项目路径
find /home -name "package.json" -path "*/server/*" 2>/dev/null

# 初始化数据库（在 server 目录下执行）
node src/config/seed.js

# 启动后端服务
npm start

# 查看服务状态
pm2 status    # 如果用 pm2
systemctl status glowxel  # 如果用 systemd

# ESP32 固件编译
cd esp32-firmware && pio run

# ESP32 烧录
pio run --target upload
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


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
| 项目根目录 | `/Users/aflylong/Desktop/project/Glowxel` | `/home/ubuntu/glowxel-repo` |
| 后端服务 | `server/` | `/home/ubuntu/glowxel-server` |
| 官网 | `website/` | `/var/www/glowxel` |
| 后台管理 | `admin/` | `/var/www/glowxel-admin` |
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

## 一键部署（推荐）

本地执行一条命令搞定所有更新：

```bash
# 全量更新（官网 + 后台 + 后端 + 数据库）
bash deploy.sh

# 只改了后端代码（最快，跳过前端构建）
bash deploy.sh --skip-website --skip-admin

# 只改了数据库结构
bash deploy.sh --db-only

# 只改了官网
bash deploy.sh --skip-admin

# 只改了后台管理
bash deploy.sh --skip-website
```

网站地址：https://glowxel.com
后台管理：https://admin.glowxel.com

> 首次部署后台需在服务器执行：`sudo certbot --nginx -d admin.glowxel.com`

---

## 常用命令

```bash
# SSH 连接服务器
ssh ubuntu@175.178.153.146

# 查看服务状态
pm2 status
pm2 logs glowxel-server

# 重启后端
pm2 restart glowxel-server

# 手动更新数据库结构
cd ~/glowxel-server && mysql -u root -pmatrix123 matrix < src/config/init.sql

# nginx 重载
sudo nginx -t && sudo nginx -s reload

# ESP32 固件编译
cd esp32-firmware && pio run

# ESP32 烧录
pio run --target upload
```

## 服务器目录结构

```
/home/ubuntu/
├── glowxel-repo/        # git 仓库（完整项目）
└── glowxel-server/      # 软链接到 glowxel-repo/server/

/var/www/
├── glowxel/             # 官网静态文件
└── glowxel-admin/       # 后台管理静态文件
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


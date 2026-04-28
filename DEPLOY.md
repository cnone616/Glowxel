# 项目部署文档

> 本文档已按 2026-03 当前仓库状态收口，重点保留线上部署和本地编译所需信息。

## 部署前准备

部署脚本不再保存任何生产环境真实地址或密码，执行前请先在本地 shell 中显式设置以下变量：

```bash
export DEPLOY_SERVER="ubuntu@your-server"
export DEPLOY_DB_NAME="your_db_name"
export DEPLOY_DB_USER="your_db_user"
export DEPLOY_DB_PASSWORD="your_db_password"
export SERVER_IP="your.server.ip"
export SERVER_USER="ubuntu"
export DOMAIN="your-domain.com"
```

> 如果变量缺失，部署脚本和 HTTPS 脚本会直接报错退出，不再使用仓库内默认值。

## 服务器信息

| 项目      | 值                         |
| --------- | -------------------------- |
| 服务器 IP | 通过 `SERVER_IP` 提供      |
| SSH 用户  | 通过 `SERVER_USER` 提供    |
| SSH 连接  | `ssh ${SERVER_USER}@${SERVER_IP}` |
| 操作系统  | Ubuntu 22.04 LTS           |
| 内网 IP   | 按实际服务器环境填写       |
| 磁盘      | 按实际服务器环境填写       |

## 项目路径

| 模块       | 本地路径                                  | 服务器路径                    |
| ---------- | ----------------------------------------- | ----------------------------- |
| 项目根目录 | `/Users/aflylong/Desktop/project/Glowxel` | `/home/ubuntu/glowxel-repo`   |
| 后端服务   | `server/`                                 | `/home/ubuntu/glowxel-server` |
| 官网       | `website/`                                | `/var/www/glowxel`            |
| 后台管理   | `admin/`                                  | `/var/www/glowxel-admin`      |
| 小程序     | `uniapp/`                                 | 本地开发                      |
| ESP32 固件 | `esp32-firmware/`                         | 本地编译烧录                  |

## 数据库

| 项目     | 值                      |
| -------- | ----------------------- |
| 类型     | MySQL                   |
| 端口     | 3306                    |
| 用户     | 通过 `DEPLOY_DB_USER` 提供 |
| 密码     | 通过 `DEPLOY_DB_PASSWORD` 提供 |
| 数据库名 | 通过 `DEPLOY_DB_NAME` 提供 |

## 后端服务

| 项目     | 值                    |
| -------- | --------------------- |
| 端口     | 3000                  |
| JWT 密钥 | 仅保存在服务端环境变量 |
| 框架     | Express.js            |

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

网站地址：按实际域名配置
后台管理：按实际域名配置

> 首次部署后台时，请把命令中的域名替换为你的后台域名，例如：`sudo certbot --nginx -d admin.your-domain.com`

---

## 常用命令

```bash
# SSH 连接服务器
ssh ${SERVER_USER}@${SERVER_IP}

# 查看服务状态
pm2 status
pm2 logs glowxel-server

# 重启后端
pm2 restart glowxel-server

# 手动更新数据库结构
cd ~/glowxel-server && mysql -u "${DEPLOY_DB_USER}" -p"${DEPLOY_DB_PASSWORD}" "${DEPLOY_DB_NAME}" < src/config/init.sql

# nginx 重载
sudo nginx -t && sudo nginx -s reload

# ESP32 固件编译
cd esp32-firmware && pio run

# ESP32 烧录
cd esp32-firmware && pio run --target upload
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

| 环境     | 地址                                  |
| -------- | ------------------------------------- |
| 服务器   | `http://${SERVER_IP}:3000`            |
| 本地开发 | `http://localhost:3000`               |
| 健康检查 | `GET /api/health`                     |
| 固件检查 | `GET /api/firmware/check`             |

## 微信小程序

| 项目     | 值              |
| -------- | --------------- |
| AppID    | 待配置          |
| 开发工具 | 微信开发者工具  |
| 框架     | uni-app         |

## ESP32 设备

| 项目         | 值                   |
| ------------ | -------------------- |
| 芯片         | ESP32                |
| 屏幕         | 64x64 HUB75 LED 矩阵 |
| 固件版本     | 1.0.0                |
| 热点配网名称 | Glowxel PixelBoard-设备序列号 |
| 配网方式     | AP 热点门户写入 WiFi |
| WebSocket    | ws://设备IP/ws       |

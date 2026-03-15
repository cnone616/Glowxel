#!/bin/bash
# Glowxel 服务器一键部署脚本
# 用法: ssh ubuntu@175.178.153.146 'bash -s' < deploy.sh
# 或在服务器上: bash deploy.sh

set -e

APP_DIR="/home/ubuntu/glowxel-server"
REPO_URL="git@github.com:cnone616/Glowxel.git"
BRANCH="main"

echo "=== Glowxel 服务端部署 ==="
echo ""

# 1. 检查是否已有 git 仓库
if [ -d "$APP_DIR/.git" ]; then
  echo "[1/5] 拉取最新代码..."
  cd "$APP_DIR"
  git fetch origin
  git reset --hard origin/$BRANCH
else
  echo "[1/5] 首次部署，克隆仓库..."
  if [ -d "$APP_DIR" ]; then
    # 备份旧的 .env
    cp "$APP_DIR/.env" /tmp/glowxel-env-backup 2>/dev/null || true
    rm -rf "$APP_DIR"
  fi
  git clone -b $BRANCH "$REPO_URL" "$APP_DIR"
  cd "$APP_DIR"
  # 只保留 server 目录的内容
  if [ -d "server" ]; then
    # 仓库包含多个子目录，进入 server
    APP_DIR="$APP_DIR/server"
    cd "$APP_DIR"
  fi
  # 恢复 .env
  cp /tmp/glowxel-env-backup "$APP_DIR/.env" 2>/dev/null || true
fi

echo "[2/5] 安装依赖..."
cd "$APP_DIR"
# 如果是 monorepo，进入 server 目录
if [ -f "server/package.json" ]; then
  cd server
fi
npm install --production 2>&1 | tail -3

echo "[3/5] 初始化数据库..."
node src/config/seed.js 2>&1 || echo "  ! seed 有警告，继续部署"

echo "[4/5] 重启服务..."
# 检查 pm2 是否安装
if command -v pm2 &> /dev/null; then
  pm2 restart glowxel-server 2>/dev/null || pm2 start src/app.js --name glowxel-server
  pm2 save
  echo "  ✓ pm2 已重启"
else
  echo "  安装 pm2..."
  sudo npm install -g pm2
  pm2 start src/app.js --name glowxel-server
  pm2 save
  pm2 startup | tail -1 | bash 2>/dev/null || true
  echo "  ✓ pm2 已启动"
fi

echo "[5/5] 检查服务状态..."
sleep 2
curl -s http://localhost:3000/api/health | head -1 && echo "" || echo "  ! 服务可能未启动"
pm2 status

echo ""
echo "=== 部署完成 ==="


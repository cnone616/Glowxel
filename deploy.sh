#!/bin/bash
# Glowxel 一键更新部署脚本
# 本地执行: bash deploy.sh
# 可选参数: bash deploy.sh --skip-website  跳过官网构建
#           bash deploy.sh --skip-admin    跳过后台构建
#           bash deploy.sh --db-only       只更新数据库

set -e

SERVER="ubuntu@175.178.153.146"
SKIP_WEBSITE=false
SKIP_ADMIN=false
DB_ONLY=false

for arg in "$@"; do
  case $arg in
    --skip-website) SKIP_WEBSITE=true ;;
    --skip-admin)   SKIP_ADMIN=true ;;
    --db-only)      DB_ONLY=true; SKIP_WEBSITE=true; SKIP_ADMIN=true ;;
  esac
done

echo "🚀 Glowxel 一键部署"
echo "================================"

# ── 1. 推送代码 ──────────────────────────────────────────────
echo "[1/5] 推送代码到 GitHub..."
git add -A
git diff --cached --quiet && echo "  无新改动，跳过 commit" || git commit -m "deploy: $(date '+%Y-%m-%d %H:%M')"
git push
echo "  ✓ 代码已推送"

# ── 2. 服务器拉取代码 + 更新数据库 + 重启后端 ────────────────
echo "[2/5] 更新服务器代码 + 数据库 + 重启后端..."
ssh $SERVER << 'REMOTE'
set -e
cd ~/glowxel-repo && git pull
cd ~/glowxel-server
# 数据库结构更新（IF NOT EXISTS 安全执行，不影响已有数据）
mysql -u root -pmatrix123 matrix < src/config/init.sql 2>/dev/null
npm install --production --silent
pm2 restart glowxel-server 2>/dev/null || pm2 start src/app.js --name glowxel-server
pm2 save --silent
echo "  ✓ 后端已重启"
REMOTE

# ── 3. 构建官网 ───────────────────────────────────────────────
if [ "$SKIP_WEBSITE" = false ]; then
  echo "[3/5] 构建官网..."
  cd "$(dirname "$0")/website"
  npm install --silent
  npm run build
  ssh $SERVER "sudo cp -r /dev/stdin /tmp/website-dist.tar" < <(tar -czf - -C dist .)
  ssh $SERVER << 'REMOTE'
sudo mkdir -p /var/www/glowxel
cd /tmp && tar -xzf website-dist.tar -C /var/www/glowxel
sudo chown -R www-data:www-data /var/www/glowxel
rm -f /tmp/website-dist.tar
echo "  ✓ 官网已更新"
REMOTE
  cd "$(dirname "$0")"
else
  echo "[3/5] 跳过官网构建"
fi

# ── 4. 构建后台管理 ───────────────────────────────────────────
if [ "$SKIP_ADMIN" = false ]; then
  echo "[4/5] 构建后台管理..."
  cd "$(dirname "$0")/admin"
  npm install --silent
  npm run build
  ssh $SERVER << 'REMOTE'
sudo mkdir -p /var/www/glowxel-admin
REMOTE
  scp -r dist/* $SERVER:/tmp/admin-dist/
  ssh $SERVER << 'REMOTE'
sudo cp -r /tmp/admin-dist/* /var/www/glowxel-admin/
sudo chown -R www-data:www-data /var/www/glowxel-admin
rm -rf /tmp/admin-dist
echo "  ✓ 后台管理已更新"
REMOTE
  cd "$(dirname "$0")"
else
  echo "[4/5] 跳过后台管理构建"
fi

# ── 5. 检查 nginx admin 配置 + reload ────────────────────────
echo "[5/5] 检查 nginx 配置..."
ssh $SERVER << 'REMOTE'
# 如果 admin nginx 配置不存在，自动创建
if [ ! -f /etc/nginx/sites-enabled/glowxel-admin ]; then
  echo "  创建 admin.glowxel.com nginx 配置..."
  sudo tee /etc/nginx/sites-available/glowxel-admin > /dev/null << 'NGINX'
server {
    server_name admin.glowxel.com;
    root /var/www/glowxel-admin;
    index index.html;

    # SPA 路由支持
    location / {
        try_files $uri $uri/ /index.html;
    }

    # API 代理（和官网共用同一个后端）
    location /api/ {
        proxy_pass http://localhost:3000/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    listen 80;
}
NGINX
  sudo ln -sf /etc/nginx/sites-available/glowxel-admin /etc/nginx/sites-enabled/glowxel-admin
  echo "  ✓ nginx 配置已创建，请手动执行 certbot 申请 HTTPS："
  echo "    sudo certbot --nginx -d admin.glowxel.com"
fi
sudo nginx -t && sudo nginx -s reload
echo "  ✓ nginx 已 reload"
REMOTE

echo ""
echo "================================"
echo "✅ 部署完成！"
echo "   官网:   https://glowxel.com"
echo "   后台:   http://admin.glowxel.com (首次需申请 HTTPS)"
echo "   API:    https://glowxel.com/api/health"


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


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
  tar -czf /tmp/website-dist.tar.gz -C dist .
  scp /tmp/website-dist.tar.gz $SERVER:/tmp/
  ssh $SERVER << 'REMOTE'
sudo mkdir -p /var/www/glowxel
sudo tar -xzf /tmp/website-dist.tar.gz -C /var/www/glowxel
sudo chown -R www-data:www-data /var/www/glowxel
rm -f /tmp/website-dist.tar.gz
echo "  ✓ 官网已更新"
REMOTE
  rm -f /tmp/website-dist.tar.gz
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
  tar -czf /tmp/admin-dist.tar.gz -C dist .
  scp /tmp/admin-dist.tar.gz $SERVER:/tmp/
  ssh $SERVER << 'REMOTE'
sudo mkdir -p /var/www/glowxel-admin
sudo tar -xzf /tmp/admin-dist.tar.gz -C /var/www/glowxel-admin
sudo chown -R www-data:www-data /var/www/glowxel-admin
rm -f /tmp/admin-dist.tar.gz
echo "  ✓ 后台管理已更新"
REMOTE
  rm -f /tmp/admin-dist.tar.gz
  cd "$(dirname "$0")"
else
  echo "[4/5] 跳过后台管理构建"
fi

# ── 5. 检查 nginx admin 配置 + reload ────────────────────────
echo "[5/5] 检查 nginx 配置..."
ssh $SERVER << 'REMOTE'
if [ ! -f /etc/nginx/sites-available/glowxel-admin ]; then
  echo "  创建 admin.glowxel.com nginx 配置..."
  sudo tee /etc/nginx/sites-available/glowxel-admin > /dev/null << 'NGINX'
server {
    listen 80;
    server_name admin.glowxel.com;
    root /var/www/glowxel-admin;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /api/ {
        proxy_pass http://localhost:3000/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
NGINX
  sudo ln -sf /etc/nginx/sites-available/glowxel-admin /etc/nginx/sites-enabled/glowxel-admin
fi
sudo nginx -t && sudo nginx -s reload
echo "  ✓ nginx 已 reload"
REMOTE

echo ""
echo "================================"
echo "✅ 部署完成！"
echo "   官网:   https://glowxel.com"
echo "   后台:   http://admin.glowxel.com"
echo "   API:    https://glowxel.com/api/health"


#!/bin/bash
# Glowxel 官网部署脚本

set -e

echo "🚀 开始部署 Glowxel 官网..."

# 服务器信息
SERVER_IP="175.178.153.146"
SERVER_USER="ubuntu"
DOMAIN="glowxel.com"

# 1. 本地构建网站
echo "📦 构建网站..."
cd website
npm install
npm run build
cd ..

# 2. 上传到服务器
echo "📤 上传文件到服务器..."
scp -r website/dist ${SERVER_USER}@${SERVER_IP}:/tmp/glowxel-website

# 3. 在服务器上配置 Nginx
echo "⚙️  配置服务器..."
ssh ${SERVER_USER}@${SERVER_IP} << 'ENDSSH'

# 安装 Nginx（如果未安装）
if ! command -v nginx &> /dev/null; then
    echo "安装 Nginx..."
    sudo apt update
    sudo apt install -y nginx
fi

# 创建网站目录
sudo mkdir -p /var/www/glowxel
sudo rm -rf /var/www/glowxel/*
sudo mv /tmp/glowxel-website/* /var/www/glowxel/
sudo chown -R www-data:www-data /var/www/glowxel

# 创建 Nginx 配置
sudo tee /etc/nginx/sites-available/glowxel.com > /dev/null << 'EOF'
server {
    listen 80;
    server_name glowxel.com www.glowxel.com;
    
    root /var/www/glowxel;
    index index.html;
    
    # 日志
    access_log /var/log/nginx/glowxel-access.log;
    error_log /var/log/nginx/glowxel-error.log;
    
    # Gzip 压缩
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
    
    # 静态文件缓存
    location ~* \.(jpg|jpeg|png|gif|ico|css|js|svg|woff|woff2|ttf|eot)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
    
    # Vue Router history 模式支持
    location / {
        try_files $uri $uri/ /index.html;
    }
    
    # API 代理到后端
    location /api/ {
        proxy_pass http://localhost:3000/api/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_cache_bypass $http_upgrade;
    }
}
EOF

# 启用站点
sudo ln -sf /etc/nginx/sites-available/glowxel.com /etc/nginx/sites-enabled/

# 删除默认站点（可选）
sudo rm -f /etc/nginx/sites-enabled/default

# 测试配置
sudo nginx -t

# 重启 Nginx
sudo systemctl restart nginx
sudo systemctl enable nginx

echo "✅ Nginx 配置完成"

# 显示状态
sudo systemctl status nginx --no-pager

ENDSSH

echo ""
echo "🎉 部署完成！"
echo ""
echo "📝 接下来的步骤："
echo "1. 在域名注册商处添加 DNS 记录："
echo "   类型: A, 主机记录: @, 记录值: ${SERVER_IP}"
echo "   类型: A, 主机记录: www, 记录值: ${SERVER_IP}"
echo ""
echo "2. 等待 DNS 生效（10分钟-24小时）"
echo ""
echo "3. 访问测试："
echo "   http://${SERVER_IP} (立即可用)"
echo "   http://${DOMAIN} (DNS 生效后)"
echo ""
echo "4. 配置 HTTPS（推荐）："
echo "   ssh ${SERVER_USER}@${SERVER_IP}"
echo "   sudo apt install certbot python3-certbot-nginx"
echo "   sudo certbot --nginx -d ${DOMAIN} -d www.${DOMAIN}"
echo ""


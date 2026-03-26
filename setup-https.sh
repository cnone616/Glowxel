#!/bin/bash
# 配置 HTTPS 证书（Let's Encrypt）

: "${SERVER_IP:?请先设置 SERVER_IP}"
: "${SERVER_USER:?请先设置 SERVER_USER}"
: "${DOMAIN:?请先设置 DOMAIN}"

echo "🔒 配置 HTTPS 证书..."

ssh ${SERVER_USER}@${SERVER_IP} << ENDSSH

# 安装 Certbot
if ! command -v certbot &> /dev/null; then
    echo "安装 Certbot..."
    sudo apt update
    sudo apt install -y certbot python3-certbot-nginx
fi

# 获取证书并自动配置 Nginx
echo "申请 SSL 证书..."
sudo certbot --nginx -d ${DOMAIN} -d www.${DOMAIN} --non-interactive --agree-tos --email admin@${DOMAIN} --redirect

# 设置自动续期
sudo systemctl enable certbot.timer
sudo systemctl start certbot.timer

echo "✅ HTTPS 配置完成"
echo "证书将自动续期"

ENDSSH

echo ""
echo "🎉 HTTPS 已启用！"
echo "现在可以通过 https://${DOMAIN} 访问"

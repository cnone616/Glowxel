# Glowxel 域名配置指南

## 域名信息
- 域名：glowxel.com
- 服务器 IP：175.178.153.146

## 🚀 快速部署步骤

### 1️⃣ DNS 配置（在域名注册商处）

登录你的域名注册商（阿里云/腾讯云/Cloudflare/GoDaddy等），添加以下 DNS 记录：

#### 主域名记录
```
类型: A
主机记录: @
记录值: 175.178.153.146
TTL: 600（或默认）
```

#### www 子域名记录
```
类型: A
主机记录: www
记录值: 175.178.153.146
TTL: 600（或默认）
```

保存后等待 DNS 生效（通常 10-30 分钟，最长 24 小时）

#### 验证 DNS 是否生效
```bash
# 在本地终端执行
ping glowxel.com
nslookup glowxel.com
```

---

### 2️⃣ 部署网站到服务器

#### 方式一：使用自动部署脚本（推荐）

```bash
# 在项目根目录执行
./deploy-website.sh
```

这个脚本会自动：
- 构建网站（npm run build）
- 上传到服务器
- 配置 Nginx
- 设置反向代理

#### 方式二：手动部署

```bash
# 1. 本地构建
cd website
npm install
npm run build

# 2. 上传到服务器
scp -r dist ubuntu@175.178.153.146:/tmp/glowxel-website

# 3. SSH 到服务器配置
ssh ubuntu@175.178.153.146

# 4. 安装 Nginx
sudo apt update
sudo apt install -y nginx

# 5. 移动文件
sudo mkdir -p /var/www/glowxel
sudo mv /tmp/glowxel-website/* /var/www/glowxel/
sudo chown -R www-data:www-data /var/www/glowxel

# 6. 创建 Nginx 配置（见下方配置文件）

# 7. 启用并重启
sudo ln -s /etc/nginx/sites-available/glowxel.com /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

---

### 3️⃣ 配置 HTTPS（推荐）

DNS 生效后，运行：

```bash
./setup-https.sh
```

或手动配置：

```bash
ssh ubuntu@175.178.153.146

# 安装 Certbot
sudo apt install -y certbot python3-certbot-nginx

# 申请证书
sudo certbot --nginx -d glowxel.com -d www.glowxel.com

# 证书会自动续期
```

---

## 📝 Nginx 配置文件

位置：`/etc/nginx/sites-available/glowxel.com`

```nginx
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
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml;
    
    # 静态文件缓存
    location ~* \.(jpg|jpeg|png|gif|ico|css|js|svg|woff|woff2)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
    
    # Vue Router history 模式
    location / {
        try_files $uri $uri/ /index.html;
    }
    
    # API 代理
    location /api/ {
        proxy_pass http://localhost:3000/api/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

---

## ✅ 验证部署

### 检查服务状态
```bash
ssh ubuntu@175.178.153.146

# 检查 Nginx
sudo systemctl status nginx

# 检查网站文件
ls -la /var/www/glowxel/

# 查看日志
sudo tail -f /var/log/nginx/glowxel-access.log
sudo tail -f /var/log/nginx/glowxel-error.log
```

### 访问测试
- HTTP: http://glowxel.com
- HTTPS: https://glowxel.com (配置证书后)
- 直接 IP: http://175.178.153.146

---

## 🔄 更新网站

每次更新代码后：

```bash
# 方式一：使用脚本
./deploy-website.sh

# 方式二：手动
cd website && npm run build
scp -r dist/* ubuntu@175.178.153.146:/var/www/glowxel/
```

---

## 🐛 常见问题

### DNS 未生效
- 等待时间：最长 24 小时
- 清除本地 DNS 缓存：`sudo dscacheutil -flushcache` (macOS)
- 使用在线工具检查：https://dnschecker.org

### 502 Bad Gateway
- 检查后端服务是否运行：`pm2 status`
- 重启后端：`pm2 restart glowxel-server`

### 403 Forbidden
- 检查文件权限：`sudo chown -R www-data:www-data /var/www/glowxel`

### HTTPS 证书申请失败
- 确保 DNS 已生效
- 确保 80 端口可访问
- 检查防火墙：`sudo ufw status`


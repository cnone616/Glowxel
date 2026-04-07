<template>
  <footer class="footer">
    <div class="container">
      <div class="footer-content">
        <div class="footer-brand">
          <span class="brand-name">光格像素工坊</span>
          <p class="brand-desc">像素内容创作与设备连接平台</p>
        </div>
        <div class="footer-links">
          <router-link to="/community">社区</router-link>
          <router-link to="/create">创作</router-link>
          <router-link to="/templates">模板</router-link>
          <router-link to="/challenges">挑战</router-link>
        </div>
      </div>
      <div class="footer-bottom">
        <p>&copy; 2026 光格像素工坊</p>
      </div>
    </div>
  </footer>
</template>

<style scoped>
.footer {
  background: #fafafa;
  border-top: 1px solid #f0f0f0;
  padding: 40px 0 20px;
  margin-top: 60px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

.footer-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.brand-name {
  font-size: 18px;
  font-weight: 700;
  color: #1a1a1a;
}

.brand-desc {
  color: #999;
  font-size: 13px;
  margin-top: 4px;
}

.footer-links {
  display: flex;
  gap: 24px;
}

.footer-links a {
  color: #666;
  text-decoration: none;
  font-size: 14px;
  transition: color 0.2s;
}

.footer-links a:hover {
  color: #1a1a1a;
}

.footer-bottom {
  text-align: center;
  padding-top: 20px;
  border-top: 1px solid #f0f0f0;
  color: #bbb;
  font-size: 13px;
}

@media (max-width: 768px) {
  .footer-content {
    flex-direction: column;
    gap: 20px;
    text-align: center;
  }
}
</style>

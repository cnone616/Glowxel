<template>
  <footer class="footer">
    <div class="container">
      <div class="footer-content">
        <div class="footer-section">
          <h3>MATRIX</h3>
          <p>LED 矩阵创作平台</p>
        </div>
        
        <div class="footer-section">
          <h4>快速链接</h4>
          <router-link to="/showcase">作品展示</router-link>
          <router-link to="/clock-designer">时钟设计</router-link>
          <router-link to="/docs">文档</router-link>
        </div>
        
        <div class="footer-section">
          <h4>联系我们</h4>
          <p>Email: contact@example.com</p>
          <p>GitHub: github.com/example</p>
        </div>
      </div>
      
      <div class="footer-bottom">
        <p>&copy; 2026 MATRIX. All rights reserved.</p>
      </div>
    </div>
  </footer>
</template>

<style scoped>
.footer {
  background: #0a0a0a;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  padding: 40px 0 20px;
  margin-top: 80px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

.footer-content {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 40px;
  margin-bottom: 30px;
}

.footer-section h3 {
  color: #00f3ff;
  margin-bottom: 10px;
  font-family: monospace;
  font-weight: bold;
  letter-spacing: 0.15em;
}

.footer-section h4 {
  color: #fff;
  margin-bottom: 15px;
  font-size: 16px;
}

.footer-section p,
.footer-section a {
  color: rgba(255, 255, 255, 0.6);
  text-decoration: none;
  display: block;
  margin-bottom: 8px;
  font-size: 14px;
}

.footer-section a:hover {
  color: #00f3ff;
}

.footer-bottom {
  text-align: center;
  padding-top: 20px;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  color: rgba(255, 255, 255, 0.4);
  font-size: 14px;
}
</style>

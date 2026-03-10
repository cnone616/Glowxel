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
  background: linear-gradient(135deg, rgba(255, 255, 255, 0.95) 0%, rgba(255, 255, 255, 0.85) 100%);
  backdrop-filter: blur(20px);
  border-top: 2px solid rgba(255, 107, 107, 0.2);
  padding: 60px 0 30px;
  margin-top: 100px;
  box-shadow: 0 -4px 24px rgba(255, 107, 107, 0.08);
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

.footer-content {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 48px;
  margin-bottom: 40px;
}

.footer-section h3 {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  margin-bottom: 16px;
  font-family: 'SF Mono', 'Monaco', 'Courier New', monospace;
  font-weight: 800;
  letter-spacing: 0.15em;
  font-size: 20px;
}

.footer-section h4 {
  color: #2d3748;
  margin-bottom: 20px;
  font-size: 17px;
  font-weight: 700;
}

.footer-section p,
.footer-section a {
  color: #718096;
  text-decoration: none;
  display: block;
  margin-bottom: 10px;
  font-size: 15px;
  transition: all 0.3s ease;
}

.footer-section a:hover {
  color: #ff6b6b;
  transform: translateX(4px);
}

.footer-bottom {
  text-align: center;
  padding-top: 30px;
  border-top: 2px solid rgba(255, 107, 107, 0.15);
  color: #a0aec0;
  font-size: 14px;
  margin-top: 20px;
}
</style>

@media (max-width: 768px) {
  .footer {
    padding: 40px 0 24px;
    margin-top: 60px;
  }
  
  .footer-content {
    grid-template-columns: 1fr;
    gap: 32px;
    margin-bottom: 32px;
  }
  
  .footer-section h3 {
    font-size: 18px;
  }
  
  .footer-section h4 {
    font-size: 16px;
    margin-bottom: 16px;
  }
  
  .footer-section p,
  .footer-section a {
    font-size: 14px;
  }
  
  .footer-bottom {
    padding-top: 24px;
    font-size: 13px;
  }
}

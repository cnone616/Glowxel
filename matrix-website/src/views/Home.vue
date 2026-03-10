<template>
  <div class="home">
    <section class="hero">
      <div class="container">
        <h1 class="hero-title">
          <span class="gradient-text">MATRIX</span>
          <br>LED 矩阵创作平台
        </h1>
        <p class="hero-subtitle">在线设计、实时预览、一键导出</p>
        <div class="hero-buttons">
          <router-link to="/clock-designer" class="btn btn-primary">开始设计</router-link>
          <router-link to="/showcase" class="btn btn-secondary">浏览作品</router-link>
        </div>
      </div>
    </section>

    <section class="features">
      <div class="container">
        <h2 class="section-title">核心功能</h2>
        <div class="feature-grid">
          <div class="feature-card">
            <div class="feature-icon">时钟</div>
            <h3>时钟设计器</h3>
            <p>自定义时间、日期、星期的位置、大小和颜色</p>
            <router-link to="/clock-designer" class="feature-link">立即体验 →</router-link>
          </div>
          
          <div class="feature-card">
            <div class="feature-icon">编辑</div>
            <h3>图案编辑器</h3>
            <p>64x64 像素画布，支持绘制和图片转换</p>
            <router-link to="/pattern-editor" class="feature-link">开始创作 →</router-link>
          </div>
          
          <div class="feature-card">
            <div class="feature-icon">展示</div>
            <h3>作品展示</h3>
            <p>浏览社区创作，获取灵感和模板</p>
            <router-link to="/showcase" class="feature-link">查看更多 →</router-link>
          </div>
        </div>
      </div>
    </section>

    <section class="showcase-preview">
      <div class="container">
        <h2 class="section-title">精选作品</h2>
        <div class="preview-grid">
          <div v-for="item in featuredItems" :key="item.id" class="preview-item">
            <img :src="item.imageUrl" :alt="item.title">
            <div class="preview-info">
              <h4>{{ item.title }}</h4>
              <span>{{ item.author }}</span>
            </div>
          </div>
        </div>
        <router-link to="/showcase" class="btn btn-secondary">查看全部作品</router-link>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { showcaseApi } from '@/api/showcase'

const featuredItems = ref([])

onMounted(async () => {
  const { items } = await showcaseApi.getShowcaseItems({ sortBy: 'likes' })
  featuredItems.value = items.slice(0, 4)
})
</script>

<style scoped>
.hero {
  padding: 120px 0;
  text-align: center;
  background: linear-gradient(135deg, rgba(255, 255, 255, 0.9) 0%, rgba(255, 255, 255, 0.6) 100%);
  backdrop-filter: blur(20px);
  border-radius: 0 0 50px 50px;
  box-shadow: 0 10px 40px rgba(255, 107, 107, 0.15);
}

.hero-title {
  font-size: 64px;
  font-weight: 800;
  margin-bottom: 24px;
  line-height: 1.2;
  letter-spacing: -0.02em;
  text-shadow: 0 2px 20px rgba(255, 107, 107, 0.2);
}

.gradient-text {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 50%, #ff9a9e 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  animation: gradientShift 3s ease infinite;
  background-size: 200% 200%;
}

@keyframes gradientShift {
  0%, 100% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
}

.hero-subtitle {
  font-size: 22px;
  color: #4a5568;
  margin-bottom: 48px;
  font-weight: 500;
  letter-spacing: 0.01em;
}

.hero-buttons {
  display: flex;
  gap: 24px;
  justify-content: center;
  flex-wrap: wrap;
}

.btn {
  padding: 18px 48px;
  border-radius: 16px;
  text-decoration: none;
  font-size: 18px;
  font-weight: 700;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  display: inline-block;
  position: relative;
  overflow: hidden;
  min-width: 180px;
  text-align: center;
}

.btn::before {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 0;
  height: 0;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.3);
  transform: translate(-50%, -50%);
  transition: width 0.6s, height 0.6s;
}

.btn:hover::before {
  width: 300px;
  height: 300px;
}

.btn-primary {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  color: #ffffff;
  box-shadow: 0 8px 24px rgba(255, 107, 107, 0.35);
  position: relative;
  z-index: 1;
}

.btn-primary:hover {
  transform: translateY(-3px);
  box-shadow: 0 12px 32px rgba(255, 107, 107, 0.45);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.95);
  color: #ff6b6b;
  border: 3px solid rgba(255, 107, 107, 0.3);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.15);
}

.btn-secondary:hover {
  background: #ffffff;
  border-color: #ff6b6b;
  transform: translateY(-3px);
  box-shadow: 0 8px 24px rgba(255, 107, 107, 0.25);
}

.features {
  padding: 100px 0;
  position: relative;
}

.features::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: radial-gradient(circle at 30% 50%, rgba(255, 107, 107, 0.08) 0%, transparent 50%),
              radial-gradient(circle at 70% 50%, rgba(255, 165, 0, 0.08) 0%, transparent 50%);
  pointer-events: none;
}

.section-title {
  text-align: center;
  font-size: 42px;
  font-weight: 700;
  margin-bottom: 64px;
  color: #2d3748;
  position: relative;
  display: inline-block;
  width: 100%;
}

.section-title::after {
  content: '';
  position: absolute;
  bottom: -12px;
  left: 50%;
  transform: translateX(-50%);
  width: 80px;
  height: 4px;
  background: linear-gradient(90deg, #ff6b6b 0%, #ffa500 100%);
  border-radius: 2px;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 30px;
  max-width: 1000px;
  margin: 0 auto;
}

.feature-card {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 255, 255, 0.6);
  border-radius: 24px;
  padding: 48px 36px;
  text-align: center;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.12);
  position: relative;
  overflow: hidden;
}

.feature-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: linear-gradient(90deg, #ff6b6b 0%, #ffa500 100%);
  transform: scaleX(0);
  transition: transform 0.4s ease;
}

.feature-card:hover::before {
  transform: scaleX(1);
}

.feature-card:hover {
  background: rgba(255, 255, 255, 0.95);
  border-color: #ff6b6b;
  transform: translateY(-8px);
  box-shadow: 0 16px 48px rgba(255, 107, 107, 0.2);
}

.feature-icon {
  font-size: 20px;
  margin-bottom: 24px;
  color: #ff6b6b;
  font-weight: 700;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  text-transform: uppercase;
  letter-spacing: 0.1em;
}

.feature-card h3 {
  font-size: 26px;
  margin-bottom: 16px;
  color: #2d3748;
  font-weight: 700;
}

.feature-card p {
  color: #718096;
  margin-bottom: 24px;
  line-height: 1.7;
  font-size: 15px;
}

.feature-link {
  color: #ff6b6b;
  text-decoration: none;
  font-weight: 600;
  display: inline-flex;
  align-items: center;
  gap: 4px;
  transition: all 0.3s ease;
}

.feature-link:hover {
  color: #ff5252;
  gap: 8px;
}

.showcase-preview {
  padding: 100px 0;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.5) 0%, rgba(255, 255, 255, 0.3) 100%);
  backdrop-filter: blur(10px);
  text-align: center;
  border-radius: 50px 50px 0 0;
  margin-top: 60px;
}

.preview-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 20px;
  max-width: 900px;
  margin: 0 auto 40px;
}

.preview-item {
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 255, 255, 0.8);
  border-radius: 20px;
  overflow: hidden;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  cursor: pointer;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.preview-item:hover {
  transform: translateY(-8px) scale(1.02);
  border-color: #ff6b6b;
  box-shadow: 0 12px 40px rgba(255, 107, 107, 0.25);
}

.preview-item img {
  width: 100%;
  height: 220px;
  object-fit: cover;
  background: linear-gradient(135deg, #fff5f5 0%, #ffe0e0 100%);
  transition: transform 0.4s ease;
}

.preview-item:hover img {
  transform: scale(1.05);
}

.preview-info {
  padding: 20px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.95) 0%, rgba(255, 255, 255, 0.85) 100%);
}

.preview-info h4 {
  color: #2d3748;
  margin-bottom: 8px;
  font-weight: 600;
  font-size: 16px;
}

.preview-info span {
  color: #718096;
  font-size: 14px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

@media (max-width: 768px) {
  .hero-title {
    font-size: 42px;
  }
  
  .hero-subtitle {
    font-size: 18px;
  }
  
  .hero-buttons {
    flex-direction: column;
    align-items: center;
    gap: 16px;
  }
  
  .btn {
    width: 100%;
    max-width: 320px;
  }
  
  .section-title {
    font-size: 32px;
  }
  
  .feature-grid {
    grid-template-columns: 1fr;
  }
}
</style>

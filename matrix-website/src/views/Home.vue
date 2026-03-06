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
  padding: 100px 0;
  text-align: center;
  background: linear-gradient(180deg, rgba(0, 243, 255, 0.1) 0%, transparent 100%);
}

.hero-title {
  font-size: 56px;
  font-weight: 700;
  margin-bottom: 20px;
  line-height: 1.2;
}

.gradient-text {
  background: linear-gradient(135deg, #00f3ff 0%, #c864ff 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.hero-subtitle {
  font-size: 20px;
  color: rgba(255, 255, 255, 0.7);
  margin-bottom: 40px;
}

.hero-buttons {
  display: flex;
  gap: 20px;
  justify-content: center;
}

.btn {
  padding: 14px 32px;
  border-radius: 8px;
  text-decoration: none;
  font-size: 16px;
  font-weight: 500;
  transition: all 0.3s;
  display: inline-block;
}

.btn-primary {
  background: #00f3ff;
  color: #0a0a0a;
}

.btn-primary:hover {
  background: #00d4dd;
  transform: translateY(-2px);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
  border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.15);
  transform: translateY(-2px);
}

.features {
  padding: 80px 0;
}

.section-title {
  text-align: center;
  font-size: 36px;
  margin-bottom: 60px;
  color: #fff;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 30px;
  max-width: 1000px;
  margin: 0 auto;
}

.feature-card {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 40px 30px;
  text-align: center;
  transition: all 0.3s;
}

.feature-card:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: #00f3ff;
  transform: translateY(-5px);
}

.feature-icon {
  font-size: 18px;
  margin-bottom: 20px;
  color: #00f3ff;
  font-weight: 600;
}

.feature-card h3 {
  font-size: 24px;
  margin-bottom: 15px;
  color: #fff;
}

.feature-card p {
  color: rgba(255, 255, 255, 0.6);
  margin-bottom: 20px;
  line-height: 1.6;
}

.feature-link {
  color: #00f3ff;
  text-decoration: none;
  font-weight: 500;
}

.feature-link:hover {
  text-decoration: underline;
}

.showcase-preview {
  padding: 80px 0;
  background: rgba(255, 255, 255, 0.02);
  text-align: center;
}

.preview-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 20px;
  max-width: 900px;
  margin: 0 auto 40px;
}

.preview-item {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  overflow: hidden;
  transition: all 0.3s;
  cursor: pointer;
}

.preview-item:hover {
  transform: translateY(-5px);
  border-color: #00f3ff;
}

.preview-item img {
  width: 100%;
  height: 200px;
  object-fit: cover;
  background: #111;
}

.preview-info {
  padding: 15px;
}

.preview-info h4 {
  color: #fff;
  margin-bottom: 5px;
}

.preview-info span {
  color: rgba(255, 255, 255, 0.5);
  font-size: 14px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

@media (max-width: 768px) {
  .hero-title {
    font-size: 36px;
  }
  
  .hero-buttons {
    flex-direction: column;
    align-items: center;
  }
  
  .btn {
    width: 100%;
    max-width: 300px;
  }
}
</style>

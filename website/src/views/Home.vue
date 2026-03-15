<template>
  <div class="home">
    <section class="hero">
      <div class="container">
        <h1 class="hero-title">Glowxel</h1>
        <p class="hero-subtitle">像素创作平台 · 让创意点亮每一格</p>
        <p class="hero-desc">在线设计像素图案、自定义时钟、导入图片处理，与社区分享你的作品</p>
        <div class="hero-actions">
          <router-link to="/create" class="btn btn-primary">开始创作</router-link>
          <router-link to="/community" class="btn btn-outline">浏览社区</router-link>
        </div>
      </div>
    </section>

    <section class="features">
      <div class="container">
        <h2 class="section-title">核心功能</h2>
        <div class="feature-grid">
          <div class="feature-card" v-for="f in features" :key="f.title">
            <div class="feature-emoji">{{ f.icon }}</div>
            <h3>{{ f.title }}</h3>
            <p>{{ f.desc }}</p>
          </div>
        </div>
      </div>
    </section>

    <section class="showcase" v-if="artworks.length">
      <div class="container">
        <h2 class="section-title">精选作品</h2>
        <div class="artwork-grid">
          <div class="artwork-card" v-for="item in artworks" :key="item.id">
            <div class="artwork-img" :style="{ background: '#f5f5f5' }"></div>
            <div class="artwork-info">
              <span class="artwork-title">{{ item.title || '未命名作品' }}</span>
              <span class="artwork-author">{{ item.author || '匿名' }}</span>
            </div>
          </div>
        </div>
        <div class="center">
          <router-link to="/community" class="btn btn-outline">查看更多</router-link>
        </div>
      </div>
    </section>

    <section class="about">
      <div class="container">
        <h2 class="section-title">关于 Glowxel</h2>
        <p class="about-text">
          Glowxel（光格科技）是一个面向像素艺术爱好者的创作平台。
          我们提供在线像素编辑器、图片转像素工具、时钟自定义设计等功能，
          支持将作品同步到 Glowxel 像素显示设备。无论你是像素艺术新手还是资深创作者，
          都能在这里找到灵感、分享作品、参与社区挑战。
        </p>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { artworkAPI } from '@/api/index.js'

const features = [
  { icon: '🎨', title: '像素编辑器', desc: '64×64 画布，支持画笔、橡皮擦、填充、图片导入' },
  { icon: '⏰', title: '时钟设计', desc: '自定义时间、日期、星期的位置和颜色' },
  { icon: '🖼️', title: '图片处理', desc: '导入任意图片，智能转换为像素图案' },
  { icon: '👥', title: '社区分享', desc: '发布作品、点赞评论、关注创作者' },
  { icon: '📦', title: '模板库', desc: '丰富的预设模板，一键使用' },
  { icon: '🏆', title: '创作挑战', desc: '参与主题挑战，展示你的创意' }
]

const artworks = ref([])

onMounted(async () => {
  try {
    const res = await artworkAPI.getList({ page: 1, limit: 4 })
    if (res.success) artworks.value = res.data?.list || []
  } catch (e) { /* ignore */ }
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.center { text-align: center; margin-top: 32px; }

.hero {
  padding: 80px 0 60px;
  text-align: center;
}

.hero-title {
  font-size: 56px;
  font-weight: 800;
  color: #1a1a1a;
  letter-spacing: -0.02em;
}

.hero-subtitle {
  font-size: 20px;
  color: #666;
  margin-top: 12px;
}

.hero-desc {
  font-size: 15px;
  color: #999;
  margin-top: 8px;
  max-width: 500px;
  margin-left: auto;
  margin-right: auto;
}

.hero-actions {
  display: flex;
  gap: 16px;
  justify-content: center;
  margin-top: 32px;
}

.btn {
  padding: 12px 32px;
  border-radius: 8px;
  font-size: 15px;
  font-weight: 600;
  text-decoration: none;
  transition: all 0.2s;
  cursor: pointer;
}

.btn-primary {
  background: #1a1a1a;
  color: #fff;
}

.btn-primary:hover { background: #333; }

.btn-outline {
  background: #fff;
  color: #1a1a1a;
  border: 1px solid #e0e0e0;
}

.btn-outline:hover { border-color: #1a1a1a; }

.features { padding: 60px 0; }

.section-title {
  text-align: center;
  font-size: 28px;
  font-weight: 700;
  color: #1a1a1a;
  margin-bottom: 40px;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
}

.feature-card {
  background: #fafafa;
  border: 1px solid #f0f0f0;
  border-radius: 12px;
  padding: 28px 24px;
  transition: border-color 0.2s;
}

.feature-card:hover { border-color: #ddd; }

.feature-emoji { font-size: 28px; margin-bottom: 12px; }
.feature-card h3 { font-size: 16px; font-weight: 600; color: #1a1a1a; margin-bottom: 8px; }
.feature-card p { font-size: 14px; color: #888; line-height: 1.6; }

.showcase { padding: 60px 0; }

.artwork-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}

.artwork-card {
  background: #fafafa;
  border: 1px solid #f0f0f0;
  border-radius: 12px;
  overflow: hidden;
  cursor: pointer;
  transition: border-color 0.2s;
}

.artwork-card:hover { border-color: #ddd; }

.artwork-img {
  width: 100%;
  height: 160px;
}

.artwork-info {
  padding: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.artwork-title { font-size: 14px; font-weight: 500; color: #1a1a1a; }
.artwork-author { font-size: 12px; color: #999; }

.about { padding: 60px 0; }

.about-text {
  max-width: 640px;
  margin: 0 auto;
  text-align: center;
  font-size: 15px;
  color: #666;
  line-height: 1.8;
}

@media (max-width: 768px) {
  .hero-title { font-size: 36px; }
  .hero-subtitle { font-size: 16px; }
  .hero-actions { flex-direction: column; align-items: center; }
  .btn { width: 100%; max-width: 280px; text-align: center; }
  .feature-grid { grid-template-columns: 1fr; }
  .artwork-grid { grid-template-columns: repeat(2, 1fr); }
  .section-title { font-size: 22px; margin-bottom: 28px; }
}
</style>

<template>
  <div class="home">
    <section class="hero">
      <div class="container">
        <h1 class="hero-title">RenLight</h1>
        <p class="hero-subtitle">像素创作平台 · 让创意点亮每一格</p>
        <p class="hero-desc">
          在线设计像素图案、自定义时钟、导入图片处理，与社区分享你的作品
        </p>
        <div class="hero-actions">
          <router-link to="/create" class="btn btn-primary"
            >开始创作</router-link
          >
          <router-link to="/community" class="btn btn-outline"
            >浏览社区</router-link
          >
        </div>
      </div>
    </section>

    <section class="features">
      <div class="container">
        <h2 class="section-title">核心功能</h2>
        <div class="feature-grid">
          <div class="feature-card" v-for="f in features" :key="f.title">
            <div class="feature-icon" v-html="f.icon"></div>
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
              <span class="artwork-title">{{
                item.title || "未命名作品"
              }}</span>
              <span class="artwork-author">{{ item.author || "匿名" }}</span>
            </div>
          </div>
        </div>
        <div class="center">
          <router-link to="/community" class="btn btn-outline"
            >查看更多</router-link
          >
        </div>
      </div>
    </section>

    <!-- 硬件产品落地页 -->
    <section class="hardware">
      <div class="container">
        <h2 class="section-title">RenLight 像素显示设备</h2>
        <p class="section-desc">
          64×64 RGB LED 矩阵，配套小程序实时控制，让你的像素作品真实发光
        </p>
        <div class="hardware-grid">
          <div
            class="hardware-card"
            v-for="spec in hardwareSpecs"
            :key="spec.label"
          >
            <div class="spec-icon" v-html="spec.icon"></div>
            <div class="spec-label">{{ spec.label }}</div>
            <div class="spec-value">{{ spec.value }}</div>
          </div>
        </div>
        <div class="hardware-steps">
          <div class="step" v-for="(step, i) in setupSteps" :key="i">
            <div class="step-num">{{ i + 1 }}</div>
            <div class="step-text">{{ step }}</div>
          </div>
        </div>
        <div class="center">
          <a
            href="https://item.taobao.com"
            target="_blank"
            rel="noopener"
            class="btn btn-primary"
            >立即购买</a
          >
          <a
            href="/hardware-guide.pdf"
            target="_blank"
            rel="noopener"
            class="btn btn-outline"
            >组装指南</a
          >
        </div>
      </div>
    </section>

    <section class="about">
      <div class="container">
        <h2 class="section-title">关于 RenLight</h2>
        <p class="about-text">
          RenLight（仁光数字科技）是一个面向像素艺术爱好者的创作平台。
          我们提供在线像素编辑器、图片转像素工具、时钟自定义设计等功能，
          支持将作品同步到 RenLight
          像素显示设备。无论你是像素艺术新手还是资深创作者，
          都能在这里找到灵感、分享作品、参与社区挑战。
        </p>
      </div>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from "vue";
import { artworkAPI } from "@/api/index.js";

const features = [
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 19l7-7 3 3-7 7-3-3z"/><path d="M18 13l-1.5-7.5L2 2l3.5 14.5L13 18l5-5z"/><path d="M2 2l7.586 7.586"/><circle cx="11" cy="11" r="2"/></svg>`,
    title: "像素编辑器",
    desc: "64×64 画布，支持画笔、橡皮擦、填充、图片导入",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>`,
    title: "时钟设计",
    desc: "自定义时间、日期、星期的位置和颜色",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>`,
    title: "图片处理",
    desc: "导入任意图片，智能转换为像素图案",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>`,
    title: "社区分享",
    desc: "发布作品、点赞评论、关注创作者",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>`,
    title: "模板库",
    desc: "丰富的预设模板，一键使用",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/></svg>`,
    title: "创作挑战",
    desc: "参与主题挑战，展示你的创意",
  },
];

const artworks = ref([]);

const hardwareSpecs = [
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="20" rx="2"/><path d="M8 12h8M12 8v8"/></svg>`,
    label: "分辨率",
    value: "64 × 64 像素",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>`,
    label: "像素间距",
    value: "P2 (2mm)",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>`,
    label: "主控芯片",
    value: "ESP32-S3",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0M1.42 9a16 16 0 0 1 21.16 0M8.53 16.11a6 6 0 0 1 6.95 0M12 20h.01"/></svg>`,
    label: "连接方式",
    value: "WiFi + BLE",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 21V5a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16"/></svg>`,
    label: "外壳尺寸",
    value: "约 135 × 135mm",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>`,
    label: "供电",
    value: "USB-C 5V 2A",
  },
];

const setupSteps = [
  "购买硬件套件，按组装指南完成焊接和组装",
  "下载 RenLight 小程序，蓝牙配网连接 WiFi",
  "在编辑器设计像素图案或时钟样式",
  "一键同步到设备，实时点亮你的作品",
];

onMounted(async () => {
  try {
    const res = await artworkAPI.getList({ page: 1, limit: 4 });
    if (res.success) artworks.value = res.data?.list || [];
  } catch (e) {
    /* ignore */
  }
});
</script>

<style scoped>
.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}
.center {
  text-align: center;
  margin-top: 32px;
}

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
  background: var(--color-primary);
  color: #fff;
}

.btn-primary:hover {
  background: var(--color-primary-hover);
}

.btn-outline {
  background: #fff;
  color: var(--color-text-primary);
  border: 1px solid var(--color-border);
}

.btn-outline:hover {
  border-color: var(--color-primary);
  color: var(--color-primary);
}

.features {
  padding: 60px 0;
}

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

.feature-card:hover {
  border-color: #ddd;
}

.feature-icon {
  width: 44px;
  height: 44px;
  background: var(--color-bg-muted);
  border-radius: var(--radius-sm);
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 16px;
  color: var(--color-text-primary);
}
.feature-card h3 {
  font-size: 16px;
  font-weight: 600;
  color: #1a1a1a;
  margin-bottom: 8px;
}
.feature-card p {
  font-size: 14px;
  color: #888;
  line-height: 1.6;
}

.showcase {
  padding: 60px 0;
}

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

.artwork-card:hover {
  border-color: #ddd;
}

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

.artwork-title {
  font-size: 14px;
  font-weight: 500;
  color: #1a1a1a;
}
.artwork-author {
  font-size: 12px;
  color: #999;
}

.about {
  padding: 60px 0;
}

.about-text {
  max-width: 640px;
  margin: 0 auto;
  text-align: center;
  font-size: 15px;
  color: #666;
  line-height: 1.8;
}

@media (max-width: 768px) {
  .hero-title {
    font-size: 36px;
  }
  .hero-subtitle {
    font-size: 16px;
  }
  .hero-actions {
    flex-direction: column;
    align-items: center;
  }
  .btn {
    width: 100%;
    max-width: 280px;
    text-align: center;
  }
  .feature-grid {
    grid-template-columns: 1fr;
  }
  .artwork-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  .hardware-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  .section-title {
    font-size: 22px;
    margin-bottom: 28px;
  }
}

.hardware {
  padding: 60px 0;
  background: #f8faff;
}
.section-desc {
  text-align: center;
  color: #666;
  margin-bottom: 40px;
  font-size: 16px;
}

.hardware-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
  margin-bottom: 40px;
}

.hardware-card {
  background: #fff;
  border: 1px solid #e8edf5;
  border-radius: 12px;
  padding: 24px;
  text-align: center;
}

.spec-icon {
  color: #4f7fff;
  margin-bottom: 12px;
  display: flex;
  justify-content: center;
}
.spec-label {
  font-size: 13px;
  color: #999;
  margin-bottom: 6px;
}
.spec-value {
  font-size: 16px;
  font-weight: 600;
  color: #1a1a1a;
}

.hardware-steps {
  display: flex;
  gap: 16px;
  margin-bottom: 40px;
  flex-wrap: wrap;
}

.step {
  flex: 1;
  min-width: 180px;
  display: flex;
  align-items: flex-start;
  gap: 12px;
  background: #fff;
  border-radius: 10px;
  padding: 16px;
  border: 1px solid #e8edf5;
}

.step-num {
  width: 28px;
  height: 28px;
  border-radius: 50%;
  background: #4f7fff;
  color: #fff;
  font-size: 14px;
  font-weight: 700;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.step-text {
  font-size: 14px;
  color: #444;
  line-height: 1.5;
}
</style>

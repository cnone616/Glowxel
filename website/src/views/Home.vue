<template>
  <div class="home">
    <section class="hero">
      <div class="container">
        <h1 class="hero-title">光格像素工坊</h1>
        <p class="hero-subtitle">像素内容创作与设备连接平台</p>
        <p class="hero-desc">
          提供像素内容编辑、图片生成、社区展示与设备连接能力，用户完成连接后即可直接使用设备功能
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
          <div class="artwork-card" v-for="item in artworks" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
            <div
              class="artwork-img"
              :style="item.cover_url
                ? `background-image:url(${item.cover_url});background-size:cover;background-position:center;background-color:#f5f5f5`
                : 'background:#f5f5f5'"
            ></div>
            <div class="artwork-info">
              <span class="artwork-title">{{
                item.title || "未命名作品"
              }}</span>
              <span class="artwork-author">{{ item.author_name || "匿名" }}</span>
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

    <section class="hardware">
      <div class="container">
        <h2 class="section-title">设备连接与使用</h2>
        <p class="section-desc">
          设备由我们完成整机设计与调试，用户只需要连接设备、选择内容并同步显示
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
          <router-link to="/device-control" class="btn btn-primary"
            >连接设备</router-link
          >
          <router-link to="/clock-editor" class="btn btn-outline"
            >时钟配置</router-link
          >
        </div>
      </div>
    </section>

    <section class="about">
      <div class="container">
        <h2 class="section-title">关于光格像素工坊</h2>
        <p class="about-text">
          光格像素工坊面向用户提供内容编辑、图片生成、社区浏览、设备连接与常用控制能力。
          设备本身由我们完成整机方案设计与实现，用户无需了解组装过程，连接后即可直接使用。
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
    title: "设备时钟",
    desc: "网页与小程序都可配置时钟样式并同步到设备",
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
    title: "设备连接",
    desc: "连接设备后即可下发内容并进行常用控制",
  },
  {
    icon: `<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/></svg>`,
    title: "模板与灵感",
    desc: "浏览作品、模板和社区内容，快速开始创作",
  },
];

const artworks = ref([]);

const hardwareSpecs = [
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="20" rx="2"/><path d="M8 12h8M12 8v8"/></svg>`,
    label: "显示规格",
    value: "64 × 64 像素",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>`,
    label: "显示形式",
    value: "RGB 像素显示",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>`,
    label: "连接方式",
    value: "热点配网 + 局域网连接",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0M1.42 9a16 16 0 0 1 21.16 0M8.53 16.11a6 6 0 0 1 6.95 0M12 20h.01"/></svg>`,
    label: "使用方式",
    value: "连接后直接控制",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 21V5a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16"/></svg>`,
    label: "内容同步",
    value: "网页 / 小程序",
  },
  {
    icon: `<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>`,
    label: "应用场景",
    value: "创作展示 / 日常互动",
  },
];

const setupSteps = [
  "首次使用先连接设备热点并打开 192.168.4.1 完成配网",
  "选择图片、像素内容或时钟样式",
  "一键同步到设备并立即预览效果",
  "在网页或小程序中继续调整和控制",
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
  max-width: var(--content-max-width);
  margin: 0 auto;
  padding: 0 24px;
}

.center {
  text-align: center;
  margin-top: 28px;
}

.home {
  padding: 28px 0 72px;
}

.hero {
  padding: 18px 0 48px;
}

.hero .container {
  padding: 64px 40px;
  text-align: center;
  border: 4px solid var(--nb-ink);
  background: linear-gradient(90deg, rgba(255, 246, 214, 0.72), rgba(255, 255, 255, 0.98) 52%, rgba(237, 244, 255, 0.8));
  box-shadow: var(--nb-shadow-strong);
}

.hero-title {
  font-size: 64px;
  font-weight: 800;
  color: var(--nb-ink);
  letter-spacing: -0.04em;
  line-height: 1.05;
}

.hero-subtitle {
  margin-top: 16px;
  font-size: 18px;
  font-weight: 800;
  color: #333333;
}

.hero-desc {
  font-size: 16px;
  color: var(--nb-text-secondary);
  margin-top: 18px;
  max-width: 680px;
  margin-left: auto;
  margin-right: auto;
  line-height: 1.8;
}

.hero-actions {
  display: flex;
  gap: 16px;
  justify-content: center;
  margin-top: 32px;
  flex-wrap: wrap;
}

.features {
  padding: 52px 0;
}

.section-title {
  text-align: center;
  font-size: 34px;
  font-weight: 800;
  color: var(--nb-ink);
  margin-bottom: 40px;
  letter-spacing: -0.02em;
}

.feature-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 22px;
}

.feature-card {
  background: #ffffff;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  padding: 28px 24px;
  box-shadow: var(--nb-shadow-card);
}

.feature-icon {
  width: 56px;
  height: 56px;
  background: var(--nb-yellow);
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 16px;
  color: var(--nb-ink);
}

.feature-card h3 {
  font-size: 18px;
  font-weight: 700;
  color: var(--nb-ink);
  margin-bottom: 8px;
}

.feature-card p {
  font-size: 14px;
  color: var(--nb-text-secondary);
  line-height: 1.6;
}

.showcase {
  padding: 52px 0;
}

.artwork-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 20px;
}

.artwork-card {
  background: #ffffff;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  overflow: hidden;
  cursor: pointer;
  box-shadow: var(--nb-shadow-card);
  transition: background-color 0.18s ease;
}

.artwork-card:hover {
  background: #f8f8f8;
}

.artwork-img {
  width: 100%;
  height: 176px;
  border-bottom: 3px solid var(--nb-ink);
}

.artwork-info {
  padding: 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
}

.artwork-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--nb-ink);
}

.artwork-author {
  font-size: 12px;
  color: var(--nb-text-secondary);
  flex-shrink: 0;
}

.about {
  padding: 52px 0;
}

.about-text {
  max-width: 640px;
  margin: 0 auto;
  text-align: center;
  font-size: 15px;
  color: var(--nb-text-secondary);
  line-height: 1.8;
}

.hardware {
  padding: 52px 0;
}

.section-desc {
  text-align: center;
  color: var(--nb-text-secondary);
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
  background: #ffffff;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  padding: 24px;
  text-align: center;
  box-shadow: var(--nb-shadow-card);
}

.spec-icon {
  color: var(--nb-ink);
  margin-bottom: 12px;
  display: flex;
  justify-content: center;
}

.spec-label {
  font-size: 13px;
  color: var(--nb-text-secondary);
  margin-bottom: 6px;
  font-weight: 700;
}

.spec-value {
  font-size: 16px;
  font-weight: 700;
  color: var(--nb-ink);
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
  background: #ffffff;
  padding: 16px;
  border: 3px solid var(--nb-ink);
  border-radius: 0;
  box-shadow: var(--nb-shadow-soft);
}

.step-num {
  width: 28px;
  height: 28px;
  background: var(--nb-yellow);
  border: 2px solid var(--nb-ink);
  color: var(--nb-ink);
  font-size: 14px;
  font-weight: 700;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  border-radius: 0;
}

.step-text {
  font-size: 14px;
  color: var(--nb-text-secondary);
  line-height: 1.5;
}

@media (max-width: 768px) {
  .container {
    padding: 0 16px;
  }

  .home {
    padding: 16px 0 48px;
  }

  .hero {
    padding: 12px 0 36px;
  }

  .hero .container {
    padding: 32px 18px;
    box-shadow: var(--nb-shadow-strong);
  }

  .hero-title {
    font-size: 38px;
  }

  .hero-subtitle {
    font-size: 15px;
  }

  .hero-actions {
    flex-direction: column;
    align-items: center;
  }

  .hero-actions .btn {
    width: 100%;
    max-width: 280px;
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
    font-size: 24px;
    margin-bottom: 24px;
  }

  .feature-card,
  .hardware-card,
  .artwork-card,
  .about .container,
  .step {
    border-radius: 0;
  }
}
</style>

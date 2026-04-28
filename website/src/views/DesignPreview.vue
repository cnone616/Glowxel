<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">QA</span>
      <h1 class="glx-page-shell__title">Design Preview</h1>
      <p class="glx-page-shell__desc">
        这是隐藏 QA 路由，用来快速检查正式页面和设计系统页面，不进入主导航。
      </p>
    </section>

    <section class="glx-grid glx-grid--three">
      <article v-for="item in previewRoutes" :key="item.to" class="glx-section-card glx-section-card--stack">
        <strong class="glx-section-title">{{ item.title }}</strong>
        <p class="glx-page-shell__desc">{{ item.description }}</p>
        <div class="glx-inline-actions">
          <router-link :to="item.to" class="glx-button glx-button--ghost">打开页面</router-link>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
const previewRoutes = [
  { title: "设计系统", to: "/design-system", description: "检查当前 tokens、壳层和按钮组件。" },
  { title: "工作台", to: "/workspace", description: "检查项目列表、创作入口和工作台结构。" },
  { title: "社区", to: "/community", description: "检查公开作品列表和详情页跳转。" },
  { title: "设备控制", to: "/device-control", description: "检查网站保留的关键设备主链。" },
  { title: "个人中心", to: "/profile", description: "检查我的作品、收藏、设置等个人链路。" },
  { title: "云端同步", to: "/cloud-sync", description: "检查项目同步状态和云端项目列表。" },
];
</script>

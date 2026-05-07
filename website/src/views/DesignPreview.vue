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
import { devicePreviewRoutes } from "@/config/appNavigation.js";

const previewRoutes = devicePreviewRoutes;
</script>

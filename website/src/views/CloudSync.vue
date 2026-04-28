<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Cloud Sync</span>
      <h1 class="glx-page-shell__title">云端同步</h1>
      <p class="glx-page-shell__desc">
        云端同步页负责承接项目列表和同步状态，不应该被直接从网站端拿掉。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">云端项目</span>
          <strong class="glx-hero-metric__value">{{ projectStore.totalProjects }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">草稿</span>
          <strong class="glx-hero-metric__value">{{ projectStore.draftProjects.length }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">已发布</span>
          <strong class="glx-hero-metric__value">{{ projectStore.publishedProjects.length }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">最近同步</span>
          <strong class="glx-hero-metric__value">{{ lastSyncText }}</strong>
        </article>
      </div>
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--ghost" :disabled="loading" @click="reloadAll">
          {{ loading ? "刷新中..." : "刷新同步状态" }}
        </button>
      </div>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">云端项目列表</h2>
        <span class="glx-section-meta">共 {{ projectStore.totalProjects }} 个</span>
      </div>

      <div v-if="projectStore.projects.length === 0" class="glx-empty-card">
        <strong class="glx-section-title">当前没有云端项目</strong>
        <p class="glx-page-shell__desc">去工作台保存草稿后，这里会显示同步到云端的项目列表。</p>
      </div>

      <div v-else class="glx-stack">
        <article v-for="item in projectStore.projects" :key="item.id" class="glx-list-card">
          <div class="glx-list-card__copy">
            <strong class="glx-list-card__title">{{ projectName(item) }}</strong>
            <span class="glx-list-card__desc">{{ projectMeta(item) }}</span>
          </div>
          <div class="glx-inline-actions">
            <router-link :to="`/overview/${item.id}`" class="glx-button glx-button--ghost">总览</router-link>
            <router-link :to="`/editor/${item.id}`" class="glx-button glx-button--ghost">编辑</router-link>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from "vue";
import { useProjectStore } from "@/stores/project.js";

const projectStore = useProjectStore();
const loading = ref(false);

const lastSyncText = computed(() => {
  if (
    projectStore.syncStatus != null &&
    typeof projectStore.syncStatus.lastSync === "string" &&
    projectStore.syncStatus.lastSync.length > 0
  ) {
    return projectStore.syncStatus.lastSync.slice(0, 10);
  }
  return "--";
});

function projectName(item) {
  if (typeof item.name === "string" && item.name.length > 0) {
    return item.name;
  }
  return "未命名项目";
}

function projectMeta(item) {
  const parts = [];

  if (typeof item.width === "number" && typeof item.height === "number") {
    parts.push(`${item.width} × ${item.height}`);
  }

  if (typeof item.status === "string" && item.status.length > 0) {
    parts.push(item.status);
  }

  if (typeof item.updated_at === "string" && item.updated_at.length > 0) {
    parts.push(item.updated_at.slice(0, 10));
  }

  return parts.join(" · ");
}

async function reloadAll() {
  loading.value = true;
  try {
    await Promise.all([
      projectStore.loadProjects(),
      projectStore.loadSyncStatus(),
    ]);
  } finally {
    loading.value = false;
  }
}

onMounted(async () => {
  await reloadAll();
});
</script>

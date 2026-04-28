<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">My Works</span>
      <h1 class="glx-page-shell__title">我的作品</h1>
      <p class="glx-page-shell__desc">
        这里恢复的是“已发布作品管理”，不是云端项目草稿。项目草稿继续在工作台和总览页管理。
      </p>
      <div class="glx-inline-actions">
        <router-link to="/workspace" class="glx-button glx-button--ghost">回工作台</router-link>
        <router-link to="/profile" class="glx-button glx-button--ghost">回个人中心</router-link>
      </div>
    </section>

    <section v-if="loading" class="glx-grid glx-grid--three">
      <article v-for="index in 3" :key="index" class="glx-skeleton-card">
        <div class="glx-skeleton my-works-skeleton my-works-skeleton--cover"></div>
        <div class="glx-skeleton my-works-skeleton"></div>
        <div class="glx-skeleton my-works-skeleton"></div>
      </article>
    </section>

    <section v-else-if="works.length === 0" class="glx-empty-card">
      <strong class="glx-section-title">还没有已发布作品</strong>
      <p class="glx-page-shell__desc">先去编辑器发布作品，这里就会出现你的作品管理列表。</p>
    </section>

    <section v-else class="glx-grid glx-grid--three">
      <article v-for="item in works" :key="item.id" class="glx-section-card glx-section-card--stack">
        <img
          v-if="coverUrl(item.cover_url).length > 0"
          :src="coverUrl(item.cover_url)"
          alt="cover"
          class="work-cover"
        />
        <div v-else class="glx-empty-card">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前作品没有封面图。</p>
        </div>
        <strong class="glx-section-title">{{ resolveTitle(item) }}</strong>
        <p class="glx-page-shell__desc">{{ resolveMeta(item) }}</p>
        <div class="glx-inline-actions">
          <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看</router-link>
          <button type="button" class="glx-button glx-button--danger" @click="removeWork(item.id)">删除</button>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { onMounted, ref } from "vue";
import { artworkAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";

const feedback = useFeedback();

const works = ref([]);
const loading = ref(false);

function coverUrl(value) {
  if (typeof value === "string" && value.length > 0) {
    return value;
  }
  return "";
}

function resolveTitle(item) {
  if (typeof item.title === "string" && item.title.length > 0) {
    return item.title;
  }
  return "未命名作品";
}

function resolveMeta(item) {
  const parts = [];

  if (typeof item.status === "string" && item.status.length > 0) {
    parts.push(item.status);
  }

  if (typeof item.likes === "number") {
    parts.push(`${item.likes} 赞`);
  }

  if (typeof item.views === "number") {
    parts.push(`${item.views} 浏览`);
  }

  if (typeof item.created_at === "string" && item.created_at.length > 0) {
    parts.push(item.created_at.slice(0, 10));
  }

  return parts.join(" · ");
}

async function loadWorks() {
  loading.value = true;
  try {
    const response = await artworkAPI.getMine({ page: 1, limit: 50 });
    if (response.success && response.data != null && Array.isArray(response.data.list)) {
      works.value = response.data.list;
      return;
    }

    works.value = [];
    feedback.error("作品加载失败", "没有成功取回我的作品列表。");
  } finally {
    loading.value = false;
  }
}

async function removeWork(id) {
  const response = await artworkAPI.remove(id);
  if (response.success) {
    works.value = works.value.filter((item) => item.id !== id);
    feedback.success("删除成功", "作品已经从列表里移除。");
    return;
  }

  feedback.error("删除失败", "作品没有成功删除。");
}

onMounted(async () => {
  await loadWorks();
});
</script>

<style scoped>
.work-cover {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: cover;
  border: 2px solid #111111;
}

.my-works-skeleton {
  min-height: 18px;
}

.my-works-skeleton--cover {
  min-height: 220px;
}
</style>

<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">My Favorites</span>
      <h1 class="glx-page-shell__title">我的收藏</h1>
      <p class="glx-page-shell__desc">
        收藏链路需要保留，当前页面继续承接已收藏作品的查看和取消收藏。
      </p>
    </section>

    <section v-if="loading" class="glx-grid glx-grid--three">
      <article v-for="index in 3" :key="index" class="glx-skeleton-card">
        <div class="glx-skeleton favorites-skeleton favorites-skeleton--cover"></div>
        <div class="glx-skeleton favorites-skeleton"></div>
        <div class="glx-skeleton favorites-skeleton"></div>
      </article>
    </section>

    <section v-else-if="favorites.length === 0" class="glx-empty-card">
      <strong class="glx-section-title">还没有收藏作品</strong>
      <p class="glx-page-shell__desc">去社区逛一逛，喜欢的作品可以继续收藏到这里。</p>
    </section>

    <section v-else class="glx-grid glx-grid--three">
      <article v-for="item in favorites" :key="item.id" class="glx-section-card glx-section-card--stack">
        <img
          v-if="coverUrl(item.cover_url).length > 0"
          :src="coverUrl(item.cover_url)"
          alt="favorite cover"
          class="favorite-cover"
        />
        <div v-else class="glx-empty-card">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前作品没有封面图。</p>
        </div>
        <strong class="glx-section-title">{{ resolveTitle(item) }}</strong>
        <p class="glx-page-shell__desc">{{ resolveMeta(item) }}</p>
        <div class="glx-inline-actions">
          <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看</router-link>
          <button type="button" class="glx-button glx-button--danger" @click="removeFavorite(item.id)">取消收藏</button>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { onMounted, ref } from "vue";
import { collectAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";

const feedback = useFeedback();

const favorites = ref([]);
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

  if (typeof item.author_name === "string" && item.author_name.length > 0) {
    parts.push(item.author_name);
  }

  if (typeof item.likes === "number") {
    parts.push(`${item.likes} 赞`);
  }

  if (typeof item.views === "number") {
    parts.push(`${item.views} 浏览`);
  }

  return parts.join(" · ");
}

async function loadFavorites() {
  loading.value = true;
  try {
    const response = await collectAPI.getMyCollections({ page: 1, limit: 50 });
    if (response.success && response.data != null && Array.isArray(response.data.list)) {
      favorites.value = response.data.list;
      return;
    }

    favorites.value = [];
    feedback.error("收藏加载失败", "没有成功取回收藏列表。");
  } finally {
    loading.value = false;
  }
}

async function removeFavorite(artworkId) {
  const response = await collectAPI.uncollect(artworkId);
  if (response.success) {
    favorites.value = favorites.value.filter((item) => item.id !== artworkId);
    feedback.success("取消成功", "作品已经从收藏列表中移除。");
    return;
  }

  feedback.error("取消失败", "当前作品没有成功取消收藏。");
}

onMounted(async () => {
  await loadFavorites();
});
</script>

<style scoped>
.favorite-cover {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: cover;
  border: 2px solid #111111;
}

.favorites-skeleton {
  min-height: 18px;
}

.favorites-skeleton--cover {
  min-height: 220px;
}
</style>

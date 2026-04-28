<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Community</span>
      <h1 class="glx-page-shell__title">社区</h1>
      <p class="glx-page-shell__desc">
        社区首页继续保留公开浏览壳，但列表、空态、错误反馈和作品回流统一收口到 `glx` 页面族。
      </p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前排序</span>
          <strong class="glx-hero-metric__value">{{ tabText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">当前页码</span>
          <strong class="glx-hero-metric__value">{{ pageText }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">已加载作品</span>
          <strong class="glx-hero-metric__value">{{ list.length }}</strong>
        </article>
      </div>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-tabs">
        <button
          type="button"
          class="glx-tab"
          :class="{ 'is-active': tab === 'latest' }"
          @click="switchTab('latest')"
        >
          最新
        </button>
        <button
          type="button"
          class="glx-tab"
          :class="{ 'is-active': tab === 'hot' }"
          @click="switchTab('hot')"
        >
          热门
        </button>
      </div>
    </section>

    <div v-if="loading && list.length === 0" class="glx-grid glx-grid--three">
      <GlxSkeletonCard />
      <GlxSkeletonCard />
      <GlxSkeletonCard />
    </div>

    <section v-else-if="list.length === 0" class="glx-empty-card">
      <strong class="glx-section-title">暂时还没有作品</strong>
      <p class="glx-page-shell__desc">等作品发布后，这里会自动显示最新和热门列表。</p>
    </section>

    <section v-else class="glx-grid glx-grid--three">
      <article
        v-for="item in list"
        :key="item.id"
        class="glx-section-card glx-section-card--stack community-card"
      >
        <img
          v-if="typeof item.cover_url === 'string' && item.cover_url.length > 0"
          :src="item.cover_url"
          alt="cover"
          class="community-card__image"
        />
        <div v-else class="glx-empty-card community-card__empty">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前作品还没有封面图。</p>
        </div>
        <strong class="glx-section-title community-card__title">{{ resolveTitle(item) }}</strong>
        <p class="glx-page-shell__desc">{{ resolveMeta(item) }}</p>
        <div class="glx-inline-actions">
          <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看详情</router-link>
        </div>
      </article>
    </section>

    <section class="glx-inline-actions">
      <button
        v-if="hasMore"
        type="button"
        class="glx-button glx-button--ghost"
        :disabled="loading"
        @click="loadMore"
      >
        {{ loading ? "加载中..." : "加载更多" }}
      </button>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from "vue";
import { artworkAPI } from "@/api/index.js";
import { useFeedback } from "@/composables/useFeedback.js";
import GlxSkeletonCard from "@/components/glx/GlxSkeletonCard.vue";

const feedback = useFeedback();

const tab = ref("latest");
const list = ref([]);
const page = ref(1);
const pageSize = 12;
const hasMore = ref(true);
const loading = ref(false);

const tabText = computed(() => {
  if (tab.value === "hot") {
    return "热门";
  }
  return "最新";
});

const pageText = computed(() => String(page.value));

function resolveTitle(item) {
  if (typeof item.title === "string" && item.title.length > 0) {
    return item.title;
  }
  return "未命名";
}

function resolveMeta(item) {
  const authorText = typeof item.author_name === "string" && item.author_name.length > 0 ? item.author_name : "匿名创作者";
  const likeText = typeof item.likes === "number" ? `${item.likes} 赞` : "0 赞";
  return `${authorText} · ${likeText}`;
}

async function fetchList(reset) {
  if (loading.value) {
    return;
  }

  if (reset) {
    page.value = 1;
  }

  loading.value = true;
  try {
    const response = tab.value === "hot"
      ? await artworkAPI.getPopular(page.value * pageSize)
      : await artworkAPI.getLatest(page.value * pageSize);

    if (response.success && response.data != null && Array.isArray(response.data.list)) {
      const startIndex = (page.value - 1) * pageSize;
      const endIndex = page.value * pageSize;
      const nextItems = response.data.list.slice(startIndex, endIndex);
      if (reset) {
        list.value = nextItems;
      } else {
        list.value = list.value.concat(nextItems);
      }
      hasMore.value = nextItems.length === pageSize;
      return;
    }

    hasMore.value = false;
    if (reset) {
      list.value = [];
    }
    feedback.error("列表加载失败", response.message || "社区列表没有成功返回。");
  } catch (error) {
    hasMore.value = false;
    if (reset) {
      list.value = [];
    }
    if (error instanceof Error) {
      feedback.error("列表加载失败", error.message);
    } else {
      feedback.error("列表加载失败", "社区列表读取失败。");
    }
  } finally {
    loading.value = false;
  }
}

async function switchTab(nextTab) {
  if (tab.value === nextTab) {
    return;
  }
  tab.value = nextTab;
  await fetchList(true);
}

async function loadMore() {
  page.value += 1;
  await fetchList(false);
}

onMounted(async () => {
  await fetchList(true);
});
</script>

<style scoped>
.community-card__image {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: cover;
  border: 2px solid #111111;
}

.community-card__empty {
  min-height: 220px;
}

.community-card__title {
  font-size: 18px;
}
</style>

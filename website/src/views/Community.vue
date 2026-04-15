<template>
  <div class="community">
    <div class="container">
      <h1 class="page-title">社区</h1>
      <p class="page-desc">发现优秀的像素艺术作品，与创作者互动</p>

      <div class="tabs">
        <button :class="{ active: tab === 'latest' }" @click="tab = 'latest'">最新</button>
        <button :class="{ active: tab === 'hot' }" @click="tab = 'hot'">热门</button>
      </div>

      <div class="artwork-grid">
        <div class="artwork-card" v-for="item in list" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
          <div class="artwork-img" :style="item.cover_url ? `background-image:url(${item.cover_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="artwork-info">
            <span class="title">{{ item.title || '未命名' }}</span>
            <div class="meta">
              <span class="author">{{ item.author_name || '匿名' }}</span>
              <span class="likes">
                <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor" style="color:var(--nb-coral);vertical-align:-1px"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
                {{ item.likes || 0 }}
              </span>
            </div>
          </div>
        </div>
      </div>

      <div class="load-more" v-if="hasMore">
        <button class="btn btn-outline" @click="loadMore">加载更多</button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { artworkAPI } from '@/api/index.js'

const tab = ref('latest')
const list = ref([])
const page = ref(1)
const hasMore = ref(true)

async function fetchList(reset = false) {
  if (reset) { page.value = 1; list.value = [] }
  try {
    const res = tab.value === 'hot'
      ? await artworkAPI.getPopular(12 * page.value)
      : await artworkAPI.getLatest(12 * page.value)
    if (res.success) {
      const allItems = res.data?.list || []
      const items = tab.value === 'hot' || tab.value === 'latest'
        ? allItems.slice((page.value - 1) * 12, page.value * 12)
        : allItems
      list.value = reset ? items : [...list.value, ...items]
      hasMore.value = items.length >= 12
    }
  } catch (e) { /* ignore */ }
}

function loadMore() { page.value++; fetchList() }

watch(tab, () => fetchList(true))
onMounted(() => fetchList())
</script>

<style scoped>
.community {
  padding: 28px 0 64px;
}

.container {
  max-width: var(--content-max-width);
  margin: 0 auto;
  padding: 0 24px;
}

.page-title {
  font-size: 40px;
  font-weight: 900;
  color: var(--nb-ink);
  text-align: center;
  padding-top: 20px;
}

.page-desc {
  text-align: center;
  color: var(--text-secondary);
  font-size: 15px;
  margin-top: 10px;
}

.tabs {
  display: flex;
  justify-content: center;
  gap: 12px;
  margin: 28px 0 36px;
}

.tabs button {
  min-height: 42px;
  padding: 8px 22px;
  border: 2px solid var(--nb-ink);
  background: var(--tone-paper-soft);
  color: var(--text-secondary);
  font-size: 14px;
  font-weight: 800;
  cursor: pointer;
  box-shadow: 4px 4px 0 #000000;
}

.tabs button.active {
  background: var(--nb-blue);
  color: var(--nb-ink);
}

.artwork-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 20px;
}

.artwork-card {
  background: var(--tone-paper-soft);
  border: 3px solid var(--nb-ink);
  overflow: hidden;
  cursor: pointer;
  box-shadow: var(--nb-shadow-card);
}

.artwork-img {
  width: 100%;
  height: 190px;
  border-bottom: 3px solid var(--nb-ink);
}

.artwork-info {
  padding: 14px;
}

.title {
  font-size: 15px;
  font-weight: 900;
  color: var(--nb-ink);
  display: block;
  margin-bottom: 8px;
}

.meta {
  display: flex;
  justify-content: space-between;
  gap: 10px;
}

.author,
.likes {
  font-size: 12px;
  color: var(--text-secondary);
  font-weight: 700;
}

.load-more {
  text-align: center;
  margin: 36px 0 0;
}

@media (max-width: 768px) {
  .container {
    padding: 0 16px;
  }

  .artwork-grid {
    grid-template-columns: repeat(2, 1fr);
  }

  .page-title {
    font-size: 28px;
  }
}
</style>

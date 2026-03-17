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
                <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor" style="color:#e53e3e;vertical-align:-1px"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
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
    const res = await artworkAPI.getList({ page: page.value, limit: 12, sort: tab.value })
    if (res.success) {
      const items = res.data?.list || []
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
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }

.page-title { font-size: 28px; font-weight: 700; color: #1a1a1a; text-align: center; padding-top: 40px; }
.page-desc { text-align: center; color: #999; font-size: 14px; margin-top: 8px; }

.tabs {
  display: flex; justify-content: center; gap: 12px; margin: 32px 0;
}
.tabs button {
  padding: 8px 24px; border-radius: 20px; border: 1px solid #e0e0e0;
  background: #fff; color: #666; font-size: 14px; cursor: pointer; transition: all 0.2s;
}
.tabs button.active { background: var(--color-primary); color: #fff; border-color: var(--color-primary); }

.artwork-grid {
  display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px;
}

.artwork-card {
  background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px;
  overflow: hidden; cursor: pointer; transition: border-color 0.2s;
}
.artwork-card:hover { border-color: #ddd; }

.artwork-img { width: 100%; height: 180px; }

.artwork-info { padding: 12px; }
.title { font-size: 14px; font-weight: 500; color: #1a1a1a; display: block; margin-bottom: 6px; }
.meta { display: flex; justify-content: space-between; }
.author { font-size: 12px; color: #999; }
.likes { font-size: 12px; color: #999; }

.load-more { text-align: center; margin: 32px 0; }
.btn-outline {
  padding: 10px 28px; border-radius: 8px; border: 1px solid #e0e0e0;
  background: #fff; color: #1a1a1a; font-size: 14px; cursor: pointer;
}
.btn-outline:hover { border-color: var(--color-primary); color: var(--color-primary); }

@media (max-width: 768px) {
  .artwork-grid { grid-template-columns: repeat(2, 1fr); }
  .page-title { font-size: 22px; }
}
</style>


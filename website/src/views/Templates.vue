<template>
  <div class="templates">
    <div class="container">
      <h1 class="page-title">边框素材</h1>
      <p class="page-desc">这里放的是边框式拼豆参考，不是整张成品模板。中间主体由你自己选，周边做花边或装饰。</p>

      <div class="template-grid">
        <div class="template-card" v-for="item in list" :key="item.id">
          <div class="template-img" :style="item.image_url ? `background-image:url(${item.image_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="template-info">
            <span class="name">{{ item.name || '未命名边框' }}</span>
            <span class="category">{{ item.category || '边框' }}</span>
          </div>
          <button class="use-btn" @click="handleUse(item)">套用边框</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { templateAPI } from '@/api/index.js'

const router = useRouter()
const list = ref([])

async function handleUse(item) {
  const res = await templateAPI.use(item.id)
  if (res.success) {
    router.push(`/editor?templateId=${item.id}`)
  }
}

onMounted(async () => {
  try {
    const res = await templateAPI.getList({ page: 1, limit: 20 })
    if (res.success) list.value = res.data?.list || []
  } catch (e) { /* ignore */ }
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 32px 20px 56px; }
.page-title { font-size: 34px; font-weight: 800; color: var(--nb-ink); text-align: center; }
.page-desc { text-align: center; color: var(--text-secondary); font-size: 14px; margin-top: 8px; }

.template-grid {
  display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin: 40px 0;
}

.template-card {
  background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0;
  overflow: hidden; transition: background-color 0.2s; box-shadow: var(--nb-shadow-soft);
}
.template-card:hover { background: #f8f8f8; }

.template-img { width: 100%; height: 160px; border-bottom: 3px solid var(--nb-ink); }

.template-info {
  padding: 12px; display: flex; justify-content: space-between; align-items: center;
}
.name { font-size: 14px; font-weight: 700; color: var(--nb-ink); }
.category { font-size: 12px; color: var(--nb-ink); background: var(--tone-blue-soft); padding: 2px 8px; border: 2px solid var(--nb-ink); border-radius: 0; font-weight: 700; }

.use-btn {
  width: 100%; padding: 10px; border: none; border-top: 3px solid var(--nb-ink);
  background: var(--nb-yellow); color: var(--nb-ink); font-size: 13px; font-weight: 800;
  cursor: pointer; transition: background 0.2s;
}
.use-btn:hover { background: #f5f5f5; }

@media (max-width: 768px) {
  .template-grid { grid-template-columns: repeat(2, 1fr); }
  .page-title { font-size: 22px; }
}
</style>

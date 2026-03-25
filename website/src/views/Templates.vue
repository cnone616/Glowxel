<template>
  <div class="templates">
    <div class="container">
      <h1 class="page-title">模板库</h1>
      <p class="page-desc">丰富的预设模板，一键使用开始创作</p>

      <div class="template-grid">
        <div class="template-card" v-for="item in list" :key="item.id">
          <div class="template-img" :style="item.image_url ? `background-image:url(${item.image_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="template-info">
            <span class="name">{{ item.name || '未命名模板' }}</span>
            <span class="category">{{ item.category || '通用' }}</span>
          </div>
          <button class="use-btn" @click="handleUse(item)">使用模板</button>
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
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.page-title { font-size: 28px; font-weight: 700; color: #1a1a1a; text-align: center; padding-top: 40px; }
.page-desc { text-align: center; color: #999; font-size: 14px; margin-top: 8px; }

.template-grid {
  display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin: 40px 0;
}

.template-card {
  background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px;
  overflow: hidden; transition: border-color 0.2s;
}
.template-card:hover { border-color: #ddd; }

.template-img { width: 100%; height: 160px; }

.template-info {
  padding: 12px; display: flex; justify-content: space-between; align-items: center;
}
.name { font-size: 14px; font-weight: 500; color: #1a1a1a; }
.category { font-size: 12px; color: #999; background: #f0f0f0; padding: 2px 8px; border-radius: 4px; }

.use-btn {
  width: 100%; padding: 10px; border: none; border-top: 1px solid #f0f0f0;
  background: #fff; color: #1a1a1a; font-size: 13px; font-weight: 500;
  cursor: pointer; transition: background 0.2s;
}
.use-btn:hover { background: #f5f5f5; }

@media (max-width: 768px) {
  .template-grid { grid-template-columns: repeat(2, 1fr); }
  .page-title { font-size: 22px; }
}
</style>

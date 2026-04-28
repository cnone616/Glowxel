<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Templates</span>
      <h1 class="glx-page-shell__title">边框素材</h1>
      <p class="glx-page-shell__desc">
        这里保留的是边框式拼豆参考，不是整张成品模板。中间主体由你自己决定，周边做花边或装饰。
      </p>
    </section>

    <section class="glx-grid glx-grid--three">
      <article v-for="item in list" :key="item.id" class="glx-section-card glx-section-card--stack">
        <img
          v-if="typeof item.image_url === 'string' && item.image_url.length > 0"
          :src="item.image_url"
          alt="template"
          class="template-cover"
        />
        <div v-else class="glx-empty-card">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前边框素材没有可展示的封面图。</p>
        </div>
        <div class="glx-section-head">
          <strong class="glx-section-title">{{ typeof item.name === "string" && item.name.length > 0 ? item.name : "未命名边框" }}</strong>
          <span class="glx-chip glx-chip--blue">{{ typeof item.category === "string" && item.category.length > 0 ? item.category : "边框" }}</span>
        </div>
        <p class="glx-page-shell__desc">当前网站只保留边框素材浏览与套用入口，不再拆成另一套旧模板壳。</p>
        <div class="glx-inline-actions">
          <button type="button" class="glx-button glx-button--primary" @click="handleUse(item)">套用边框</button>
        </div>
      </article>
    </section>
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
  } catch (e) {
    list.value = []
  }
})
</script>

<style scoped>
.template-cover {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: cover;
  border: 2px solid #111111;
}
</style>

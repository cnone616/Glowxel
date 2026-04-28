<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Challenges</span>
      <h1 class="glx-page-shell__title">创作挑战</h1>
      <p class="glx-page-shell__desc">
        网站当前保留挑战浏览和作品展示，不再在账号体系未冻结前继续保留参与与投稿的半成品动作。
      </p>
    </section>

    <section class="glx-stack">
      <article
        v-for="item in list"
        :key="item.id"
        class="glx-section-card glx-section-card--stack"
      >
        <div class="glx-section-head">
          <div class="glx-stack challenge-copy">
            <span class="glx-chip" :class="statusChipClass(item.status)">{{ statusText(item.status) }}</span>
            <strong class="glx-section-title">{{ item.title || "未命名挑战" }}</strong>
          </div>
          <span class="glx-section-meta">{{ item.end_date ? item.end_date.slice(0, 10) : "--" }}</span>
        </div>
        <p class="glx-page-shell__desc">{{ item.description || "暂无描述" }}</p>
        <div class="glx-kv-grid">
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">参与人数</span>
            <strong class="glx-kv-card__value">{{ item.participants || 0 }}</strong>
          </div>
          <div class="glx-kv-card">
            <span class="glx-kv-card__label">当前状态</span>
            <strong class="glx-kv-card__value">{{ statusText(item.status) }}</strong>
          </div>
        </div>
        <div class="glx-inline-actions">
          <router-link :to="`/challenge/${item.id}`" class="glx-button glx-button--ghost">查看详情</router-link>
        </div>
      </article>
    </section>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { challengeAPI } from '@/api/index.js'

const list = ref([])

function statusText(s) {
  return { active: '进行中', ended: '已结束', upcoming: '即将开始' }[s] || '未知'
}

function statusChipClass(status) {
  if (status === 'active') return 'glx-chip--green'
  if (status === 'upcoming') return 'glx-chip--yellow'
  return ''
}

onMounted(async () => {
  try {
    const res = await challengeAPI.getList({ page: 1, limit: 10 })
    if (res.success) list.value = res.data?.list || []
  } catch (e) {
    list.value = []
  }
})
</script>

<style scoped>
.challenge-copy {
  gap: 10px;
}
</style>

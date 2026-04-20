<template>
  <div class="challenges">
    <div class="container">
      <h1 class="page-title">创作挑战</h1>
      <p class="page-desc">参与主题挑战，展示你的创意</p>

      <div class="challenge-list">
        <div class="challenge-card" v-for="item in list" :key="item.id" @click="router.push(`/challenge/${item.id}`)">
          <div class="challenge-header">
            <span class="challenge-tag" :class="item.status">{{ statusText(item.status) }}</span>
            <span class="challenge-date">{{ item.end_date ? item.end_date.slice(0,10) : '' }}</span>
          </div>
          <h3>{{ item.title || '未命名挑战' }}</h3>
          <p>{{ item.description || '暂无描述' }}</p>
          <div class="challenge-footer">
            <span class="participants">{{ item.participants || 0 }} 人参与</span>
            <button class="join-btn" v-if="item.status === 'active'" @click.stop="handleJoin(item)">参与挑战</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { challengeAPI } from '@/api/index.js'

const router = useRouter()
const list = ref([])

function statusText(s) {
  return { active: '进行中', ended: '已结束', upcoming: '即将开始' }[s] || '未知'
}

async function handleJoin(item) {
  const token = localStorage.getItem('auth_token')
  if (!token) { router.push('/login'); return }
  const res = await challengeAPI.join(item.id)
  if (res.success) {
    if (res.data?.changed) {
      item.participants = (item.participants || 0) + 1
    }
    router.push(`/editor?challengeId=${item.id}`)
  }
}

onMounted(async () => {
  try {
    const res = await challengeAPI.getList({ page: 1, limit: 10 })
    if (res.success) list.value = res.data?.list || []
  } catch (e) { /* ignore */ }
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 32px 20px 56px; }
.page-title { font-size: 34px; font-weight: 800; color: var(--nb-ink); text-align: center; }
.page-desc { text-align: center; color: var(--text-secondary); font-size: 14px; margin-top: 8px; }

.challenge-list { display: flex; flex-direction: column; gap: 16px; margin: 40px 0; max-width: 800px; margin-left: auto; margin-right: auto; }

.challenge-card {
  background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0;
  padding: 24px; transition: background-color 0.2s; cursor: pointer; box-shadow: var(--nb-shadow-soft);
}
.challenge-card:hover { background: #f8f8f8; }

.challenge-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.challenge-tag {
  font-size: 12px; padding: 3px 10px; border: 2px solid var(--nb-ink); border-radius: 0; font-weight: 700;
}
.challenge-tag.active { background: var(--tone-green-soft); color: var(--nb-ink); }
.challenge-tag.ended { background: #f5f5f5; color: var(--text-secondary); }
.challenge-tag.upcoming { background: var(--tone-yellow-soft); color: var(--nb-ink); }
.challenge-date { font-size: 12px; color: var(--text-secondary); }

.challenge-card h3 { font-size: 18px; font-weight: 700; color: var(--nb-ink); margin-bottom: 8px; }
.challenge-card p { font-size: 14px; color: var(--text-secondary); line-height: 1.6; margin-bottom: 16px; }

.challenge-footer { display: flex; justify-content: space-between; align-items: center; }
.participants { font-size: 13px; color: var(--text-secondary); }
.join-btn {
  padding: 8px 20px; border-radius: 0; border: 2px solid var(--nb-ink);
  background: var(--nb-yellow); color: var(--nb-ink); font-size: 13px; font-weight: 800; cursor: pointer; box-shadow: var(--nb-shadow-soft);
}
.join-btn:hover { background: var(--color-primary-hover); }

@media (max-width: 768px) {
  .page-title { font-size: 22px; }
  .challenge-card { padding: 16px; }
}
</style>

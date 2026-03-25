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
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.page-title { font-size: 28px; font-weight: 700; color: #1a1a1a; text-align: center; padding-top: 40px; }
.page-desc { text-align: center; color: #999; font-size: 14px; margin-top: 8px; }

.challenge-list { display: flex; flex-direction: column; gap: 16px; margin: 40px 0; max-width: 800px; margin-left: auto; margin-right: auto; }

.challenge-card {
  background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px;
  padding: 24px; transition: border-color 0.2s; cursor: pointer;
}
.challenge-card:hover { border-color: #ddd; }

.challenge-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.challenge-tag {
  font-size: 12px; padding: 3px 10px; border-radius: 12px; font-weight: 500;
}
.challenge-tag.active { background: #e8f5e9; color: #2e7d32; }
.challenge-tag.ended { background: #f5f5f5; color: #999; }
.challenge-tag.upcoming { background: #fff3e0; color: #e65100; }
.challenge-date { font-size: 12px; color: #999; }

.challenge-card h3 { font-size: 18px; font-weight: 600; color: #1a1a1a; margin-bottom: 8px; }
.challenge-card p { font-size: 14px; color: #888; line-height: 1.6; margin-bottom: 16px; }

.challenge-footer { display: flex; justify-content: space-between; align-items: center; }
.participants { font-size: 13px; color: #999; }
.join-btn {
  padding: 8px 20px; border-radius: 6px; border: none;
  background: var(--color-primary); color: #fff; font-size: 13px; font-weight: 500; cursor: pointer;
}
.join-btn:hover { background: var(--color-primary-hover); }

@media (max-width: 768px) {
  .page-title { font-size: 22px; }
  .challenge-card { padding: 16px; }
}
</style>

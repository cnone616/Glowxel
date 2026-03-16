<template>
  <div class="challenge-detail">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <div v-if="challenge.id" class="detail-layout">
        <div class="banner" :style="challenge.banner_url ? `background-image:url(${challenge.banner_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
        <div class="info-card">
          <div class="header-row">
            <span class="tag" :class="challenge.status">{{ statusText(challenge.status) }}</span>
            <span class="date">截止：{{ challenge.end_date ? challenge.end_date.slice(0,10) : '—' }}</span>
          </div>
          <h1>{{ challenge.title }}</h1>
          <p class="desc">{{ challenge.description }}</p>
          <div class="stats-row">
            <span>{{ challenge.participants || 0 }} 人参与</span>
            <span>{{ challenge.submissions || 0 }} 件作品</span>
            <span v-if="challenge.prize">奖励：{{ challenge.prize }}</span>
          </div>
          <button class="btn-join" v-if="challenge.status === 'active'" :disabled="joined" @click="handleJoin">
            {{ joined ? '已参与' : '参与挑战' }}
          </button>
        </div>

        <h2 class="section-title">参赛作品</h2>
        <div class="artwork-grid">
          <div class="artwork-card" v-for="item in submissions" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
            <div class="artwork-img" :style="item.cover_url ? `background-image:url(${item.cover_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
            <div class="artwork-info">
              <span class="title">{{ item.title || '未命名' }}</span>
              <span class="author">{{ item.author_name }}</span>
            </div>
          </div>
          <div class="empty" v-if="submissions.length === 0">暂无参赛作品</div>
        </div>
      </div>
      <div v-else class="loading">加载中...</div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { challengeAPI } from '@/api/index.js'

const route = useRoute()
const router = useRouter()
const challenge = ref({})
const submissions = ref([])
const joined = ref(false)

function statusText(s) {
  return { active: '进行中', ended: '已结束', upcoming: '即将开始' }[s] || ''
}

async function handleJoin() {
  if (!localStorage.getItem('auth_token')) { router.push('/login'); return }
  const res = await challengeAPI.join(challenge.value.id)
  if (res.success) { joined.value = true; challenge.value.participants = (challenge.value.participants || 0) + 1 }
}

onMounted(async () => {
  const id = route.params.id
  const [res, subRes] = await Promise.allSettled([
    challengeAPI.getDetail(id),
    challengeAPI.getSubmissions(id, { page: 1, limit: 20 })
  ])
  if (res.value?.success) challenge.value = res.value.data?.challenge || res.value.data || {}
  if (subRes.value?.success) submissions.value = subRes.value.data?.list || []
})
</script>

<style scoped>
.container { max-width: 900px; margin: 0 auto; padding: 0 20px; }
.back-btn { background: none; border: none; font-size: 14px; color: #666; cursor: pointer; padding: 20px 0; display: block; }
.back-btn:hover { color: #1a1a1a; }
.banner { width: 100%; height: 240px; border-radius: 16px; margin-bottom: 24px; }
.info-card { background: #fafafa; border: 1px solid #f0f0f0; border-radius: 16px; padding: 28px; margin-bottom: 32px; }
.header-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.tag { font-size: 12px; padding: 3px 10px; border-radius: 12px; font-weight: 500; }
.tag.active { background: #e8f5e9; color: #2e7d32; }
.tag.ended { background: #f5f5f5; color: #999; }
.tag.upcoming { background: #fff3e0; color: #e65100; }
.date { font-size: 12px; color: #999; }
.info-card h1 { font-size: 22px; font-weight: 700; color: #1a1a1a; margin-bottom: 10px; }
.desc { font-size: 14px; color: #888; line-height: 1.7; margin-bottom: 16px; }
.stats-row { display: flex; gap: 20px; font-size: 13px; color: #666; margin-bottom: 20px; }
.btn-join { padding: 10px 28px; border: none; border-radius: 8px; background: #1a1a1a; color: #fff; font-size: 14px; font-weight: 500; cursor: pointer; }
.btn-join:hover { background: #333; }
.btn-join:disabled { background: #ccc; cursor: not-allowed; }
.section-title { font-size: 18px; font-weight: 600; color: #1a1a1a; margin-bottom: 16px; }
.artwork-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 40px; }
.artwork-card { background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px; overflow: hidden; cursor: pointer; transition: border-color 0.2s; }
.artwork-card:hover { border-color: #ddd; }
.artwork-img { width: 100%; height: 140px; }
.artwork-info { padding: 10px 12px; }
.title { display: block; font-size: 13px; font-weight: 500; color: #1a1a1a; }
.author { display: block; font-size: 12px; color: #999; margin-top: 2px; }
.empty { grid-column: 1/-1; text-align: center; padding: 40px 0; color: #999; font-size: 14px; }
.loading { text-align: center; padding: 80px 0; color: #999; }
@media (max-width: 768px) { .artwork-grid { grid-template-columns: repeat(2, 1fr); } }
</style>


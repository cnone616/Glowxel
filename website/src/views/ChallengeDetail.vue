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
          <button class="btn-join" v-if="challenge.status === 'active'" @click="handleJoin">
            {{ joined ? '开始创作' : '参与并创作' }}
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
import { ref, onMounted, watch } from 'vue'
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
  if (joined.value) {
    router.push(`/editor?challengeId=${challenge.value.id}`)
    return
  }
  const res = await challengeAPI.join(challenge.value.id)
  if (res.success) {
    joined.value = true
    if (res.data?.changed) {
      challenge.value.participants = (challenge.value.participants || 0) + 1
    }
    router.push(`/editor?challengeId=${challenge.value.id}`)
  }
}

async function loadChallenge() {
  const id = route.params.id
  challenge.value = {}
  submissions.value = []
  joined.value = false
  const [res, subRes] = await Promise.allSettled([
    challengeAPI.getDetail(id),
    challengeAPI.getSubmissions(id, { page: 1, limit: 20 })
  ])
  if (res.value?.success) {
    challenge.value = res.value.data?.challenge || res.value.data || {}
    joined.value = !!challenge.value.joined
  }
  if (subRes.value?.success) submissions.value = subRes.value.data?.list || []
}

onMounted(loadChallenge)
watch(() => route.params.id, loadChallenge)
</script>

<style scoped>
.container { max-width: 900px; margin: 0 auto; padding: 24px 20px 56px; }
.back-btn { min-height: 42px; padding: 0 16px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-soft); font-size: 14px; font-weight: 800; color: var(--nb-ink); cursor: pointer; display: inline-flex; align-items: center; margin-bottom: 20px; }
.back-btn:hover { background: #f6f6f6; }
.banner { width: 100%; height: 240px; border: 3px solid var(--nb-ink); border-radius: 0; margin-bottom: 24px; box-shadow: var(--nb-shadow-card); }
.info-card { background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0; padding: 28px; margin-bottom: 32px; box-shadow: var(--nb-shadow-card); }
.header-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.tag { font-size: 12px; padding: 3px 10px; border: 2px solid var(--nb-ink); border-radius: 0; font-weight: 700; }
.tag.active { background: var(--tone-green-soft); color: var(--nb-ink); }
.tag.ended { background: #f5f5f5; color: var(--text-secondary); }
.tag.upcoming { background: var(--tone-yellow-soft); color: var(--nb-ink); }
.date { font-size: 12px; color: var(--text-secondary); }
.info-card h1 { font-size: 26px; font-weight: 800; color: var(--nb-ink); margin-bottom: 10px; }
.desc { font-size: 14px; color: var(--text-secondary); line-height: 1.7; margin-bottom: 16px; }
.stats-row { display: flex; gap: 20px; font-size: 13px; color: var(--text-secondary); margin-bottom: 20px; }
.btn-join { padding: 10px 28px; border: 2px solid var(--nb-ink); border-radius: 0; background: var(--nb-yellow); color: var(--nb-ink); font-size: 14px; font-weight: 800; cursor: pointer; box-shadow: var(--nb-shadow-soft); }
.btn-join:hover { background: var(--color-primary-hover); }
.btn-join:disabled { background: #ccc; cursor: not-allowed; }
.section-title { font-size: 24px; font-weight: 800; color: var(--nb-ink); margin-bottom: 16px; }
.artwork-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 40px; }
.artwork-card { background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0; overflow: hidden; cursor: pointer; transition: background-color 0.2s; box-shadow: var(--nb-shadow-soft); }
.artwork-card:hover { background: #f8f8f8; }
.artwork-img { width: 100%; height: 140px; border-bottom: 3px solid var(--nb-ink); }
.artwork-info { padding: 10px 12px; }
.title { display: block; font-size: 13px; font-weight: 700; color: var(--nb-ink); }
.author { display: block; font-size: 12px; color: var(--text-secondary); margin-top: 2px; }
.empty { grid-column: 1/-1; text-align: center; padding: 40px 0; color: var(--text-secondary); font-size: 14px; }
.loading { text-align: center; padding: 80px 0; color: var(--text-secondary); }
@media (max-width: 768px) { .artwork-grid { grid-template-columns: repeat(2, 1fr); } }
</style>

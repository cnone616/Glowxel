<template>
  <div class="follow-list">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <div class="tab-nav">
        <button :class="{ active: tab === 'followers' }" @click="switchTab('followers')">粉丝 {{ followerTotal }}</button>
        <button :class="{ active: tab === 'following' }" @click="switchTab('following')">关注 {{ followingTotal }}</button>
      </div>
      <div class="user-list">
        <div class="user-item" v-for="u in list" :key="u.id" @click="$router.push(`/user/${u.id}`)">
          <div class="avatar">{{ (u.name || '?')[0].toUpperCase() }}</div>
          <div class="user-info">
            <span class="name">{{ u.name }}</span>
            <span class="bio">{{ u.bio || '暂无简介' }}</span>
          </div>
          <button v-if="String(u.id) !== String(currentUserId)" class="btn-follow" @click.stop="handleToggle(u)">
            {{ u.is_following ? '已关注' : '关注' }}
          </button>
        </div>
        <div class="empty" v-if="list.length === 0 && !loading">暂无数据</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { followAPI } from '@/api/index.js'

const route = useRoute()
const router = useRouter()
const getTabFromRoute = () => {
  if (route.query.tab === 'followers' || route.query.tab === 'following') {
    return route.query.tab
  }
  return route.path.includes('/following') ? 'following' : 'followers'
}

const tab = ref(getTabFromRoute())
const list = ref([])
const loading = ref(false)
const followerTotal = ref(0)
const followingTotal = ref(0)

const currentUserId = JSON.parse(localStorage.getItem('user_info') || '{}').id
const userId = route.params.id || currentUserId

async function load(t) {
  loading.value = true
  const res = t === 'followers'
    ? await followAPI.getFollowers(userId, { page: 1, limit: 50 })
    : await followAPI.getFollowing(userId, { page: 1, limit: 50 })
  if (res.success) {
    list.value = res.data?.list || []
    if (t === 'followers') followerTotal.value = res.data?.total || list.value.length
    else followingTotal.value = res.data?.total || list.value.length
  }
  loading.value = false
}

async function switchTab(t) { tab.value = t; await load(t) }

async function handleToggle(u) {
  if (!currentUserId) {
    router.push('/login')
    return
  }
  if (String(u.id) === String(currentUserId)) return
  const res = await followAPI.toggle(u.id)
  if (res.success) u.is_following = res.data?.followed
}

onMounted(() => load(tab.value))

watch(() => route.fullPath, async () => {
  tab.value = getTabFromRoute()
  await load(tab.value)
})
</script>

<style scoped>
.container { max-width: 700px; margin: 0 auto; padding: 24px 20px 56px; }
.back-btn { min-height: 42px; padding: 0 16px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-soft); font-size: 14px; font-weight: 800; color: var(--nb-ink); cursor: pointer; display: inline-flex; align-items: center; margin-bottom: 18px; }
.back-btn:hover { background: #f6f6f6; }
.tab-nav { display: flex; gap: 10px; margin-bottom: 20px; }
.tab-nav button { padding: 10px 18px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); font-size: 14px; font-weight: 800; color: var(--text-secondary); cursor: pointer; box-shadow: var(--nb-shadow-soft); }
.tab-nav button.active { color: var(--nb-ink); background: var(--nb-yellow); }
.user-list { border: 3px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-card); }
.user-item { display: flex; align-items: center; gap: 12px; padding: 14px 16px; border-bottom: 2px solid var(--nb-ink); cursor: pointer; }
.user-item:last-child { border-bottom: none; }
.user-item:hover { background: #fafafa; }
.avatar { width: 44px; height: 44px; border: 2px solid var(--nb-ink); border-radius: 0; background: var(--tone-blue-soft); display: flex; align-items: center; justify-content: center; font-size: 18px; font-weight: 800; color: var(--nb-ink); flex-shrink: 0; box-shadow: var(--nb-shadow-soft); }
.user-info { flex: 1; }
.name { display: block; font-size: 14px; font-weight: 700; color: var(--nb-ink); }
.bio { display: block; font-size: 12px; color: var(--text-secondary); margin-top: 2px; }
.btn-follow { padding: 6px 16px; border-radius: 0; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); font-size: 13px; font-weight: 800; cursor: pointer; flex-shrink: 0; box-shadow: var(--nb-shadow-soft); }
.btn-follow:hover { background: #f6f6f6; }
.empty { text-align: center; padding: 60px 0; color: var(--text-secondary); font-size: 14px; }
</style>

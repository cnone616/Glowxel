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
          <button class="btn-follow" @click.stop="handleToggle(u)">
            {{ u._following ? '已关注' : '关注' }}
          </button>
        </div>
        <div class="empty" v-if="list.length === 0 && !loading">暂无数据</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { followAPI } from '@/api/index.js'

const route = useRoute()
const tab = ref(route.query.tab || 'followers')
const list = ref([])
const loading = ref(false)
const followerTotal = ref(0)
const followingTotal = ref(0)

const userId = route.params.id || JSON.parse(localStorage.getItem('user_info') || '{}').id

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
  const res = await followAPI.toggle(u.id)
  if (res.success) u._following = res.data?.followed
}

onMounted(() => load(tab.value))
</script>

<style scoped>
.container { max-width: 700px; margin: 0 auto; padding: 0 20px; }
.back-btn { background: none; border: none; font-size: 14px; color: #666; cursor: pointer; padding: 20px 0; display: block; }
.back-btn:hover { color: #1a1a1a; }
.tab-nav { display: flex; border-bottom: 1px solid #f0f0f0; margin-bottom: 20px; }
.tab-nav button { padding: 10px 24px; border: none; background: none; font-size: 14px; color: #999; cursor: pointer; border-bottom: 2px solid transparent; margin-bottom: -1px; }
.tab-nav button.active { color: #1a1a1a; font-weight: 600; border-bottom-color: #1a1a1a; }
.user-item { display: flex; align-items: center; gap: 12px; padding: 14px 0; border-bottom: 1px solid #f5f5f5; cursor: pointer; }
.user-item:hover { background: #fafafa; }
.avatar { width: 44px; height: 44px; border-radius: 50%; background: #e0e0e0; display: flex; align-items: center; justify-content: center; font-size: 18px; font-weight: 700; color: #999; flex-shrink: 0; }
.user-info { flex: 1; }
.name { display: block; font-size: 14px; font-weight: 500; color: #1a1a1a; }
.bio { display: block; font-size: 12px; color: #999; margin-top: 2px; }
.btn-follow { padding: 6px 16px; border-radius: 6px; border: 1px solid #e0e0e0; background: #fff; font-size: 13px; cursor: pointer; flex-shrink: 0; }
.btn-follow:hover { border-color: #1a1a1a; }
.empty { text-align: center; padding: 60px 0; color: #999; font-size: 14px; }
</style>


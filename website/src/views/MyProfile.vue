<template>
  <div class="my-profile">
    <div class="container">
      <div class="profile-header">
        <div class="avatar">{{ (user.name || '?')[0].toUpperCase() }}</div>
        <div class="profile-info">
          <h1>{{ user.name || '加载中...' }}</h1>
          <p class="bio">{{ user.bio || '这个人很懒，什么都没写' }}</p>
          <div class="stats">
            <span @click="tab = 'works'" style="cursor:pointer">{{ user.works_count || 0 }} 作品</span>
            <span @click="$router.push('/followers')" style="cursor:pointer">{{ user.followers_count || 0 }} 粉丝</span>
            <span @click="$router.push('/following')" style="cursor:pointer">{{ user.following_count || 0 }} 关注</span>
          </div>
          <div class="actions">
            <button class="btn-edit" @click="$router.push('/settings/profile')">编辑资料</button>
            <button class="btn-settings" @click="$router.push('/settings')">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
            </button>
          </div>
        </div>
      </div>

      <div class="tab-nav">
        <button :class="{ active: tab === 'works' }" @click="tab = 'works'">我的作品</button>
        <button :class="{ active: tab === 'favorites' }" @click="tab = 'favorites'">我的收藏</button>
      </div>

      <div class="artwork-grid" v-if="tab === 'works'">
        <div class="artwork-card" v-for="item in works" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
          <div class="artwork-img" :style="item.cover_url ? `background-image:url(${item.cover_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="artwork-info">
            <span class="title">{{ item.title || '未命名' }}</span>
            <span class="likes">{{ item.likes || 0 }}</span>
          </div>
        </div>
        <div class="empty" v-if="works.length === 0">暂无作品，<span @click="$router.push('/create')" style="color:#1a1a1a;cursor:pointer;text-decoration:underline">去创作</span></div>
      </div>

      <div class="artwork-grid" v-if="tab === 'favorites'">
        <div class="artwork-card" v-for="item in favorites" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
          <div class="artwork-img" :style="item.cover_url ? `background-image:url(${item.cover_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="artwork-info">
            <span class="title">{{ item.title || '未命名' }}</span>
            <span class="author">{{ item.author_name || '' }}</span>
          </div>
        </div>
        <div class="empty" v-if="favorites.length === 0">暂无收藏</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { userAPI, artworkAPI, collectAPI } from '@/api/index.js'
import { useRouter } from 'vue-router'

const router = useRouter()
const user = ref({})
const works = ref([])
const favorites = ref([])
const tab = ref('works')

async function loadWorks() {
  const info = JSON.parse(localStorage.getItem('user_info') || '{}')
  if (!info.id) return
  const res = await artworkAPI.getUserArtworks(info.id, { page: 1, limit: 20 })
  if (res.success) works.value = res.data?.list || []
}

async function loadFavorites() {
  const res = await collectAPI.getMyCollections({ page: 1, limit: 20 })
  if (res.success) favorites.value = res.data?.list || []
}

watch(tab, (v) => { if (v === 'favorites' && favorites.value.length === 0) loadFavorites() })

onMounted(async () => {
  const token = localStorage.getItem('auth_token')
  if (!token) { router.push('/login'); return }
  const res = await userAPI.getProfile()
  if (res.success) {
    user.value = res.data?.user || res.data || {}
    localStorage.setItem('user_info', JSON.stringify(user.value))
  }
  loadWorks()
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.profile-header { display: flex; gap: 24px; align-items: flex-start; padding: 40px 0 32px; }
.avatar { width: 80px; height: 80px; border-radius: 50%; background: #f0f0f0; display: flex; align-items: center; justify-content: center; font-size: 32px; font-weight: 700; color: #999; flex-shrink: 0; }
.profile-info h1 { font-size: 22px; font-weight: 700; color: #1a1a1a; }
.bio { font-size: 14px; color: #888; margin: 4px 0 8px; }
.stats { display: flex; gap: 20px; font-size: 13px; color: #666; margin-bottom: 12px; }
.actions { display: flex; gap: 8px; align-items: center; }
.btn-edit { padding: 7px 20px; border-radius: 6px; border: 1px solid #e0e0e0; background: #fff; font-size: 13px; cursor: pointer; }
.btn-edit:hover { border-color: #1a1a1a; }
.btn-settings { width: 32px; height: 32px; border-radius: 6px; border: 1px solid #e0e0e0; background: #fff; cursor: pointer; display: flex; align-items: center; justify-content: center; color: #666; }
.btn-settings:hover { border-color: #1a1a1a; color: #1a1a1a; }
.tab-nav { display: flex; gap: 0; border-bottom: 1px solid #f0f0f0; margin-bottom: 24px; }
.tab-nav button { padding: 10px 24px; border: none; background: none; font-size: 14px; color: #999; cursor: pointer; border-bottom: 2px solid transparent; margin-bottom: -1px; }
.tab-nav button.active { color: #1a1a1a; font-weight: 600; border-bottom-color: #1a1a1a; }
.artwork-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 40px; }
.artwork-card { background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px; overflow: hidden; cursor: pointer; transition: border-color 0.2s; }
.artwork-card:hover { border-color: #ddd; }
.artwork-img { width: 100%; height: 160px; }
.artwork-info { padding: 10px 12px; display: flex; justify-content: space-between; align-items: center; }
.title { font-size: 13px; font-weight: 500; color: #1a1a1a; }
.likes, .author { font-size: 12px; color: #999; }
.empty { grid-column: 1/-1; text-align: center; padding: 60px 0; color: #999; font-size: 14px; }
@media (max-width: 768px) { .artwork-grid { grid-template-columns: repeat(2, 1fr); } .profile-header { flex-direction: column; } }
</style>

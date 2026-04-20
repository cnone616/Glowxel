<template>
  <div class="user-profile">
    <div class="container">
      <div class="profile-header">
        <div class="avatar">{{ (user.name || '?')[0].toUpperCase() }}</div>
        <div class="profile-info">
          <h1>{{ user.name || '加载中...' }}</h1>
          <p class="bio">{{ user.bio || '这个人很懒，什么都没写' }}</p>
          <div class="stats">
            <span>{{ user.works_count || 0 }} 作品</span>
            <span>{{ user.followers_count || 0 }} 粉丝</span>
            <span>{{ user.following_count || 0 }} 关注</span>
          </div>
        </div>
      </div>
      <h2 class="section-title">TA 的作品</h2>
      <div class="artwork-grid">
        <div class="artwork-card" v-for="item in artworks" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
          <div class="artwork-img" :style="item.cover_url ? `background-image:url(${item.cover_url});background-size:cover;background-position:center` : 'background:#f0f0f0'"></div>
          <div class="artwork-info">
            <span class="title">{{ item.title || '未命名' }}</span>
            <span class="likes">
              <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor" style="color:#e53e3e;vertical-align:-1px"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
              {{ item.likes || 0 }}
            </span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { userAPI, artworkAPI } from '@/api/index.js'

const route = useRoute()
const user = ref({})
const artworks = ref([])

async function loadProfile() {
  const id = route.params.id
  user.value = {}
  artworks.value = []
  try {
    const uRes = await userAPI.getUserDetail(id)
    if (uRes.success) user.value = uRes.data?.user || {}
    const aRes = await artworkAPI.getUserArtworks(id, { page: 1, limit: 20 })
    if (aRes.success) artworks.value = aRes.data?.list || []
  } catch (e) { /* ignore */ }
}

onMounted(loadProfile)
watch(() => route.params.id, loadProfile)
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 24px 20px 56px; }

.profile-header { display: flex; gap: 24px; align-items: center; padding: 24px; margin-bottom: 24px; border: 3px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-card); }
.avatar { width: 72px; height: 72px; border: 3px solid var(--nb-ink); border-radius: 0; background: var(--tone-blue-soft); display: flex; align-items: center; justify-content: center; font-size: 28px; font-weight: 800; color: var(--nb-ink); box-shadow: var(--nb-shadow-soft); }
.profile-info h1 { font-size: 24px; font-weight: 800; color: var(--nb-ink); }
.bio { font-size: 14px; color: var(--text-secondary); margin-top: 4px; }
.stats { display: flex; gap: 20px; margin-top: 8px; font-size: 13px; color: var(--text-secondary); font-weight: 700; }

.section-title { font-size: 24px; font-weight: 800; color: var(--nb-ink); margin-bottom: 20px; }

.artwork-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 40px; }
.artwork-card { background: var(--tone-paper-soft); border: 3px solid var(--nb-ink); border-radius: 0; overflow: hidden; cursor: pointer; transition: background-color 0.2s; box-shadow: var(--nb-shadow-soft); }
.artwork-card:hover { background: #f8f8f8; }
.artwork-img { width: 100%; height: 160px; border-bottom: 3px solid var(--nb-ink); }
.artwork-info { padding: 12px; display: flex; justify-content: space-between; }
.title { font-size: 14px; font-weight: 700; color: var(--nb-ink); }
.likes { font-size: 12px; color: var(--text-secondary); font-weight: 700; }

@media (max-width: 768px) {
  .artwork-grid { grid-template-columns: repeat(2, 1fr); }
  .profile-header { flex-direction: column; text-align: center; }
  .stats { justify-content: center; }
}
</style>

<template>
  <div class="user-profile">
    <div class="container">
      <div class="profile-header">
        <div class="avatar">{{ (user.nickname || '?')[0] }}</div>
        <div class="profile-info">
          <h1>{{ user.nickname || '加载中...' }}</h1>
          <p class="bio">{{ user.bio || '这个人很懒，什么都没写' }}</p>
          <div class="stats">
            <span>{{ user.artworkCount || 0 }} 作品</span>
            <span>{{ user.followerCount || 0 }} 粉丝</span>
            <span>{{ user.followingCount || 0 }} 关注</span>
          </div>
        </div>
      </div>
      <h2 class="section-title">TA 的作品</h2>
      <div class="artwork-grid">
        <div class="artwork-card" v-for="item in artworks" :key="item.id" @click="$router.push(`/artwork/${item.id}`)">
          <div class="artwork-img" :style="{ background: '#f5f5f5' }"></div>
          <div class="artwork-info">
            <span class="title">{{ item.title || '未命名' }}</span>
            <span class="likes">❤ {{ item.likes || 0 }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { userAPI, artworkAPI } from '@/api/index.js'

const route = useRoute()
const user = ref({})
const artworks = ref([])

onMounted(async () => {
  const id = route.params.id
  try {
    const uRes = await userAPI.getUserDetail(id)
    if (uRes.success) user.value = uRes.data || {}
    const aRes = await artworkAPI.getUserArtworks(id, { page: 1, limit: 20 })
    if (aRes.success) artworks.value = aRes.data?.list || []
  } catch (e) { /* ignore */ }
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }

.profile-header { display: flex; gap: 24px; align-items: center; padding: 40px 0 32px; }
.avatar { width: 72px; height: 72px; border-radius: 50%; background: #f0f0f0; display: flex; align-items: center; justify-content: center; font-size: 28px; font-weight: 700; color: #999; }
.profile-info h1 { font-size: 22px; font-weight: 700; color: #1a1a1a; }
.bio { font-size: 14px; color: #888; margin-top: 4px; }
.stats { display: flex; gap: 20px; margin-top: 8px; font-size: 13px; color: #666; }

.section-title { font-size: 18px; font-weight: 600; color: #1a1a1a; margin-bottom: 20px; }

.artwork-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 40px; }
.artwork-card { background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px; overflow: hidden; cursor: pointer; transition: border-color 0.2s; }
.artwork-card:hover { border-color: #ddd; }
.artwork-img { width: 100%; height: 160px; }
.artwork-info { padding: 12px; display: flex; justify-content: space-between; }
.title { font-size: 14px; font-weight: 500; color: #1a1a1a; }
.likes { font-size: 12px; color: #999; }

@media (max-width: 768px) {
  .artwork-grid { grid-template-columns: repeat(2, 1fr); }
  .profile-header { flex-direction: column; text-align: center; }
  .stats { justify-content: center; }
}
</style>


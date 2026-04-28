<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">User Profile</span>
      <h1 class="glx-page-shell__title">{{ user.name || "用户主页" }}</h1>
      <p class="glx-page-shell__desc">{{ user.bio || "这个用户还没有留下简介。" }}</p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">作品</span>
          <strong class="glx-hero-metric__value">{{ user.works_count || 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">粉丝</span>
          <strong class="glx-hero-metric__value">{{ user.followers_count || 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">关注</span>
          <strong class="glx-hero-metric__value">{{ user.following_count || 0 }}</strong>
        </article>
      </div>
      <div class="glx-inline-actions">
        <button
          v-if="showFollowButton"
          type="button"
          class="glx-button glx-button--ghost"
          @click="toggleFollowUser"
        >
          {{ isFollowing ? "取消关注" : "关注 TA" }}
        </button>
        <router-link :to="`/user/${route.params.id}/followers`" class="glx-button glx-button--ghost">看粉丝</router-link>
        <router-link :to="`/user/${route.params.id}/following`" class="glx-button glx-button--ghost">看关注</router-link>
      </div>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">TA 的作品</h2>
        <span class="glx-section-meta">{{ artworks.length }} 件</span>
      </div>
      <div v-if="artworks.length === 0" class="glx-empty-card">
        <strong class="glx-section-title">还没有公开作品</strong>
        <p class="glx-page-shell__desc">等这个用户发布作品后，这里会自动展示对应列表。</p>
      </div>
      <div v-else class="glx-grid glx-grid--three">
        <article v-for="item in artworks" :key="item.id" class="glx-section-card glx-section-card--stack">
          <img
            v-if="typeof item.cover_url === 'string' && item.cover_url.length > 0"
            :src="item.cover_url"
            alt="artwork"
            class="artwork-cover"
          />
          <div v-else class="glx-empty-card">
            <strong class="glx-section-title">暂无封面</strong>
            <p class="glx-page-shell__desc">当前作品没有封面图。</p>
          </div>
          <strong class="glx-section-title">{{ item.title || "未命名作品" }}</strong>
          <p class="glx-page-shell__desc">{{ typeof item.likes === "number" ? `${item.likes} 赞` : "0 赞" }}</p>
          <div class="glx-inline-actions">
            <router-link :to="`/artwork/${item.id}`" class="glx-button glx-button--ghost">查看作品</router-link>
          </div>
        </article>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, ref, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { followAPI, userAPI, artworkAPI } from '@/api/index.js'
import { useFeedback } from '@/composables/useFeedback.js'
import { useUserStore } from '@/stores/user.js'

const route = useRoute()
const feedback = useFeedback()
const userStore = useUserStore()
const user = ref({})
const artworks = ref([])
const isFollowing = ref(false)

const showFollowButton = computed(() => {
  if (userStore.userId == null) {
    return true
  }
  return String(route.params.id) !== String(userStore.userId)
})

async function loadProfile() {
  const id = route.params.id
  user.value = {}
  artworks.value = []
  try {
    const [uRes, aRes] = await Promise.all([
      userAPI.getUserDetail(id),
      artworkAPI.getUserArtworks(id, { page: 1, limit: 20 }),
    ])

    if (uRes.success) {
      user.value = uRes.data?.user || {}
      if (typeof user.value.isFollowing === 'boolean') {
        isFollowing.value = user.value.isFollowing
      }
    }
    if (aRes.success) artworks.value = aRes.data?.list || []
  } catch (e) {
    user.value = {}
    artworks.value = []
  }
}

async function toggleFollowUser() {
  if (!userStore.isLoggedIn) {
    feedback.warning('需要登录', '登录后才能关注用户。')
    return
  }

  const response = await followAPI.toggle(route.params.id)
  if (!response.success) {
    feedback.error('操作失败', '关注状态没有成功更新。')
    return
  }

  if (response.data != null && typeof response.data.followed === 'boolean') {
    isFollowing.value = response.data.followed
  } else {
    isFollowing.value = !isFollowing.value
  }

  if (typeof user.value.followers_count === 'number') {
    if (isFollowing.value) {
      user.value.followers_count += 1
    } else {
      user.value.followers_count = Math.max(0, user.value.followers_count - 1)
    }
  }
}

onMounted(loadProfile)
watch(() => route.params.id, loadProfile)
</script>

<style scoped>
.artwork-cover {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: cover;
  border: 2px solid #111111;
}
</style>

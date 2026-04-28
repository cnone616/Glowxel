<template>
  <div class="glx-page-shell">
    <section class="glx-page-shell__hero">
      <span class="glx-page-shell__eyebrow">Artwork Detail</span>
      <h1 class="glx-page-shell__title">{{ detail.title || "作品详情" }}</h1>
      <p class="glx-page-shell__desc">{{ detail.description || "当前作品暂无描述。" }}</p>
      <div class="glx-hero-metrics">
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">作者</span>
          <strong class="glx-hero-metric__value">{{ detail.author_name || "--" }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">点赞</span>
          <strong class="glx-hero-metric__value">{{ detail.likes || 0 }}</strong>
        </article>
        <article class="glx-hero-metric">
          <span class="glx-hero-metric__label">评论</span>
          <strong class="glx-hero-metric__value">{{ comments.length }}</strong>
        </article>
      </div>
      <div class="glx-inline-actions">
        <button type="button" class="glx-button glx-button--ghost" @click="handleLike">
          {{ isLiked ? "取消点赞" : "点赞" }}
        </button>
        <button type="button" class="glx-button glx-button--ghost" @click="handleCollect">
          {{ isCollected ? "取消收藏" : "收藏" }}
        </button>
        <button
          v-if="showFollowAction"
          type="button"
          class="glx-button glx-button--ghost"
          @click="handleFollow"
        >
          {{ isFollowing ? "取消关注" : "关注作者" }}
        </button>
      </div>
    </section>

    <section class="glx-grid glx-grid--two">
      <article class="glx-section-card glx-section-card--stack">
        <img
          v-if="typeof detail.cover_url === 'string' && detail.cover_url.length > 0"
          :src="detail.cover_url"
          alt="artwork"
          class="artwork-cover"
        />
        <div v-else class="glx-empty-card">
          <strong class="glx-section-title">暂无封面</strong>
          <p class="glx-page-shell__desc">当前作品没有封面图。</p>
        </div>
      </article>

      <article class="glx-section-card glx-section-card--stack">
        <div class="glx-section-head">
          <h2 class="glx-section-title">作品信息</h2>
          <span class="glx-section-meta">公开浏览</span>
        </div>
        <div class="glx-stack">
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">作者主页</strong>
              <span class="glx-list-card__desc">公开主页和关注链路都继续保留。</span>
            </div>
            <router-link v-if="detail.author_id" :to="`/user/${detail.author_id}`" class="glx-button glx-button--ghost">查看</router-link>
          </div>
          <div class="glx-list-card">
            <div class="glx-list-card__copy">
              <strong class="glx-list-card__title">互动动作</strong>
              <span class="glx-list-card__desc">点赞、收藏、评论和关注动作已经恢复到作品详情页。</span>
            </div>
          </div>
        </div>
      </article>
    </section>

    <section class="glx-section-card glx-section-card--stack">
      <div class="glx-section-head">
        <h2 class="glx-section-title">评论列表</h2>
        <span class="glx-section-meta">{{ comments.length }} 条</span>
      </div>
      <div class="glx-inline-actions">
        <input v-model="commentText" class="glx-input artwork-comment-input" placeholder="写下你的评论..." />
        <button type="button" class="glx-button glx-button--primary" @click="submitComment">发送</button>
      </div>
      <div v-if="comments.length === 0" class="glx-empty-card">
        <strong class="glx-section-title">暂无评论</strong>
        <p class="glx-page-shell__desc">当前作品还没有公开评论。</p>
      </div>
      <div v-else class="glx-stack">
        <div v-for="comment in comments" :key="comment.id" class="glx-list-card">
          <div class="glx-list-card__copy">
            <strong class="glx-list-card__title">{{ comment.user_name || "匿名用户" }}</strong>
            <span class="glx-list-card__desc">{{ comment.content || "" }}</span>
          </div>
        </div>
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { artworkAPI, collectAPI, commentAPI, followAPI, likeAPI } from '@/api/index.js'
import { useFeedback } from '@/composables/useFeedback.js'
import { useUserStore } from '@/stores/user.js'

const route = useRoute()
const router = useRouter()
const feedback = useFeedback()
const userStore = useUserStore()
const detail = ref({})
const comments = ref([])
const commentText = ref('')
const isLiked = ref(false)
const isCollected = ref(false)
const isFollowing = ref(false)

const showFollowAction = computed(() => {
  if (!(typeof detail.value.author_id !== 'undefined' && detail.value.author_id !== null)) {
    return false
  }
  if (userStore.userId == null) {
    return true
  }
  return String(detail.value.author_id) !== String(userStore.userId)
})

async function loadArtwork() {
  const id = route.params.id
  detail.value = {}
  comments.value = []
  commentText.value = ''
  isLiked.value = false
  isCollected.value = false
  isFollowing.value = false
  const [res, cRes] = await Promise.allSettled([
    artworkAPI.getDetail(id),
    commentAPI.getList(id, { page: 1, limit: 20 }),
  ])

  if (res.status === 'fulfilled' && res.value?.success) {
    detail.value = res.value.data?.artwork || res.value.data || {}
    if (typeof detail.value.isLiked === 'boolean') {
      isLiked.value = detail.value.isLiked
    }
    if (typeof detail.value.isCollected === 'boolean') {
      isCollected.value = detail.value.isCollected
    }
    if (typeof detail.value.isFollowing === 'boolean') {
      isFollowing.value = detail.value.isFollowing
    }
  }
  if (cRes.status === 'fulfilled' && cRes.value?.success) {
    comments.value = cRes.value.data?.list || []
  }
}

function goToLogin() {
  router.push({
    name: 'Login',
    query: {
      redirect: route.fullPath,
    },
  })
}

async function handleLike() {
  if (!userStore.isLoggedIn) {
    goToLogin()
    return
  }

  let response
  if (isLiked.value) {
    response = await likeAPI.unlike(route.params.id)
  } else {
    response = await likeAPI.like(route.params.id)
  }

  if (!response.success) {
    feedback.error('操作失败', '点赞状态没有成功更新。')
    return
  }

  isLiked.value = !isLiked.value
  if (typeof detail.value.likes === 'number') {
    if (isLiked.value) {
      detail.value.likes += 1
    } else {
      detail.value.likes = Math.max(0, detail.value.likes - 1)
    }
  }
}

async function handleCollect() {
  if (!userStore.isLoggedIn) {
    goToLogin()
    return
  }

  let response
  if (isCollected.value) {
    response = await collectAPI.uncollect(route.params.id)
  } else {
    response = await collectAPI.collect(route.params.id)
  }

  if (!response.success) {
    feedback.error('操作失败', '收藏状态没有成功更新。')
    return
  }

  isCollected.value = !isCollected.value
}

async function handleFollow() {
  if (!userStore.isLoggedIn) {
    goToLogin()
    return
  }

  if (!(typeof detail.value.author_id !== 'undefined' && detail.value.author_id !== null)) {
    return
  }

  const response = await followAPI.toggle(detail.value.author_id)
  if (!response.success) {
    feedback.error('操作失败', '关注状态没有成功更新。')
    return
  }

  if (response.data != null && typeof response.data.followed === 'boolean') {
    isFollowing.value = response.data.followed
    return
  }

  isFollowing.value = !isFollowing.value
}

async function submitComment() {
  if (!userStore.isLoggedIn) {
    goToLogin()
    return
  }

  if (commentText.value.trim().length === 0) {
    return
  }

  const response = await commentAPI.add(route.params.id, commentText.value.trim())
  if (!response.success) {
    feedback.error('发送失败', '评论没有成功发送。')
    return
  }

  if (response.data != null && response.data.comment != null) {
    comments.value.unshift(response.data.comment)
  }
  commentText.value = ''
}

onMounted(loadArtwork)
watch(() => route.params.id, loadArtwork)
</script>

<style scoped>
.artwork-cover {
  display: block;
  width: 100%;
  aspect-ratio: 1;
  object-fit: contain;
  border: 2px solid #111111;
  background: #f0f0f0;
}

.artwork-comment-input {
  flex: 1;
}
</style>

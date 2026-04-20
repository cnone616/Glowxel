<template>
  <div class="artwork-detail">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <div class="detail-layout">
        <div class="artwork-preview">
          <div class="preview-img" :style="detail.cover_url ? `background-image:url(${detail.cover_url});background-size:contain;background-repeat:no-repeat;background-position:center;background-color:#f0f0f0` : 'background:#f0f0f0'"></div>
        </div>
        <div class="artwork-sidebar">
          <h1>{{ detail.title || '加载中...' }}</h1>
          <div class="author-row" v-if="detail.author_name">
            <span class="author-name" style="cursor:pointer" @click="$router.push(`/user/${detail.author_id}`)">{{ detail.author_name }}</span>
            <button v-if="String(detail.author_id) !== String(currentUserId)" class="follow-btn" @click="handleFollow">{{ isFollowing ? '已关注' : '关注' }}</button>
          </div>
          <p class="desc">{{ detail.description || '暂无描述' }}</p>
          <div class="actions">
            <button class="action-btn" :class="{ active: isLiked }" @click="handleLike">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
              {{ detail.likes || 0 }}
            </button>
            <button class="action-btn" :class="{ active: isCollected }" @click="handleCollect">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
              {{ isCollected ? '已收藏' : '收藏' }}
            </button>
          </div>
          <div class="comments-section">
            <h3>评论</h3>
            <div class="comment-input">
              <input v-model="commentText" placeholder="写下你的评论..." @keyup.enter="submitComment" />
              <button @click="submitComment">发送</button>
            </div>
            <div class="comment-list">
              <div class="comment-item" v-for="c in comments" :key="c.id">
                <span class="comment-author">{{ c.user_name }}</span>
                <span class="comment-text">{{ c.content }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { artworkAPI, likeAPI, collectAPI, commentAPI, followAPI } from '@/api/index.js'

const route = useRoute()
const detail = ref({})
const comments = ref([])
const commentText = ref('')
const isLiked = ref(false)
const isCollected = ref(false)
const isFollowing = ref(false)
const currentUserId = JSON.parse(localStorage.getItem('user_info') || '{}').id

async function loadArtwork() {
  const id = route.params.id
  detail.value = {}
  comments.value = []
  commentText.value = ''
  isLiked.value = false
  isCollected.value = false
  isFollowing.value = false
  const [res, cRes, likedRes, collectedRes] = await Promise.allSettled([
    artworkAPI.getDetail(id),
    commentAPI.getList(id, { page: 1, limit: 20 }),
    likeAPI.check(id),
    collectAPI.check(id),
  ])
  if (res.value?.success) {
    detail.value = res.value.data?.artwork || res.value.data || {}
    isFollowing.value = !!detail.value.isFollowing
    isLiked.value = typeof detail.value.isLiked === 'boolean' ? detail.value.isLiked : isLiked.value
    isCollected.value = typeof detail.value.isCollected === 'boolean' ? detail.value.isCollected : isCollected.value
  }
  if (cRes.value?.success) comments.value = cRes.value.data?.list || []
  if (likedRes.value?.success) isLiked.value = likedRes.value.data === true
  if (collectedRes.value?.success) isCollected.value = collectedRes.value.data === true
}

onMounted(loadArtwork)
watch(() => route.params.id, loadArtwork)

async function handleLike() {
  if (!detail.value.id) return
  if (isLiked.value) {
    await likeAPI.unlike(route.params.id)
    detail.value.likes = Math.max(0, (detail.value.likes || 1) - 1)
  } else {
    await likeAPI.like(route.params.id)
    detail.value.likes = (detail.value.likes || 0) + 1
  }
  isLiked.value = !isLiked.value
}

async function handleCollect() {
  if (!detail.value.id) return
  if (isCollected.value) {
    await collectAPI.uncollect(route.params.id)
  } else {
    await collectAPI.collect(route.params.id)
  }
  isCollected.value = !isCollected.value
}

async function handleFollow() {
  if (!detail.value.author_id || String(detail.value.author_id) === String(currentUserId)) return
  const res = await followAPI.toggle(detail.value.author_id)
  if (res.success) isFollowing.value = res.data?.followed ?? !isFollowing.value
}

async function submitComment() {
  if (!detail.value.id) return
  if (!commentText.value.trim()) return
  const res = await commentAPI.add(route.params.id, commentText.value.trim())
  if (res.success) {
    comments.value.unshift(res.data?.comment || { id: Date.now(), user_name: '我', content: commentText.value.trim() })
    commentText.value = ''
  }
}
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 24px 20px 56px; }
.back-btn { min-height: 42px; padding: 0 16px; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-soft); font-size: 14px; font-weight: 800; color: var(--nb-ink); cursor: pointer; display: inline-flex; align-items: center; margin-bottom: 20px; }
.back-btn:hover { background: #f6f6f6; }

.detail-layout { display: grid; grid-template-columns: 1fr 360px; gap: 32px; }
.preview-img { width: 100%; aspect-ratio: 1; border: 3px solid var(--nb-ink); border-radius: 0; box-shadow: var(--nb-shadow-card); }

.artwork-sidebar { align-self: start; padding: 20px; border: 3px solid var(--nb-ink); background: var(--tone-paper-soft); box-shadow: var(--nb-shadow-card); }
.artwork-sidebar h1 { font-size: 26px; font-weight: 800; color: var(--nb-ink); }
.author-row { display: flex; justify-content: space-between; align-items: center; margin: 12px 0; }
.author-name { font-size: 14px; color: var(--text-secondary); font-weight: 700; }
.follow-btn { padding: 6px 16px; border-radius: 0; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); font-size: 13px; font-weight: 800; cursor: pointer; box-shadow: var(--nb-shadow-soft); }
.follow-btn:hover { background: #f6f6f6; }
.desc { font-size: 14px; color: var(--text-secondary); line-height: 1.6; margin-bottom: 16px; }

.actions { display: flex; gap: 12px; margin-bottom: 24px; }
.action-btn { padding: 8px 16px; border-radius: 0; border: 2px solid var(--nb-ink); background: var(--tone-paper-soft); font-size: 13px; font-weight: 800; cursor: pointer; box-shadow: var(--nb-shadow-soft); }
.action-btn:hover { background: #f6f6f6; }

.comments-section { padding-top: 18px; border-top: 3px solid var(--nb-ink); }
.comments-section h3 { font-size: 18px; font-weight: 800; margin-bottom: 12px; color: var(--nb-ink); }
.comment-input { display: flex; gap: 8px; margin-bottom: 16px; }
.comment-input input { flex: 1; padding: 8px 12px; border: 2px solid var(--nb-ink); border-radius: 0; font-size: 13px; }
.comment-input button { padding: 8px 16px; border: 2px solid var(--nb-ink); background: var(--nb-yellow); color: var(--nb-ink); border-radius: 0; font-size: 13px; font-weight: 800; cursor: pointer; box-shadow: var(--nb-shadow-soft); }
.comment-item { padding: 8px 0; border-bottom: 2px solid var(--nb-ink); }
.comment-author { font-size: 13px; font-weight: 700; color: var(--nb-ink); margin-right: 8px; }
.comment-text { font-size: 13px; color: var(--text-secondary); }

@media (max-width: 768px) {
  .detail-layout { grid-template-columns: 1fr; }
}
</style>

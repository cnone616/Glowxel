<template>
  <div class="artwork-detail">
    <div class="container">
      <button class="back-btn" @click="$router.back()">← 返回</button>
      <div class="detail-layout">
        <div class="artwork-preview">
          <div class="preview-img" :style="{ background: '#f5f5f5' }"></div>
        </div>
        <div class="artwork-sidebar">
          <h1>{{ detail.title || '加载中...' }}</h1>
          <div class="author-row" v-if="detail.author">
            <span class="author-name">{{ detail.author }}</span>
            <button class="follow-btn">关注</button>
          </div>
          <p class="desc">{{ detail.description || '暂无描述' }}</p>
          <div class="actions">
            <button class="action-btn" @click="handleLike">❤ {{ detail.likes || 0 }}</button>
            <button class="action-btn" @click="handleCollect">⭐ 收藏</button>
          </div>
          <div class="comments-section">
            <h3>评论</h3>
            <div class="comment-input">
              <input v-model="commentText" placeholder="写下你的评论..." @keyup.enter="submitComment" />
              <button @click="submitComment">发送</button>
            </div>
            <div class="comment-list">
              <div class="comment-item" v-for="c in comments" :key="c.id">
                <span class="comment-author">{{ c.author }}</span>
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
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { artworkAPI, likeAPI, collectAPI, commentAPI } from '@/api/index.js'

const route = useRoute()
const detail = ref({})
const comments = ref([])
const commentText = ref('')

onMounted(async () => {
  const id = route.params.id
  try {
    const res = await artworkAPI.getDetail(id)
    if (res.success) detail.value = res.data || {}
    const cRes = await commentAPI.getList(id, { page: 1, limit: 20 })
    if (cRes.success) comments.value = cRes.data?.list || []
  } catch (e) { /* ignore */ }
})

async function handleLike() {
  await likeAPI.like(route.params.id)
  detail.value.likes = (detail.value.likes || 0) + 1
}
async function handleCollect() { await collectAPI.collect(route.params.id) }
async function submitComment() {
  if (!commentText.value.trim()) return
  await commentAPI.add({ artworkId: route.params.id, content: commentText.value })
  commentText.value = ''
}
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.back-btn { background: none; border: none; font-size: 14px; color: #666; cursor: pointer; padding: 20px 0; }
.back-btn:hover { color: #1a1a1a; }

.detail-layout { display: grid; grid-template-columns: 1fr 360px; gap: 32px; }
.preview-img { width: 100%; aspect-ratio: 1; border-radius: 12px; }

.artwork-sidebar h1 { font-size: 22px; font-weight: 700; color: #1a1a1a; }
.author-row { display: flex; justify-content: space-between; align-items: center; margin: 12px 0; }
.author-name { font-size: 14px; color: #666; }
.follow-btn { padding: 6px 16px; border-radius: 6px; border: 1px solid #e0e0e0; background: #fff; font-size: 13px; cursor: pointer; }
.follow-btn:hover { border-color: #1a1a1a; }
.desc { font-size: 14px; color: #888; line-height: 1.6; margin-bottom: 16px; }

.actions { display: flex; gap: 12px; margin-bottom: 24px; }
.action-btn { padding: 8px 16px; border-radius: 8px; border: 1px solid #f0f0f0; background: #fafafa; font-size: 13px; cursor: pointer; }
.action-btn:hover { border-color: #ddd; }

.comments-section h3 { font-size: 16px; font-weight: 600; margin-bottom: 12px; }
.comment-input { display: flex; gap: 8px; margin-bottom: 16px; }
.comment-input input { flex: 1; padding: 8px 12px; border: 1px solid #e0e0e0; border-radius: 6px; font-size: 13px; }
.comment-input button { padding: 8px 16px; border: none; background: #1a1a1a; color: #fff; border-radius: 6px; font-size: 13px; cursor: pointer; }
.comment-item { padding: 8px 0; border-bottom: 1px solid #f5f5f5; }
.comment-author { font-size: 13px; font-weight: 500; color: #1a1a1a; margin-right: 8px; }
.comment-text { font-size: 13px; color: #666; }

@media (max-width: 768px) {
  .detail-layout { grid-template-columns: 1fr; }
}
</style>


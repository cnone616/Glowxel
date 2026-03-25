<template>
  <div class="workspace">
    <div class="container">
      <!-- 顶部 -->
      <div class="ws-header">
        <div class="ws-header-left">
          <h1 class="ws-title">工作台</h1>
          <span class="ws-count">{{ projects.length }} 个项目</span>
        </div>
        <div class="ws-header-right">
          <div class="search-box">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input v-model="keyword" placeholder="搜索项目..." />
          </div>
          <router-link to="/create" class="btn-new">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>
            新建项目
          </router-link>
        </div>
      </div>

      <!-- 加载中 -->
      <div v-if="loading" class="state-box">
        <div class="spinner"></div>
        <p>加载中...</p>
      </div>

      <!-- 空状态 -->
      <div v-else-if="!filteredProjects.length" class="state-box">
        <div class="empty-icon">
          <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#ccc" stroke-width="1.5"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M3 9h18"/><path d="M9 21V9"/></svg>
        </div>
        <p class="empty-title">{{ keyword ? '没有找到匹配的项目' : '还没有项目' }}</p>
        <p class="empty-desc">{{ keyword ? '换个关键词试试' : '点击新建项目开始创作' }}</p>
        <router-link v-if="!keyword" to="/create" class="btn-new" style="margin-top:16px">新建项目</router-link>
      </div>

      <!-- 项目网格 -->
      <div v-else class="project-grid">
        <div class="project-card" v-for="p in filteredProjects" :key="p.id">
          <div class="project-thumb" :style="p.thumbnail_url ? `background-image:url(${p.thumbnail_url})` : ''">
            <div v-if="!p.thumbnail_url" class="thumb-placeholder">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#ccc" stroke-width="1.5"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
            </div>
            <div class="project-actions">
              <button class="action-btn" title="编辑" @click="openEditor(p)">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>
              </button>
              <button class="action-btn danger" title="删除" @click="deleteProject(p)">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/><path d="M10 11v6"/><path d="M14 11v6"/><path d="M9 6V4h6v2"/></svg>
              </button>
            </div>
          </div>
          <div class="project-info">
            <p class="project-name">{{ p.title || p.name || '未命名' }}</p>
            <p class="project-meta">{{ p.width }}×{{ p.height }} · {{ formatDate(p.updated_at || p.created_at) }}</p>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>



<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { projectAPI } from '@/api/index.js'

const router = useRouter()
const projects = ref([])
const loading = ref(true)
const keyword = ref('')

const filteredProjects = computed(() => {
  if (!keyword.value) return projects.value
  return projects.value.filter(p =>
    (p.name || '').toLowerCase().includes(keyword.value.toLowerCase())
  )
})

function formatDate(dateStr) {
  if (!dateStr) return ''
  const d = new Date(dateStr)
  return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`
}

function openEditor(p) { router.push(`/editor/${p.id}`) }

async function deleteProject(p) {
  if (!confirm(`确认删除「${p.name || '未命名'}」？`)) return
  const res = await projectAPI.remove(p.id)
  if (res.success || res.code === 0) {
    projects.value = projects.value.filter(x => x.id !== p.id)
  }
}

onMounted(async () => {
  try {
    const token = localStorage.getItem('auth_token')
    if (!token) { loading.value = false; return }
    const res = await projectAPI.getList({ page: 1, limit: 50 })
    if (res.success) projects.value = res.data?.list || []
  } finally { loading.value = false }
})
</script>

<style scoped>
.workspace { padding: 40px 0 80px; min-height: calc(100vh - 60px); }
.ws-header { display: flex; align-items: center; justify-content: space-between; margin-bottom: 32px; gap: 16px; flex-wrap: wrap; }
.ws-header-left { display: flex; align-items: baseline; gap: 12px; }
.ws-title { font-size: 24px; font-weight: 700; color: var(--color-text-primary); }
.ws-count { font-size: 13px; color: var(--color-text-muted); }
.ws-header-right { display: flex; align-items: center; gap: 10px; }
.search-box { display: flex; align-items: center; gap: 8px; border: 1px solid var(--color-border); border-radius: var(--radius-sm); padding: 7px 12px; background: #fff; transition: border-color 0.15s; }
.search-box:focus-within { border-color: var(--color-border-hover); }
.search-box input { border: none; outline: none; font-size: 13px; width: 180px; color: var(--color-text-primary); }
.search-box svg { color: var(--color-text-muted); flex-shrink: 0; }
.btn-new { display: inline-flex; align-items: center; gap: 6px; padding: 8px 16px; background: var(--color-primary); color: #fff; border-radius: var(--radius-sm); font-size: 13px; font-weight: 500; text-decoration: none; border: none; cursor: pointer; transition: background 0.15s; }
.btn-new:hover { background: var(--color-primary-hover); }
.state-box { display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 80px 0; gap: 12px; }
.spinner { width: 28px; height: 28px; border: 2px solid var(--color-border); border-top-color: var(--color-primary); border-radius: 50%; animation: spin 0.7s linear infinite; }
@keyframes spin { to { transform: rotate(360deg); } }
.empty-icon { width: 72px; height: 72px; background: var(--color-bg-muted); border-radius: 50%; display: flex; align-items: center; justify-content: center; }
.empty-title { font-size: 15px; font-weight: 500; color: var(--color-text-primary); }
.empty-desc { font-size: 13px; color: var(--color-text-muted); }
.project-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 16px; }
.project-card { border: 1px solid var(--color-border); border-radius: var(--radius-md); overflow: hidden; background: #fff; transition: box-shadow 0.15s; }
.project-card:hover { box-shadow: var(--shadow-md); }
.project-thumb { position: relative; aspect-ratio: 1; background: var(--color-bg-muted); background-size: cover; background-position: center; }
.thumb-placeholder { width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; }
.project-actions { position: absolute; top: 8px; right: 8px; display: flex; gap: 4px; opacity: 0; transition: opacity 0.15s; }
.project-card:hover .project-actions { opacity: 1; }
.action-btn { width: 28px; height: 28px; border-radius: var(--radius-sm); background: rgba(255,255,255,0.9); border: 1px solid var(--color-border); display: flex; align-items: center; justify-content: center; cursor: pointer; color: var(--color-text-secondary); transition: all 0.15s; }
.action-btn:hover { background: #fff; color: var(--color-primary); border-color: var(--color-primary); }
.action-btn.danger:hover { color: var(--color-danger); border-color: var(--color-danger); }
.project-info { padding: 12px; }
.project-name { font-size: 13px; font-weight: 500; color: var(--color-text-primary); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.project-meta { font-size: 11px; color: var(--color-text-muted); margin-top: 3px; }
@media (max-width: 768px) {
  .ws-header { flex-direction: column; align-items: flex-start; }
  .ws-header-right { width: 100%; }
  .search-box { flex: 1; }
  .search-box input { width: 100%; }
  .project-grid { grid-template-columns: repeat(2, 1fr); }
}
</style>

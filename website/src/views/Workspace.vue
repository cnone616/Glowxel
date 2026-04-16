<template>
  <div class="workspace-page">
    <div class="container">
      <div class="header">
        <div class="header-content">
          <span class="header-kicker">Workspace</span>
          <h1 class="header-title">创作中心</h1>
          <p class="header-desc">从这里开始新建、导入图片，或者继续编辑已有画布。</p>
        </div>
      </div>

      <div class="main-content">
        <section class="quick-actions">
          <div class="section-intro">
            <h2 class="section-title">快速开始</h2>
            <span class="section-note">先选一种方式进入创作流程</span>
          </div>
          <div class="action-grid">
            <router-link to="/create?mode=blank" class="action-card">
              <div class="action-icon blue">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>
              </div>
              <strong class="action-title">新建画布</strong>
              <span class="action-subtitle">从空白开始创作</span>
            </router-link>

            <router-link to="/create?mode=image" class="action-card">
              <div class="action-icon green">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M4 4h16v16H4z"/><path d="M9 4v16"/><path d="M15 4v16"/><path d="M4 9h16"/><path d="M4 15h16"/></svg>
              </div>
              <strong class="action-title">导入图片</strong>
              <span class="action-subtitle">转换为像素画</span>
            </router-link>
          </div>
        </section>

        <section class="my-projects">
          <div class="section-header">
            <h2 class="section-title">我的画布</h2>
            <span class="project-count">{{ filteredProjects.length }} 个</span>
          </div>

          <div v-if="loading" class="state-box">
            <div class="spinner"></div>
            <p>加载中...</p>
          </div>

          <div v-else-if="!filteredProjects.length" class="empty-state">
            <div class="empty-icon">
              <svg width="36" height="36" viewBox="0 0 24 24" fill="none" stroke="#9aa6b2" stroke-width="1.7"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M3 9h18"/><path d="M9 21V9"/></svg>
            </div>
            <strong class="empty-title">还没有画布</strong>
            <span class="empty-subtitle">开始您的第一个创作吧</span>
            <router-link to="/create?mode=blank" class="empty-btn">立即创建</router-link>
          </div>

          <div v-else class="projects-grid">
            <div class="project-card" v-for="project in filteredProjects" :key="project.id">
              <div
                class="project-thumb"
                :style="project.thumbnail_url ? `background-image:url(${project.thumbnail_url})` : ''"
                @click="openEditor(project)"
              >
                <div v-if="!project.thumbnail_url" class="thumb-placeholder">
                  <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#c4ccd6" stroke-width="1.6"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
                </div>
                <div class="project-actions">
                  <button class="action-btn" type="button" @click.stop="openEditor(project)">编辑</button>
                  <button class="action-btn danger" type="button" @click.stop="deleteProject(project)">删除</button>
                </div>
              </div>
              <div class="project-info">
                <strong class="project-name">{{ project.title || project.name || '未命名' }}</strong>
                <span class="project-meta">{{ project.width }} × {{ project.height }}</span>
                <span class="project-date">{{ formatDate(project.updated_at || project.created_at) }}</span>
              </div>
            </div>
          </div>
        </section>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { projectAPI } from '@/api/index.js'

const router = useRouter()
const projects = ref([])
const loading = ref(true)

const filteredProjects = computed(() => projects.value)

function formatDate(dateStr) {
  if (!dateStr) {
    return ''
  }

  const date = new Date(dateStr)
  return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`
}

function openEditor(project) {
  router.push(`/editor/${project.id}`)
}

async function deleteProject(project) {
  if (!confirm(`确认删除「${project.title || project.name || '未命名'}」？`)) {
    return
  }

  const response = await projectAPI.remove(project.id)
  if (response.success || response.code === 0) {
    projects.value = projects.value.filter((item) => item.id !== project.id)
  }
}

onMounted(async () => {
  try {
    const token = localStorage.getItem('auth_token')
    if (!token) {
      loading.value = false
      return
    }

    const response = await projectAPI.getList({ page: 1, limit: 50 })
    if (response.success) {
      projects.value = response.data?.list || []
    }
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.workspace-page {
  min-height: calc(100vh - 60px);
  padding: 24px 0 56px;
}

.container {
  max-width: 1120px;
  margin: 0 auto;
  padding: 0 24px;
}

.header {
  margin-bottom: 24px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 30px;
  background:
    radial-gradient(circle at top right, rgba(136, 161, 186, 0.16), transparent 30%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.9), rgba(251, 247, 241, 0.82));
  box-shadow: var(--nb-shadow-strong);
}

.header-content {
  min-height: 168px;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  gap: 10px;
  padding: 30px 32px;
}

.header-kicker {
  display: inline-flex;
  align-items: center;
  min-height: 30px;
  padding: 0 12px;
  border-radius: 999px;
  background: rgba(136, 161, 186, 0.14);
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.header-title {
  font-size: 34px;
  font-weight: 700;
  color: var(--nb-ink);
}

.header-desc {
  max-width: 560px;
  font-size: 15px;
  line-height: 1.75;
  color: var(--text-secondary);
}

.main-content {
  display: grid;
  gap: 28px;
}

.section-intro {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
}

.quick-actions,
.my-projects {
  padding: 24px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 28px;
  background: rgba(255, 255, 255, 0.8);
  box-shadow: var(--nb-shadow-card);
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18px;
}

.action-card {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 10px;
  padding: 28px 18px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.74);
  text-decoration: none;
  box-shadow: var(--nb-shadow-soft);
  transition: transform 0.18s ease, box-shadow 0.18s ease, border-color 0.18s ease;
}

.action-card:hover {
  transform: translateY(-2px);
  border-color: rgba(136, 161, 186, 0.18);
  box-shadow: var(--nb-shadow-card);
}

.action-icon {
  width: 72px;
  height: 72px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 22px;
}

.action-icon.blue {
  background: rgba(136, 161, 186, 0.18);
  color: var(--nb-ink);
}

.action-icon.green {
  background: rgba(155, 179, 158, 0.2);
  color: var(--nb-ink);
}

.action-title {
  font-size: 18px;
  color: var(--nb-ink);
  font-weight: 700;
}

.action-subtitle {
  font-size: 14px;
  color: var(--text-secondary);
}

.section-note {
  color: var(--text-secondary);
  font-size: 14px;
  font-weight: 700;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
}

.section-title {
  font-size: 24px;
  font-weight: 700;
  color: var(--nb-ink);
}

.project-count {
  color: var(--text-secondary);
  font-size: 14px;
  font-weight: 700;
}

.state-box,
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;
  min-height: 260px;
}

.spinner {
  width: 28px;
  height: 28px;
  border: 3px solid rgba(136, 161, 186, 0.2);
  border-top-color: var(--nb-blue);
  border-radius: 999px;
  animation: spin 0.7s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.empty-icon {
  width: 72px;
  height: 72px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 22px;
  background: var(--tone-blue-soft);
  box-shadow: var(--nb-shadow-soft);
}

.empty-title {
  font-size: 18px;
  color: var(--nb-ink);
  font-weight: 700;
}

.empty-subtitle {
  font-size: 14px;
  color: var(--text-secondary);
}

.empty-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 44px;
  padding: 10px 18px;
  border: 1px solid rgba(215, 178, 109, 0.36);
  border-radius: 999px;
  background: var(--nb-yellow);
  box-shadow: var(--nb-shadow-soft);
  color: var(--nb-ink);
  text-decoration: none;
  font-size: 14px;
  font-weight: 700;
}

.projects-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 18px;
}

.project-card {
  overflow: hidden;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.78);
  box-shadow: var(--nb-shadow-soft);
}

.project-thumb {
  position: relative;
  aspect-ratio: 1;
  background: rgba(255, 255, 255, 0.72);
  background-size: cover;
  background-position: center;
  cursor: pointer;
  border-bottom: 1px solid rgba(36, 49, 66, 0.08);
}

.thumb-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.project-actions {
  position: absolute;
  top: 10px;
  right: 10px;
  display: flex;
  gap: 8px;
}

.action-btn {
  height: 30px;
  padding: 0 12px;
  border: 1px solid rgba(36, 49, 66, 0.08);
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.84);
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: 700;
  cursor: pointer;
  box-shadow: var(--nb-shadow-soft);
}

.action-btn.danger {
  background: rgba(213, 155, 142, 0.22);
  color: var(--nb-ink);
}

.project-info {
  display: grid;
  gap: 6px;
  padding: 14px;
}

.project-name {
  font-size: 15px;
  color: var(--nb-ink);
  font-weight: 700;
}

.project-meta,
.project-date {
  font-size: 12px;
  color: var(--text-secondary);
}

@media (max-width: 768px) {
  .workspace-page {
    min-height: calc(100vh - 56px);
    padding: 10px 0 28px;
  }

  .container {
    padding: 0 16px;
  }

  .header {
    margin-bottom: 12px;
  }

  .header-content {
    min-height: 128px;
    padding: 22px 20px;
  }

  .header-title {
    font-size: 26px;
  }

  .header-desc {
    font-size: 14px;
  }

  .quick-actions,
  .my-projects {
    padding: 14px;
  }

  .action-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
  }

  .action-card {
    padding: 18px 10px;
    align-items: center;
  }

  .action-icon {
    width: 56px;
    height: 56px;
  }

  .action-title {
    font-size: 16px;
  }

  .action-subtitle {
    font-size: 12px;
    text-align: center;
  }

  .section-title {
    font-size: 20px;
  }

  .section-intro,
  .section-header {
    margin-bottom: 14px;
  }

  .section-note,
  .project-count {
    font-size: 12px;
  }

  .projects-grid {
    grid-template-columns: 1fr;
    gap: 12px;
  }
}
</style>

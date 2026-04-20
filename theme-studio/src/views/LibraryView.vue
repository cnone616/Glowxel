<script setup>
import { computed } from 'vue'
import { useRouter } from 'vue-router'
import { OFFICIAL_TEMPLATES } from '../data/studioCatalog'
import { useProjectStore } from '../stores/projectStore'
import ProjectCard from '../components/shared/ProjectCard.vue'
import TemplateCard from '../components/shared/TemplateCard.vue'

const router = useRouter()
const projectStore = useProjectStore()

const projects = computed(() => projectStore.projects)

async function createFromTemplate(templateKey) {
  const project = await projectStore.createProjectFromTemplate(templateKey)
  router.push(`/studio/${project.id}`)
}

async function createBlank() {
  const project = await projectStore.createBlankProject()
  router.push(`/studio/${project.id}`)
}

function openProject(projectId) {
  router.push(`/studio/${projectId}`)
}

async function removeProject(projectId) {
  await projectStore.deleteProject(projectId)
}
</script>

<template>
  <section class="library-view">
    <section class="hero panel">
      <div class="hero-copy">
        <p class="section-kicker">Material Driven Studio</p>
        <h1 class="section-title">上传图片素材，布背景和路径，让动态时钟主题更贴近真实项目制作方式。</h1>
        <p class="muted">
          这版工作室不要求你从零手动画每一帧，而是围绕素材上传、背景轮播、对象路径、闪烁规则、
          时间组件和分钟 / 整点预演来搭建主题。
        </p>
        <div class="hero-actions">
          <button @click="createBlank">从空白工作室开始</button>
          <span class="chip">PNG / JPG / WebP / GIF</span>
          <span class="chip">路径移动</span>
          <span class="chip">背景轮播</span>
          <span class="chip">多事件动作</span>
        </div>
      </div>
      <div class="hero-poster">
        <div class="poster-screen">
          <span>08:45</span>
          <small>Scene Builder</small>
        </div>
      </div>
    </section>

    <section class="library-grid">
      <div class="library-column">
        <div class="section-head">
          <div>
            <p class="section-kicker">Projects</p>
            <h2>最近项目</h2>
          </div>
        </div>

        <div v-if="projects.length" class="project-grid">
          <ProjectCard
            v-for="project in projects"
            :key="project.id"
            :project="project"
            @open="openProject(project.id)"
            @delete="removeProject(project.id)"
          />
        </div>
        <div v-else class="empty-state panel">
          <strong>这里还没有项目</strong>
          <p class="muted">直接从空白工作室开始会更接近你真实做主题的方式。</p>
        </div>
      </div>

      <div class="library-column">
        <div class="section-head">
          <div>
            <p class="section-kicker">Templates</p>
            <h2>现有模板骨架</h2>
          </div>
        </div>

        <div class="template-grid">
          <TemplateCard
            v-for="template in OFFICIAL_TEMPLATES"
            :key="template.key"
            :template="template"
            @create="createFromTemplate(template.key)"
          />
        </div>
      </div>
    </section>
  </section>
</template>

<style scoped>
.library-view {
  display: grid;
  gap: 24px;
}

.hero {
  display: grid;
  grid-template-columns: minmax(0, 1.15fr) minmax(280px, 420px);
  gap: 24px;
  padding: 28px;
  overflow: hidden;
}

.hero-copy {
  display: grid;
  gap: 16px;
  align-content: center;
}

.hero-actions {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  align-items: center;
}

.hero-poster {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 320px;
  border-radius: 24px;
  background:
    radial-gradient(circle at center, rgba(102, 234, 214, 0.18), transparent 42%),
    linear-gradient(135deg, rgba(255, 191, 105, 0.12), rgba(102, 234, 214, 0.08)),
    rgba(255, 255, 255, 0.02);
}

.poster-screen {
  width: min(78%, 300px);
  aspect-ratio: 1;
  display: grid;
  place-items: center;
  border-radius: 22px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(4, 8, 9, 0.88);
  box-shadow: inset 0 0 0 1px rgba(102, 234, 214, 0.06);
}

.poster-screen span {
  font-family: var(--font-display);
  font-size: clamp(42px, 8vw, 72px);
  color: var(--accent-amber);
  letter-spacing: 0.08em;
}

.poster-screen small {
  color: var(--accent-cyan);
  letter-spacing: 0.18em;
  text-transform: uppercase;
}

.library-grid {
  display: grid;
  grid-template-columns: 0.92fr 1.08fr;
  gap: 24px;
}

.library-column {
  display: grid;
  gap: 18px;
}

.project-grid,
.template-grid {
  display: grid;
  gap: 18px;
}

.template-grid {
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
}

.empty-state {
  padding: 24px;
  display: grid;
  gap: 10px;
}

@media (max-width: 1080px) {
  .hero,
  .library-grid {
    grid-template-columns: 1fr;
  }
}
</style>

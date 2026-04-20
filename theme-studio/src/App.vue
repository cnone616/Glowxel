<script setup>
import { computed, onMounted } from 'vue'
import { RouterLink, RouterView } from 'vue-router'
import { useProjectStore } from './stores/projectStore'
import { usePublishStore } from './stores/publishStore'

const projectStore = useProjectStore()
const publishStore = usePublishStore()

const projectCount = computed(() => projectStore.projects.length)
const publishCount = computed(() => publishStore.records.length)

onMounted(async () => {
  await Promise.all([projectStore.boot(), publishStore.boot()])
})
</script>

<template>
  <div class="app-shell">
    <header class="shell-header">
      <RouterLink to="/" class="brand-mark">
        <span class="brand-pixel"></span>
        <span>
          <strong>Theme Studio</strong>
          <small>素材驱动的像素时钟主题工作室</small>
        </span>
      </RouterLink>

      <nav class="shell-nav">
        <RouterLink to="/">项目库</RouterLink>
        <RouterLink v-if="projectStore.activeProject" :to="`/studio/${projectStore.activeProject.id}`">工作室</RouterLink>
        <RouterLink v-if="projectStore.activeProject" :to="`/validate/${projectStore.activeProject.id}`">验包</RouterLink>
        <RouterLink v-if="projectStore.activeProject" :to="`/publish/${projectStore.activeProject.id}`">发布</RouterLink>
      </nav>

      <div class="shell-stats">
        <span>{{ projectCount }} 个项目</span>
        <span>{{ publishCount }} 条发布记录</span>
      </div>
    </header>

    <main class="shell-main">
      <RouterView />
    </main>
  </div>
</template>

<style scoped>
.app-shell {
  min-height: 100vh;
}

.shell-header {
  position: sticky;
  top: 0;
  z-index: 20;
  display: grid;
  grid-template-columns: auto 1fr auto;
  gap: 20px;
  align-items: center;
  padding: 12px 16px;
  backdrop-filter: blur(18px);
  background: rgba(10, 14, 18, 0.78);
  border-bottom: 1px solid rgba(102, 234, 214, 0.14);
}

.brand-mark {
  display: inline-flex;
  align-items: center;
  gap: 14px;
  color: inherit;
  text-decoration: none;
}

.brand-mark strong,
.brand-mark small {
  display: block;
}

.brand-mark small {
  color: var(--text-soft);
  font-size: 12px;
  letter-spacing: 0.08em;
}

.brand-pixel {
  width: 18px;
  height: 18px;
  background:
    linear-gradient(90deg, transparent 0 25%, var(--accent-cyan) 25% 50%, transparent 50% 75%, var(--accent-amber) 75% 100%),
    linear-gradient(transparent 0 25%, var(--accent-amber) 25% 50%, transparent 50% 75%, var(--accent-cyan) 75% 100%);
  box-shadow: 0 0 18px rgba(102, 234, 214, 0.4);
}

.shell-nav {
  display: inline-flex;
  justify-content: center;
  gap: 12px;
  flex-wrap: wrap;
}

.shell-nav a {
  padding: 8px 12px;
  border-radius: 999px;
  color: var(--text-soft);
  text-decoration: none;
  transition: background 0.25s ease, color 0.25s ease;
}

.shell-nav a.router-link-active {
  background: rgba(102, 234, 214, 0.12);
  color: var(--text-strong);
}

.shell-stats {
  display: inline-flex;
  gap: 16px;
  color: var(--text-soft);
  font-size: 13px;
}

.shell-main {
  padding: 12px;
}

@media (max-width: 980px) {
  .shell-header {
    grid-template-columns: 1fr;
    justify-items: start;
  }

  .shell-nav {
    justify-content: flex-start;
  }

  .shell-stats {
    flex-wrap: wrap;
  }

  .shell-main {
    padding: 10px;
  }
}
</style>

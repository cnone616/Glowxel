<script setup>
import { computed, onMounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useProjectStore } from '../stores/projectStore'
import { usePublishStore } from '../stores/publishStore'

const route = useRoute()
const router = useRouter()
const projectStore = useProjectStore()
const publishStore = usePublishStore()

const project = computed(() => projectStore.activeProject)
const bundle = computed(() => publishStore.activeBundle)

async function loadProject(projectId) {
  const loaded = await projectStore.loadProject(projectId)
  if (!loaded) {
    router.push('/')
    return
  }

  await publishStore.ensureBundle(loaded)
}

watch(
  () => route.params.id,
  async (nextId) => {
    if (typeof nextId === 'string') {
      await loadProject(nextId)
    }
  },
  { immediate: true },
)

onMounted(async () => {
  if (!projectStore.initialized) {
    await projectStore.boot()
  }
})
</script>

<template>
  <section v-if="project && bundle" class="validate-view">
    <header class="panel page-head compact-panel">
      <div>
        <p class="section-kicker">Validate</p>
        <h1 class="section-title">导出验包</h1>
        <p class="muted">这里检查导出合同、预算和预览一致性，确认工作室规则已经真正编译进 `theme.json`。</p>
      </div>
      <div class="chip-row">
        <span class="chip">{{ bundle.validation.ok ? '合同通过' : '合同阻断' }}</span>
        <span class="chip">{{ bundle.readability.ok ? '文字稳定' : '可读性预警' }}</span>
        <span class="chip">本地 ZIP 发布</span>
      </div>
    </header>

    <section class="metric-grid">
      <article class="metric-card panel compact-panel">
        <span class="tiny">素材数</span>
        <strong>{{ bundle.studioSummary.materialCount }}</strong>
      </article>
      <article class="metric-card panel compact-panel">
        <span class="tiny">场景对象</span>
        <strong>{{ bundle.studioSummary.objectCount }}</strong>
      </article>
      <article class="metric-card panel compact-panel">
        <span class="tiny">背景序列</span>
        <strong>{{ bundle.studioSummary.backgroundCount }}</strong>
      </article>
      <article class="metric-card panel compact-panel">
        <span class="tiny">导出帧数</span>
        <strong>{{ bundle.validation.stats.frameCount }}</strong>
      </article>
      <article class="metric-card panel compact-panel">
        <span class="tiny">估算字节</span>
        <strong>{{ bundle.validation.stats.estimatedBinaryBytes }}</strong>
      </article>
      <article class="metric-card panel compact-panel">
        <span class="tiny">预演触发</span>
        <strong>{{ bundle.studioSummary.minuteTriggerCount + bundle.studioSummary.hourTriggerCount }}</strong>
      </article>
    </section>

    <section class="info-strip panel compact-panel">
      <strong>当前导出语义</strong>
      <p class="muted">`theme.json` 只写入基础动画包；时间组件位置和分钟 / 整点触发保留在工作室预览与作品设计层，不混入导出合同字段。</p>
    </section>

    <section class="validate-grid">
      <article class="panel validate-card compact-panel">
        <p class="section-kicker">Contract</p>
        <h3>合同校验</h3>
        <ul class="warning-list" v-if="bundle.validation.errors.length">
          <li v-for="error in bundle.validation.errors" :key="`${error.path}-${error.code}`">
            {{ error.message }}（{{ error.path }}）
          </li>
        </ul>
        <ul class="warning-list" v-else>
          <li>字段、首帧类型、像素数量与计数全部通过。</li>
        </ul>
      </article>

      <article class="panel validate-card compact-panel">
        <p class="section-kicker">Budget</p>
        <h3>预算建议</h3>
        <ul class="warning-list">
          <li>工作室估算循环时长：{{ Math.round(bundle.studioSummary.loopDurationMs / 100) / 10 }} 秒</li>
          <li>工作室估算采样步长：{{ bundle.studioSummary.sampleStepMs }} 毫秒</li>
          <li v-for="hint in bundle.validation.hints" :key="hint.title">{{ hint.message }}</li>
        </ul>
      </article>

      <article class="panel validate-card compact-panel">
        <p class="section-kicker">Readability</p>
        <h3>时间组件检查</h3>
        <ul class="warning-list" v-if="bundle.readability.warnings.length">
          <li v-for="warning in bundle.readability.warnings" :key="warning.code">{{ warning.message }}</li>
        </ul>
        <ul class="warning-list" v-else>
          <li>当前时间、日期、星期、年份位置没有明显越界或重叠。</li>
        </ul>
      </article>

      <article class="panel validate-card compact-panel">
        <p class="section-kicker">Artifacts</p>
        <h3>导出产物预览</h3>
        <div class="artifact-grid">
          <img v-if="bundle.urls.coverUrl" :src="bundle.urls.coverUrl" alt="封面图" />
          <img v-if="bundle.urls.previewUrl" :src="bundle.urls.previewUrl" alt="预览动图" />
        </div>
        <div class="artifact-actions">
          <button class="secondary" @click="publishStore.downloadBlob(bundle.files.themeJsonBlob, 'theme.json')">下载 JSON</button>
          <button class="secondary" @click="publishStore.downloadBlob(bundle.files.validationBlob, 'validation.json')">下载报告</button>
          <button v-if="bundle.files.coverPngBlob" class="secondary" @click="publishStore.downloadBlob(bundle.files.coverPngBlob, 'cover.png')">下载封面</button>
          <button v-if="bundle.files.previewGifBlob" class="secondary" @click="publishStore.downloadBlob(bundle.files.previewGifBlob, 'preview.gif')">下载 GIF</button>
        </div>
      </article>
    </section>

    <footer class="validate-actions">
      <button class="secondary" @click="router.push(`/studio/${project.id}`)">返回工作室</button>
      <button @click="router.push(`/publish/${project.id}`)">进入发布中心</button>
    </footer>
  </section>
</template>

<style scoped>
.validate-view {
  display: grid;
  gap: 14px;
}

.compact-panel {
  padding: 16px;
}

.page-head,
.validate-actions,
.info-strip {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: center;
}

.info-strip {
  align-items: flex-start;
}

.info-strip strong {
  min-width: 88px;
}

.validate-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.validate-card {
  display: grid;
  gap: 10px;
}

.validate-card h3 {
  margin: 0;
}

.artifact-grid {
  display: grid;
  gap: 12px;
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.artifact-grid img {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 10px;
  background: rgba(0, 0, 0, 0.26);
  object-fit: contain;
  image-rendering: pixelated;
}

.artifact-actions,
.validate-actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

@media (max-width: 980px) {
  .page-head,
  .validate-grid,
  .info-strip {
    grid-template-columns: 1fr;
  }

  .page-head,
  .info-strip {
    flex-direction: column;
    align-items: flex-start;
  }

  .artifact-grid {
    grid-template-columns: 1fr;
  }
}
</style>

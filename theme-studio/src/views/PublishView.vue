<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useProjectStore } from '../stores/projectStore'
import { usePublishStore } from '../stores/publishStore'

const route = useRoute()
const router = useRouter()
const projectStore = useProjectStore()
const publishStore = usePublishStore()

const project = computed(() => projectStore.activeProject)
const bundle = computed(() => publishStore.activeBundle)
const records = computed(() => {
  if (!project.value) {
    return []
  }
  return publishStore.records.filter((record) => record.projectId === project.value.id)
})

const versionText = ref('')
const descriptionText = ref('')
const tagsText = ref('')
const statusText = ref('')

function buildZipName() {
  const safeName = project.value.name.replace(/\s+/g, '-')
  return `${safeName}-${versionText.value}.zip`
}

function hydrateDraft() {
  if (!project.value) {
    return
  }

  versionText.value = project.value.publishDraft.version
  descriptionText.value = project.value.publishDraft.description
  tagsText.value = project.value.publishDraft.tags.join(', ')
}

async function loadProject(projectId) {
  const loaded = await projectStore.loadProject(projectId)
  if (!loaded) {
    router.push('/')
    return
  }

  hydrateDraft()
  await publishStore.ensureBundle(loaded)
}

async function syncDraft() {
  await projectStore.updatePublishDraft({
    version: versionText.value,
    description: descriptionText.value,
    tags: tagsText.value
      .split(',')
      .map((tag) => tag.trim())
      .filter((tag) => tag.length > 0),
  })
}

async function publishZip() {
  await syncDraft()
  const result = await publishStore.publishProject(projectStore.activeProject)
  publishStore.downloadBlob(result.zipBlob, buildZipName())
  statusText.value = `已生成本地发布包，记录时间 ${new Date(result.record.publishedAt).toLocaleString('zh-CN')}`
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
  if (publishStore.records.length === 0) {
    await publishStore.boot()
  }
})
</script>

<template>
  <section v-if="project && bundle" class="publish-view">
    <header class="panel page-head compact-panel">
      <div>
        <p class="section-kicker">Publish</p>
        <h1 class="section-title">本地发布中心</h1>
        <p class="muted">这里管理作品封面、说明、版本和 ZIP 打包。简介、标签和版本只写入本地发布记录，不写进 `theme.json`。</p>
      </div>
      <div class="chip-row">
        <span class="chip">theme.json</span>
        <span class="chip">cover.png</span>
        <span class="chip">preview.gif</span>
        <span class="chip">validation.json</span>
      </div>
    </header>

    <section class="publish-grid">
      <article class="panel publish-form compact-panel">
        <p class="section-kicker">Release</p>
        <div class="summary-strip">
          <span>{{ bundle.studioSummary.materialCount }} 素材</span>
          <span>{{ bundle.studioSummary.objectCount }} 对象</span>
          <span>{{ bundle.validation.stats.frameCount }} 帧导出</span>
          <span>{{ bundle.validation.stats.estimatedBinaryBytes }} 字节</span>
        </div>

        <label>
          <span class="tiny">版本号</span>
          <input v-model="versionText" @change="syncDraft" />
        </label>
        <label>
          <span class="tiny">主题简介</span>
          <textarea v-model="descriptionText" @change="syncDraft"></textarea>
        </label>
        <label>
          <span class="tiny">标签（逗号分隔）</span>
          <input v-model="tagsText" @change="syncDraft" />
        </label>

        <div class="publish-actions">
          <button class="secondary" @click="router.push(`/validate/${project.id}`)">返回验包</button>
          <button @click="publishZip">下载发布 ZIP</button>
        </div>

        <p v-if="statusText" class="muted">{{ statusText }}</p>
      </article>

      <article class="panel publish-preview compact-panel">
        <p class="section-kicker">Preview</p>
        <div class="preview-grid">
          <img v-if="bundle.urls.coverUrl" :src="bundle.urls.coverUrl" alt="封面图" />
          <img v-if="bundle.urls.previewUrl" :src="bundle.urls.previewUrl" alt="预览动图" />
        </div>
        <div class="artifact-list">
          <div class="artifact-row">
            <strong>theme.json</strong>
            <span>基础动画主题包</span>
          </div>
          <div class="artifact-row">
            <strong>cover.png</strong>
            <span>作品封面</span>
          </div>
          <div class="artifact-row">
            <strong>preview.gif</strong>
            <span>工作室预览动图</span>
          </div>
          <div class="artifact-row">
            <strong>validation.json</strong>
            <span>导出校验报告</span>
          </div>
        </div>
        <div class="publish-actions">
          <button class="secondary" @click="publishStore.downloadBlob(bundle.files.themeJsonBlob, 'theme.json')">下载 JSON</button>
          <button class="secondary" @click="publishStore.downloadBlob(bundle.files.validationBlob, 'validation.json')">下载报告</button>
        </div>
      </article>
    </section>

    <section class="panel publish-records compact-panel">
      <div class="records-head">
        <div>
          <p class="section-kicker">History</p>
          <h3>本地发布记录</h3>
        </div>
      </div>

      <div v-if="records.length" class="records-list">
        <article v-for="record in records" :key="record.id" class="record-card">
          <strong>{{ record.version }}</strong>
          <p class="muted">{{ record.description || '暂无简介' }}</p>
          <div class="chip-row">
            <span v-for="tag in record.tags" :key="tag" class="chip">{{ tag }}</span>
          </div>
          <span class="tiny">{{ new Date(record.publishedAt).toLocaleString('zh-CN') }}</span>
        </article>
      </div>
      <p v-else class="muted">还没有发布记录，下载一次 ZIP 后会自动留下本地记录。</p>
    </section>
  </section>
</template>

<style scoped>
.publish-view {
  display: grid;
  gap: 14px;
}

.compact-panel {
  padding: 16px;
}

.page-head {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: center;
}

.publish-grid {
  display: grid;
  grid-template-columns: minmax(0, 0.9fr) minmax(0, 1.1fr);
  gap: 14px;
}

.publish-form,
.publish-preview,
.publish-records {
  display: grid;
  gap: 12px;
}

.summary-strip,
.publish-actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.summary-strip span {
  display: inline-flex;
  align-items: center;
  min-height: 24px;
  padding: 0 8px;
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.06);
  color: var(--text-soft);
  font-size: 12px;
}

.preview-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.preview-grid img {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 10px;
  background: rgba(0, 0, 0, 0.26);
  object-fit: contain;
  image-rendering: pixelated;
}

.artifact-list,
.records-list {
  display: grid;
  gap: 10px;
}

.artifact-row,
.record-card {
  display: grid;
  gap: 4px;
  padding: 10px;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.05);
}

@media (max-width: 980px) {
  .page-head,
  .publish-grid,
  .preview-grid {
    grid-template-columns: 1fr;
  }

  .page-head {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>

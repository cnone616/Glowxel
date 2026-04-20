<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { FONT_OPTIONS, TIME_PRESET_OPTIONS } from '../lib/studioSchema'
import { parsePreviewTime } from '../lib/studioProject'
import { parseBackgroundDescription, parseObjectDescription } from '../lib/studio/descriptionRules'
import { buildMaterialAssetFromFile, getAcceptedImageMimeTypes } from '../lib/studio/materialAssets'
import { useProjectStore } from '../stores/projectStore'
import { buildStudioSummary } from '../lib/studio-compiler'

const route = useRoute()
const router = useRouter()
const projectStore = useProjectStore()

const MATERIAL_CATEGORY_OPTIONS = [
  { key: 'background', label: '背景' },
  { key: 'object', label: '对象' },
  { key: 'decor', label: '装饰' },
]
const BACKGROUND_BEHAVIOR_OPTIONS = [
  { key: 'static', label: '固定' },
  { key: 'rotate', label: '轮播' },
  { key: 'blink', label: '闪烁' },
]
const OBJECT_MOTION_OPTIONS = [
  { key: 'static', label: '静止' },
  { key: 'blink', label: '闪烁' },
  { key: 'path', label: '路径' },
]
const PATH_MODE_OPTIONS = [
  { key: 'loop', label: '循环' },
  { key: 'pingpong', label: '往返' },
  { key: 'once', label: '单次' },
]
const TRIGGER_ACTION_OPTIONS = [
  { key: 'background_rotate', label: '背景轮播' },
  { key: 'background_blink', label: '背景闪烁' },
  { key: 'object_blink', label: '对象闪烁' },
  { key: 'path_play', label: '路径播放' },
  { key: 'decor_show', label: '对象显现' },
]
const PREVIEW_MODE_OPTIONS = [
  { key: 'loop', label: '常规预览' },
  { key: 'minute', label: '分钟触发' },
  { key: 'hour', label: '整点触发' },
]
const INSPECTOR_TABS = [
  { key: 'background', label: '背景' },
  { key: 'object', label: '对象' },
  { key: 'time', label: '时间' },
  { key: 'trigger', label: '触发' },
]
const WIDGET_ORDER = ['time', 'date', 'week', 'year']
const WIDGET_LABELS = {
  time: '时间',
  date: '日期',
  week: '星期',
  year: '年份',
}
const WIDGET_SIZE_OPTIONS = [
  { value: 1, label: '小' },
  { value: 2, label: '中' },
  { value: 3, label: '大' },
]
const ACCEPT_STRING = getAcceptedImageMimeTypes().join(',')

const project = computed(() => projectStore.activeProject)
const studio = computed(() => project.value?.studio ?? null)
const materials = computed(() => studio.value?.materials ?? [])
const sceneObjects = computed(() => studio.value?.scene.objects ?? [])
const backgroundRule = computed(() => studio.value?.scene.background ?? null)
const backgroundSequence = computed(() => {
  if (!studio.value) {
    return []
  }
  return studio.value.scene.background.assetIds
    .map((assetId) => materials.value.find((asset) => asset.id === assetId))
    .filter(Boolean)
})
const minuteActions = computed(() => studio.value?.triggers.minute ?? [])
const hourActions = computed(() => studio.value?.triggers.hour ?? [])
const widgetEntries = computed(() => {
  if (!studio.value) {
    return []
  }
  return WIDGET_ORDER.map((key) => ({ key, widget: studio.value.timeWidgets[key] }))
})
const previewMoment = computed(() => {
  if (!project.value) {
    return new Date('2026-01-01T12:34:00')
  }
  return parsePreviewTime(project.value)
})
const previewDateValue = computed(() => project.value?.preview.currentDate ?? '2026-01-01')
const filteredMaterials = computed(() => materials.value.filter((asset) => asset.category === selectedCategory.value))
const selectedAsset = computed(() => materials.value.find((asset) => asset.id === selectedAssetId.value) || null)
const selectedObject = computed(() => sceneObjects.value.find((item) => item.id === selectedObjectId.value) || null)
const compileSummary = computed(() => {
  if (!project.value) {
    return null
  }
  return buildStudioSummary(project.value)
})

const selectedCategory = ref('background')
const selectedAssetId = ref('')
const selectedObjectId = ref('')
const stageMode = ref('move')
const previewMode = ref('loop')
const inspectorTab = ref('background')
const stageRef = ref(null)
const feedbackMessage = ref('先上传素材，再把素材加入背景或对象。')
const sceneClock = ref(0)
let sceneTimerId = 0
let sceneStartAt = 0

function resetSceneClock() {
  sceneStartAt = Date.now()
  sceneClock.value = 0
}

function note(message) {
  feedbackMessage.value = message
}

function assetById(assetId) {
  return materials.value.find((asset) => asset.id === assetId) || null
}

function formatColor(color) {
  const channels = [color.r, color.g, color.b].map((channel) => channel.toString(16).padStart(2, '0'))
  return `#${channels.join('')}`
}

function hexToRgb(hex) {
  return {
    r: Number.parseInt(hex.slice(1, 3), 16),
    g: Number.parseInt(hex.slice(3, 5), 16),
    b: Number.parseInt(hex.slice(5, 7), 16),
  }
}

function fontFamilyFor(fontId) {
  if (fontId === 1) {
    return 'var(--font-display)'
  }
  if (fontId === 2) {
    return 'var(--font-mono)'
  }
  return 'var(--font-body)'
}

function widgetFontSize(size) {
  if (size === 1) {
    return 8
  }
  if (size === 3) {
    return 13
  }
  return 10
}

function formatWidgetText(key) {
  const date = previewMoment.value
  if (key === 'time') {
    const use12Hour = project.value.clock.hourFormat === 12
    let hours = date.getHours()
    let suffix = ''
    if (use12Hour) {
      suffix = hours >= 12 ? ' PM' : ' AM'
      hours = hours % 12 || 12
    }
    const minutes = String(date.getMinutes()).padStart(2, '0')
    const seconds = String(date.getSeconds()).padStart(2, '0')
    const hourText = String(hours).padStart(2, '0')
    const secondText = project.value.clock.showSeconds ? `:${seconds}` : ''
    return `${hourText}:${minutes}${secondText}${suffix}`
  }

  if (key === 'date') {
    return `${String(date.getMonth() + 1).padStart(2, '0')}/${String(date.getDate()).padStart(2, '0')}`
  }

  if (key === 'week') {
    return date.toLocaleDateString('zh-CN', { weekday: 'short' })
  }

  return `${date.getFullYear()}`
}

function widgetStyle(widget) {
  return {
    left: `calc(${widget.x} * var(--pixel-unit))`,
    top: `calc(${widget.y} * var(--pixel-unit))`,
    color: formatColor(widget.color),
    fontSize: `calc(${widgetFontSize(widget.size)} * var(--pixel-unit))`,
    fontFamily: fontFamilyFor(widget.font),
  }
}

const activeTriggerActions = computed(() => {
  if (previewMode.value === 'minute') {
    return minuteActions.value
  }
  if (previewMode.value === 'hour') {
    return hourActions.value
  }
  return []
})

const triggerWindowMs = computed(() => {
  if (previewMode.value === 'loop') {
    return 0
  }
  const maxDuration = activeTriggerActions.value.reduce((max, action) => Math.max(max, action.durationMs), 0)
  return Math.max(2400, maxDuration || 2400)
})

const sceneElapsedMs = computed(() => {
  if (previewMode.value === 'loop') {
    return sceneClock.value
  }
  if (triggerWindowMs.value === 0) {
    return 0
  }
  return sceneClock.value % triggerWindowMs.value
})

function buildBaseBackgroundState(elapsedMs) {
  if (!backgroundRule.value || backgroundSequence.value.length === 0) {
    return {
      asset: null,
      opacity: 1,
      assetElapsedMs: elapsedMs,
    }
  }

  if (backgroundRule.value.behavior === 'rotate' && backgroundSequence.value.length > 1) {
    const duration = Math.max(500, backgroundRule.value.intervalMs)
    const index = Math.floor(elapsedMs / duration) % backgroundSequence.value.length
    return {
      asset: backgroundSequence.value[index],
      opacity: 1,
      assetElapsedMs: elapsedMs % duration,
    }
  }

  if (backgroundRule.value.behavior === 'blink') {
    const duration = Math.max(320, backgroundRule.value.intervalMs)
    const phase = Math.floor(elapsedMs / duration) % 2
    return {
      asset: backgroundSequence.value[0],
      opacity: phase === 0 ? 1 : 0.28,
      assetElapsedMs: elapsedMs,
    }
  }

  return {
    asset: backgroundSequence.value[0],
    opacity: 1,
    assetElapsedMs: elapsedMs,
  }
}

function buildPreviewBackgroundState() {
  const state = buildBaseBackgroundState(sceneElapsedMs.value)

  for (const action of activeTriggerActions.value) {
    if (action.type === 'background_rotate' && backgroundSequence.value.length > 1) {
      const slotDuration = Math.max(160, Math.floor(Math.max(600, action.durationMs) / backgroundSequence.value.length))
      const index = Math.floor(sceneElapsedMs.value / slotDuration) % backgroundSequence.value.length
      state.asset = backgroundSequence.value[index]
      state.assetElapsedMs = sceneElapsedMs.value % slotDuration
    }

    if (action.type === 'background_blink') {
      const blinkDuration = Math.max(140, Math.floor(Math.max(600, action.durationMs) / 6))
      const phase = Math.floor(sceneElapsedMs.value / blinkDuration) % 2
      state.opacity *= phase === 0 ? 1 : 0.18
    }
  }

  return state
}

const backgroundPreviewState = computed(() => buildPreviewBackgroundState())

function distance(first, second) {
  return Math.hypot(second.x - first.x, second.y - first.y)
}

function samplePathPoint(points, progress) {
  if (points.length === 0) {
    return { x: 32, y: 32 }
  }

  if (points.length === 1) {
    return { x: points[0].x, y: points[0].y }
  }

  const segments = []
  let totalLength = 0
  for (let index = 0; index < points.length - 1; index += 1) {
    const start = points[index]
    const end = points[index + 1]
    const length = distance(start, end)
    totalLength += length
    segments.push({ start, end, length })
  }

  if (totalLength === 0) {
    return { x: points[0].x, y: points[0].y }
  }

  let targetLength = totalLength * progress
  for (const segment of segments) {
    if (targetLength <= segment.length) {
      const ratio = segment.length === 0 ? 0 : targetLength / segment.length
      return {
        x: segment.start.x + (segment.end.x - segment.start.x) * ratio,
        y: segment.start.y + (segment.end.y - segment.start.y) * ratio,
      }
    }
    targetLength -= segment.length
  }

  const lastPoint = points[points.length - 1]
  return { x: lastPoint.x, y: lastPoint.y }
}

function applyObjectTriggerState(state, object) {
  if (previewMode.value === 'loop') {
    return state
  }

  for (const action of activeTriggerActions.value) {
    if (!action.targetObjectId || action.targetObjectId !== object.id) {
      continue
    }

    if (action.type === 'object_blink') {
      const blinkDuration = Math.max(140, Math.floor(Math.max(600, action.durationMs) / 6))
      const phase = Math.floor(sceneElapsedMs.value / blinkDuration) % 2
      state.opacity = phase === 0 ? 1 : 0.1
    }

    if (action.type === 'path_play' && object.pathPoints.length > 0) {
      const progress = Math.min(1, sceneElapsedMs.value / Math.max(600, action.durationMs))
      const point = samplePathPoint(object.pathPoints, progress)
      state.x = point.x
      state.y = point.y
    }

    if (action.type === 'decor_show') {
      state.opacity = sceneElapsedMs.value <= Math.max(600, action.durationMs) ? 1 : state.opacity
    }
  }

  return state
}

function getObjectRuntimeState(object) {
  const elapsedMs = previewMode.value === 'loop' ? sceneElapsedMs.value : sceneElapsedMs.value
  const state = {
    x: object.x,
    y: object.y,
    opacity: object.visible ? 1 : 0,
  }

  if (!object.visible) {
    return state
  }

  if (object.motionType === 'blink') {
    const duration = Math.max(200, object.blinkIntervalMs)
    const phase = Math.floor(elapsedMs / duration) % 2
    state.opacity = phase === 0 ? 1 : 0.18
  }

  if (object.motionType === 'path' && object.pathPoints.length > 0) {
    const rawTime = Math.max(0, elapsedMs - object.delayMs)
    const duration = Math.max(1000, object.durationMs)
    let progress = 0

    if (object.pathMode === 'once') {
      progress = Math.min(1, rawTime / duration)
    } else if (object.pathMode === 'pingpong') {
      const total = rawTime % (duration * 2)
      progress = total <= duration ? total / duration : 1 - (total - duration) / duration
    } else {
      progress = (rawTime % duration) / duration
    }

    const point = samplePathPoint(object.pathPoints, progress)
    state.x = point.x
    state.y = point.y
  }

  return applyObjectTriggerState(state, object)
}

function buildObjectStyle(object) {
  const asset = assetById(object.assetId)
  if (!asset) {
    return {}
  }

  const runtime = getObjectRuntimeState(object)
  const width = Math.max(8, object.size)
  const height = Math.max(8, (width * asset.height) / Math.max(1, asset.width))

  return {
    left: `calc(${runtime.x} * var(--pixel-unit))`,
    top: `calc(${runtime.y} * var(--pixel-unit))`,
    width: `calc(${width} * var(--pixel-unit))`,
    height: `calc(${height} * var(--pixel-unit))`,
    opacity: runtime.opacity,
  }
}

function buildPathPoints(object) {
  if (!object || object.pathPoints.length === 0) {
    return ''
  }
  return object.pathPoints.map((point) => `${point.x},${point.y}`).join(' ')
}

function getStagePoint(event) {
  const rect = stageRef.value.getBoundingClientRect()
  const x = Math.max(0, Math.min(63, Math.round(((event.clientX - rect.left) / rect.width) * 63)))
  const y = Math.max(0, Math.min(63, Math.round(((event.clientY - rect.top) / rect.height) * 63)))
  return { x, y }
}

async function loadProject(projectId) {
  const loaded = await projectStore.loadProject(projectId)
  if (!loaded) {
    router.push('/')
    return
  }

  resetSceneClock()
  selectedAssetId.value = loaded.studio.materials[0]?.id || ''
  selectedObjectId.value = loaded.studio.scene.objects[0]?.id || ''
  inspectorTab.value = loaded.studio.scene.objects.length ? 'object' : 'background'
  note('工作室已载入。先上传素材，再配置背景和对象动作。')
}

async function handleMaterialUpload(event) {
  const files = Array.from(event.target.files || [])
  if (files.length === 0) {
    return
  }

  try {
    const builtAssets = []
    for (const file of files) {
      builtAssets.push(await buildMaterialAssetFromFile(file, selectedCategory.value))
    }
    await projectStore.appendStudioAssets(builtAssets)
    selectedAssetId.value = builtAssets[0]?.id || selectedAssetId.value
    inspectorTab.value = selectedCategory.value === 'background' ? 'background' : 'object'
    note(`已导入 ${builtAssets.length} 个${MATERIAL_CATEGORY_OPTIONS.find((item) => item.key === selectedCategory.value)?.label}素材。`)
  } catch (error) {
    note(error instanceof Error ? error.message : '素材导入失败。')
  }

  event.target.value = ''
}

async function applySelectedAsset() {
  if (!selectedAsset.value) {
    return
  }

  if (selectedAsset.value.category === 'background') {
    await projectStore.addAssetToBackgroundSequence(selectedAsset.value.id)
    inspectorTab.value = 'background'
    note(`已把 ${selectedAsset.value.name} 加入背景序列。`)
    return
  }

  const createdId = await projectStore.addStudioObjectFromAsset(selectedAsset.value.id)
  if (createdId) {
    selectedObjectId.value = createdId
    inspectorTab.value = 'object'
    note(`已把 ${selectedAsset.value.name} 加入场景对象。`)
  }
}

async function removeSelectedAsset() {
  if (!selectedAsset.value) {
    return
  }

  const removedId = selectedAsset.value.id
  await projectStore.removeStudioAsset(removedId)
  if (selectedAssetId.value === removedId) {
    selectedAssetId.value = filteredMaterials.value.find((asset) => asset.id !== removedId)?.id || ''
  }
  if (selectedObject.value && selectedObject.value.assetId === removedId) {
    selectedObjectId.value = sceneObjects.value[0]?.id || ''
  }
  note('素材已删除，并已同步清理场景引用。')
}

async function removeBackgroundAsset(assetId) {
  await projectStore.removeAssetFromBackgroundSequence(assetId)
  note('已从背景序列移除素材。')
}

async function applyBackgroundDescriptionRule() {
  if (!backgroundRule.value) {
    return
  }
  const patch = parseBackgroundDescription(backgroundRule.value.description, backgroundRule.value)
  await projectStore.updateStudioBackgroundRule(patch)
  note('已根据背景描述更新规则。')
}

async function applyObjectDescriptionRule() {
  if (!selectedObject.value) {
    return
  }
  const patch = parseObjectDescription(selectedObject.value.pathDescription, selectedObject.value)
  await projectStore.updateStudioObject(selectedObject.value.id, patch)
  note('已根据对象描述更新动作规则。')
}

async function updateBackgroundField(field, value) {
  await projectStore.updateStudioBackgroundRule({ [field]: value })
}

async function updateSelectedObjectField(field, value) {
  if (!selectedObject.value) {
    return
  }
  await projectStore.updateStudioObject(selectedObject.value.id, { [field]: value })
}

async function removeSelectedObject() {
  if (!selectedObject.value) {
    return
  }
  const removingId = selectedObject.value.id
  await projectStore.removeStudioObject(removingId)
  selectedObjectId.value = sceneObjects.value.find((item) => item.id !== removingId)?.id || ''
  note('已移除当前场景对象。')
}

async function handleStageClick(event) {
  if (!selectedObject.value || !stageRef.value) {
    note('先选中一个对象，再在场景里摆放或录入路径点。')
    return
  }

  const point = getStagePoint(event)
  if (stageMode.value === 'path') {
    await projectStore.addStudioPathPoint(selectedObject.value.id, point)
    note(`已为 ${selectedObject.value.name} 添加路径点 ${point.x}, ${point.y}。`)
    return
  }

  await projectStore.updateStudioObject(selectedObject.value.id, { x: point.x, y: point.y })
  note(`已把 ${selectedObject.value.name} 放到 ${point.x}, ${point.y}。`)
}

async function clearSelectedPath() {
  if (!selectedObject.value) {
    return
  }
  await projectStore.clearStudioPath(selectedObject.value.id)
  note('已清空当前对象路径。')
}

async function updateWidget(widgetKey, field, value) {
  await projectStore.updateStudioWidget(widgetKey, { [field]: value })
}

async function addTriggerAction(triggerKey) {
  await projectStore.addStudioTriggerAction(triggerKey)
  inspectorTab.value = 'trigger'
  note(triggerKey === 'hour' ? '已新增整点动作。' : '已新增分钟动作。')
}

async function updateTriggerAction(triggerKey, actionId, field, value) {
  await projectStore.updateStudioTriggerAction(triggerKey, actionId, { [field]: value })
}

async function removeTriggerAction(triggerKey, actionId) {
  await projectStore.removeStudioTriggerAction(triggerKey, actionId)
  note(triggerKey === 'hour' ? '已移除整点动作。' : '已移除分钟动作。')
}

async function updateStudioNotes(value) {
  await projectStore.updateStudioNotes(value)
}

async function saveProject() {
  await projectStore.persistActiveProject()
  note('工作室项目已保存。')
}

function goValidate() {
  router.push(`/validate/${project.value.id}`)
}

function goPublish() {
  router.push(`/publish/${project.value.id}`)
}

function setPreviewPreset(value) {
  projectStore.updatePreviewTime(value)
}

function selectObject(objectId) {
  selectedObjectId.value = objectId
  inspectorTab.value = 'object'
}

function previewModeLabel(key) {
  return PREVIEW_MODE_OPTIONS.find((option) => option.key === key)?.label || key
}

const pathPointText = computed(() => {
  if (!selectedObject.value || selectedObject.value.pathPoints.length === 0) {
    return []
  }
  return selectedObject.value.pathPoints.map((point) => `${point.x}, ${point.y}`)
})

watch(
  () => route.params.id,
  async (nextId) => {
    if (typeof nextId === 'string') {
      await loadProject(nextId)
    }
  },
  { immediate: true },
)

watch(filteredMaterials, (nextAssets) => {
  if (!nextAssets.some((asset) => asset.id === selectedAssetId.value)) {
    selectedAssetId.value = nextAssets[0]?.id || ''
  }
})

watch(sceneObjects, (nextObjects) => {
  if (!nextObjects.some((object) => object.id === selectedObjectId.value)) {
    selectedObjectId.value = nextObjects[0]?.id || ''
  }
})

watch(previewMode, () => {
  resetSceneClock()
})

onMounted(async () => {
  if (!projectStore.initialized) {
    await projectStore.boot()
  }
  resetSceneClock()
  sceneTimerId = window.setInterval(() => {
    sceneClock.value = Date.now() - sceneStartAt
  }, 120)
})

onBeforeUnmount(() => {
  if (sceneTimerId) {
    window.clearInterval(sceneTimerId)
    sceneTimerId = 0
  }
})
</script>

<template>
  <section v-if="project && studio" class="studio-view">
    <header class="studio-header panel">
      <div class="header-main">
        <p class="section-kicker">Theme Studio</p>
        <input class="project-name" :value="project.name" @change="projectStore.setProjectName($event.target.value)" />
        <div class="summary-strip">
          <span>{{ materials.length }} 素材</span>
          <span>{{ sceneObjects.length }} 对象</span>
          <span>{{ backgroundSequence.length }} 背景</span>
          <span v-if="compileSummary">预估 {{ compileSummary.estimatedFrameCount }} 帧</span>
          <span>{{ previewModeLabel(previewMode) }}</span>
        </div>
      </div>
      <div class="header-actions">
        <button class="secondary" @click="saveProject">保存</button>
        <button class="secondary" @click="goValidate">验包</button>
        <button @click="goPublish">发布</button>
      </div>
    </header>

    <div class="studio-grid">
      <aside class="studio-panel panel left-panel">
        <section class="panel-section compact-section">
          <div class="section-headline">
            <strong>素材上传</strong>
            <span>PNG / JPG / WebP / GIF</span>
          </div>
          <div class="segmented compact-tabs">
            <button
              v-for="option in MATERIAL_CATEGORY_OPTIONS"
              :key="option.key"
              class="secondary slim-button"
              :class="{ active: selectedCategory === option.key }"
              @click="selectedCategory = option.key"
            >
              {{ option.label }}
            </button>
          </div>
          <label class="upload-box">
            <input class="hidden-input" type="file" multiple :accept="ACCEPT_STRING" @change="handleMaterialUpload" />
            <strong>上传到当前分类</strong>
            <small>导入后可直接加入背景序列，或放进场景作为移动对象。</small>
          </label>
        </section>

        <section class="panel-section list-section">
          <div class="section-headline">
            <strong>素材库</strong>
            <span>{{ filteredMaterials.length }} 项</span>
          </div>
          <div v-if="filteredMaterials.length" class="asset-list">
            <button
              v-for="asset in filteredMaterials"
              :key="asset.id"
              class="asset-row"
              :class="{ active: selectedAssetId === asset.id }"
              @click="selectedAssetId = asset.id"
            >
              <img :src="asset.sourceDataUrl" :alt="asset.name" />
              <span>
                <strong>{{ asset.name }}</strong>
                <small>{{ asset.width }}x{{ asset.height }} · {{ asset.mimeType.split('/')[1] }}</small>
              </span>
            </button>
          </div>
          <p v-else class="muted inline-note">当前分类还没有素材。</p>
        </section>

        <section v-if="selectedAsset" class="panel-section compact-section">
          <div class="section-headline">
            <strong>当前素材</strong>
            <span>{{ selectedAsset.category }}</span>
          </div>
          <div class="selected-asset-card">
            <img class="asset-preview" :src="selectedAsset.sourceDataUrl" :alt="selectedAsset.name" />
            <div class="selected-asset-copy">
              <strong>{{ selectedAsset.name }}</strong>
              <small>{{ selectedAsset.width }}x{{ selectedAsset.height }} · {{ selectedAsset.mimeType }}</small>
            </div>
          </div>
          <div class="action-row">
            <button @click="applySelectedAsset">
              {{ selectedAsset.category === 'background' ? '加入背景' : '加入对象' }}
            </button>
            <button class="secondary" @click="removeSelectedAsset">删除素材</button>
          </div>
        </section>

        <section class="panel-section list-section">
          <div class="section-headline">
            <strong>场景对象</strong>
            <span>{{ sceneObjects.length }} 项</span>
          </div>
          <div v-if="sceneObjects.length" class="object-list">
            <button
              v-for="object in sceneObjects"
              :key="object.id"
              class="object-row"
              :class="{ active: selectedObjectId === object.id }"
              @click="selectObject(object.id)"
            >
              <span>
                <strong>{{ object.name }}</strong>
                <small>{{ object.motionType }} · {{ object.pathPoints.length }} 点</small>
              </span>
              <em>{{ object.pathMode }}</em>
            </button>
          </div>
          <p v-else class="muted inline-note">把对象素材加入场景后，这里会列出可编辑对象。</p>
        </section>
      </aside>

      <section class="studio-panel panel center-panel">
        <div class="center-toolbar">
          <div class="toolbar-group">
            <span class="toolbar-label">预览模式</span>
            <div class="segmented">
              <button
                v-for="option in PREVIEW_MODE_OPTIONS"
                :key="option.key"
                class="secondary slim-button"
                :class="{ active: previewMode === option.key }"
                @click="previewMode = option.key"
              >
                {{ option.label }}
              </button>
            </div>
          </div>

          <div class="toolbar-group">
            <span class="toolbar-label">场景点击</span>
            <div class="segmented">
              <button class="secondary slim-button" :class="{ active: stageMode === 'move' }" @click="stageMode = 'move'">摆放对象</button>
              <button class="secondary slim-button" :class="{ active: stageMode === 'path' }" @click="stageMode = 'path'">录路径点</button>
            </div>
          </div>

          <label class="time-select">
            <span class="toolbar-label">预览时刻</span>
            <select :value="project.preview.currentTime" @change="setPreviewPreset($event.target.value)">
              <option v-for="preset in TIME_PRESET_OPTIONS" :key="preset.key" :value="preset.value">{{ preset.label }}</option>
            </select>
          </label>
        </div>

        <div class="stage-shell">
          <div class="stage-frame">
            <div ref="stageRef" class="scene-stage" @click="handleStageClick">
              <img
                v-if="backgroundPreviewState.asset"
                class="scene-background"
                :src="backgroundPreviewState.asset.sourceDataUrl"
                :alt="backgroundPreviewState.asset.name"
                :style="{ opacity: backgroundPreviewState.opacity }"
              />

              <svg class="scene-overlay" viewBox="0 0 64 64" preserveAspectRatio="none">
                <g v-for="object in sceneObjects" :key="`${object.id}-path`">
                  <polyline
                    v-if="object.pathPoints.length > 1"
                    :points="buildPathPoints(object)"
                    :class="['path-line', { selected: selectedObjectId === object.id }]"
                  />
                  <circle
                    v-for="(point, pointIndex) in object.pathPoints"
                    :key="`${object.id}-${pointIndex}`"
                    :cx="point.x"
                    :cy="point.y"
                    r="1.1"
                    :class="['path-node', { selected: selectedObjectId === object.id }]"
                  />
                </g>
              </svg>

              <img
                v-for="object in sceneObjects"
                :key="object.id"
                class="scene-object"
                :class="{ selected: selectedObjectId === object.id }"
                :src="assetById(object.assetId)?.sourceDataUrl"
                :alt="object.name"
                :style="buildObjectStyle(object)"
                @click.stop="selectObject(object.id)"
              />

              <span
                v-for="entry in widgetEntries"
                :key="entry.key"
                v-show="entry.widget.visible"
                class="scene-widget"
                :style="widgetStyle(entry.widget)"
              >
                {{ formatWidgetText(entry.key) }}
              </span>
            </div>
          </div>
        </div>

        <div class="scene-footer">
          <div class="feedback-line">{{ feedbackMessage }}</div>
          <div class="scene-stats">
            <span v-if="compileSummary">循环 {{ Math.round(compileSummary.loopDurationMs / 100) / 10 }}s</span>
            <span v-if="compileSummary">采样 {{ compileSummary.sampleStepMs }}ms</span>
            <span>{{ previewDateValue }} {{ project.preview.currentTime }}</span>
            <span v-if="previewMode !== 'loop'">{{ activeTriggerActions.length }} 条触发动作</span>
          </div>
        </div>
      </section>

      <aside class="studio-panel panel right-panel">
        <div class="inspector-tabs segmented">
          <button
            v-for="tab in INSPECTOR_TABS"
            :key="tab.key"
            class="secondary slim-button"
            :class="{ active: inspectorTab === tab.key }"
            @click="inspectorTab = tab.key"
          >
            {{ tab.label }}
          </button>
        </div>

        <section v-if="inspectorTab === 'background'" class="panel-section inspector-section">
          <div class="section-headline">
            <strong>背景规则</strong>
            <span>{{ backgroundSequence.length }} 张</span>
          </div>
          <div v-if="backgroundSequence.length" class="sequence-list">
            <div v-for="asset in backgroundSequence" :key="asset.id" class="sequence-row">
              <div class="sequence-copy">
                <strong>{{ asset.name }}</strong>
                <small>{{ asset.mimeType.split('/')[1] }}</small>
              </div>
              <button class="secondary slim-button" @click="removeBackgroundAsset(asset.id)">移除</button>
            </div>
          </div>
          <p v-else class="muted inline-note">从左侧把背景素材加入序列后，这里控制轮播和闪烁。</p>

          <div class="field-grid" v-if="backgroundRule">
            <label>
              <span>背景动作</span>
              <select :value="backgroundRule.behavior" @change="updateBackgroundField('behavior', $event.target.value)">
                <option v-for="option in BACKGROUND_BEHAVIOR_OPTIONS" :key="option.key" :value="option.key">{{ option.label }}</option>
              </select>
            </label>
            <label>
              <span>节奏毫秒</span>
              <input type="number" min="300" step="100" :value="backgroundRule.intervalMs" @change="updateBackgroundField('intervalMs', Number($event.target.value))" />
            </label>
          </div>

          <label v-if="backgroundRule">
            <span>背景描述</span>
            <textarea :value="backgroundRule.description" @change="updateBackgroundField('description', $event.target.value)"></textarea>
          </label>
          <button class="secondary" v-if="backgroundRule" @click="applyBackgroundDescriptionRule">根据描述套用规则</button>
        </section>

        <section v-if="inspectorTab === 'object'" class="panel-section inspector-section">
          <div class="section-headline">
            <strong>对象规则</strong>
            <span>{{ selectedObject ? selectedObject.name : '未选择对象' }}</span>
          </div>

          <template v-if="selectedObject">
            <div class="quick-metrics">
              <span>{{ selectedObject.motionType }}</span>
              <span>{{ selectedObject.pathMode }}</span>
              <span>{{ selectedObject.pathPoints.length }} 点</span>
            </div>

            <div class="field-grid">
              <label>
                <span>动作</span>
                <select :value="selectedObject.motionType" @change="updateSelectedObjectField('motionType', $event.target.value)">
                  <option v-for="option in OBJECT_MOTION_OPTIONS" :key="option.key" :value="option.key">{{ option.label }}</option>
                </select>
              </label>
              <label>
                <span>路径模式</span>
                <select :value="selectedObject.pathMode" @change="updateSelectedObjectField('pathMode', $event.target.value)">
                  <option v-for="option in PATH_MODE_OPTIONS" :key="option.key" :value="option.key">{{ option.label }}</option>
                </select>
              </label>
            </div>

            <div class="field-grid compact-fields">
              <label>
                <span>X</span>
                <input type="number" min="0" max="63" :value="selectedObject.x" @change="updateSelectedObjectField('x', Number($event.target.value))" />
              </label>
              <label>
                <span>Y</span>
                <input type="number" min="0" max="63" :value="selectedObject.y" @change="updateSelectedObjectField('y', Number($event.target.value))" />
              </label>
              <label>
                <span>大小</span>
                <input type="number" min="8" max="48" :value="selectedObject.size" @change="updateSelectedObjectField('size', Number($event.target.value))" />
              </label>
            </div>

            <div class="field-grid compact-fields">
              <label>
                <span>起始延时</span>
                <input type="number" min="0" step="100" :value="selectedObject.delayMs" @change="updateSelectedObjectField('delayMs', Number($event.target.value))" />
              </label>
              <label>
                <span>持续毫秒</span>
                <input type="number" min="300" step="100" :value="selectedObject.durationMs" @change="updateSelectedObjectField('durationMs', Number($event.target.value))" />
              </label>
              <label>
                <span>闪烁毫秒</span>
                <input type="number" min="200" step="100" :value="selectedObject.blinkIntervalMs" @change="updateSelectedObjectField('blinkIntervalMs', Number($event.target.value))" />
              </label>
            </div>

            <label>
              <span>动作描述</span>
              <textarea :value="selectedObject.pathDescription" @change="updateSelectedObjectField('pathDescription', $event.target.value)"></textarea>
            </label>

            <div v-if="pathPointText.length" class="path-chip-list">
              <span v-for="point in pathPointText" :key="point" class="mini-chip">{{ point }}</span>
            </div>

            <div class="action-row wrap-row">
              <button class="secondary" @click="applyObjectDescriptionRule">按描述套用</button>
              <button class="secondary" @click="clearSelectedPath">清空路径</button>
              <button class="secondary" @click="removeSelectedObject">删除对象</button>
            </div>
          </template>

          <p v-else class="muted inline-note">先从左侧对象列表选中一个对象，或把素材加入场景。</p>
        </section>

        <section v-if="inspectorTab === 'time'" class="panel-section inspector-section">
          <div class="section-headline">
            <strong>时间组件</strong>
            <span>位置 / 字号 / 字体</span>
          </div>

          <div class="field-grid">
            <label>
              <span>预览日期</span>
              <input type="date" :value="previewDateValue" @change="projectStore.updatePreviewDate($event.target.value)" />
            </label>
            <label>
              <span>预览时刻</span>
              <select :value="project.preview.currentTime" @change="projectStore.updatePreviewTime($event.target.value)">
                <option v-for="preset in TIME_PRESET_OPTIONS" :key="preset.key" :value="preset.value">{{ preset.label }}</option>
              </select>
            </label>
          </div>

          <div class="field-grid">
            <label>
              <span>小时制</span>
              <select :value="project.clock.hourFormat" @change="projectStore.updateClockMeta({ hourFormat: Number($event.target.value) })">
                <option :value="24">24 小时制</option>
                <option :value="12">12 小时制</option>
              </select>
            </label>
            <label class="checkbox-row inline-switch">
              <input type="checkbox" :checked="project.clock.showSeconds" @change="projectStore.updateClockMeta({ showSeconds: $event.target.checked })" />
              <span>显示秒</span>
            </label>
          </div>

          <div v-for="entry in widgetEntries" :key="entry.key" class="widget-card">
            <div class="widget-card-head">
              <strong>{{ WIDGET_LABELS[entry.key] }}</strong>
              <label class="checkbox-row">
                <input type="checkbox" :checked="entry.widget.visible" @change="updateWidget(entry.key, 'visible', $event.target.checked)" />
                <span>显示</span>
              </label>
            </div>
            <div class="field-grid compact-fields">
              <label>
                <span>X</span>
                <input type="number" min="0" max="63" :value="entry.widget.x" @change="updateWidget(entry.key, 'x', Number($event.target.value))" />
              </label>
              <label>
                <span>Y</span>
                <input type="number" min="0" max="63" :value="entry.widget.y" @change="updateWidget(entry.key, 'y', Number($event.target.value))" />
              </label>
              <label>
                <span>大小</span>
                <select :value="entry.widget.size" @change="updateWidget(entry.key, 'size', Number($event.target.value))">
                  <option v-for="option in WIDGET_SIZE_OPTIONS" :key="option.value" :value="option.value">{{ option.label }}</option>
                </select>
              </label>
            </div>
            <div class="field-grid">
              <label>
                <span>字体</span>
                <select :value="entry.widget.font" @change="updateWidget(entry.key, 'font', Number($event.target.value))">
                  <option v-for="font in FONT_OPTIONS" :key="font.id" :value="font.id">{{ font.label }}</option>
                </select>
              </label>
              <label>
                <span>颜色</span>
                <input type="color" :value="formatColor(entry.widget.color)" @input="updateWidget(entry.key, 'color', hexToRgb($event.target.value))" />
              </label>
            </div>
          </div>
        </section>

        <section v-if="inspectorTab === 'trigger'" class="panel-section inspector-section">
          <div class="section-headline">
            <strong>触发动作</strong>
            <span>分钟 / 整点预演</span>
          </div>
          <p class="muted inline-note">这些动作会在工作室预览里模拟分钟或整点变化，但不会写进导出的 `theme.json`。</p>

          <div class="trigger-group">
            <div class="subhead-row">
              <strong>分钟触发</strong>
              <button class="secondary slim-button" @click="addTriggerAction('minute')">新增</button>
            </div>
            <div v-if="minuteActions.length" class="trigger-list">
              <div v-for="action in minuteActions" :key="action.id" class="trigger-card">
                <div class="field-grid">
                  <label>
                    <span>动作类型</span>
                    <select :value="action.type" @change="updateTriggerAction('minute', action.id, 'type', $event.target.value)">
                      <option v-for="option in TRIGGER_ACTION_OPTIONS" :key="option.key" :value="option.key">{{ option.label }}</option>
                    </select>
                  </label>
                  <label>
                    <span>目标对象</span>
                    <select :value="action.targetObjectId" @change="updateTriggerAction('minute', action.id, 'targetObjectId', $event.target.value)">
                      <option value="">无</option>
                      <option v-for="object in sceneObjects" :key="object.id" :value="object.id">{{ object.name }}</option>
                    </select>
                  </label>
                </div>
                <div class="field-grid compact-fields">
                  <label>
                    <span>持续毫秒</span>
                    <input type="number" min="200" step="100" :value="action.durationMs" @change="updateTriggerAction('minute', action.id, 'durationMs', Number($event.target.value))" />
                  </label>
                  <label>
                    <span>标签</span>
                    <input :value="action.label" @change="updateTriggerAction('minute', action.id, 'label', $event.target.value)" />
                  </label>
                </div>
                <label>
                  <span>动作说明</span>
                  <textarea :value="action.description" @change="updateTriggerAction('minute', action.id, 'description', $event.target.value)"></textarea>
                </label>
                <button class="secondary slim-button" @click="removeTriggerAction('minute', action.id)">删除</button>
              </div>
            </div>
          </div>

          <div class="trigger-group">
            <div class="subhead-row">
              <strong>整点触发</strong>
              <button class="secondary slim-button" @click="addTriggerAction('hour')">新增</button>
            </div>
            <div v-if="hourActions.length" class="trigger-list">
              <div v-for="action in hourActions" :key="action.id" class="trigger-card">
                <div class="field-grid">
                  <label>
                    <span>动作类型</span>
                    <select :value="action.type" @change="updateTriggerAction('hour', action.id, 'type', $event.target.value)">
                      <option v-for="option in TRIGGER_ACTION_OPTIONS" :key="option.key" :value="option.key">{{ option.label }}</option>
                    </select>
                  </label>
                  <label>
                    <span>目标对象</span>
                    <select :value="action.targetObjectId" @change="updateTriggerAction('hour', action.id, 'targetObjectId', $event.target.value)">
                      <option value="">无</option>
                      <option v-for="object in sceneObjects" :key="object.id" :value="object.id">{{ object.name }}</option>
                    </select>
                  </label>
                </div>
                <div class="field-grid compact-fields">
                  <label>
                    <span>持续毫秒</span>
                    <input type="number" min="200" step="100" :value="action.durationMs" @change="updateTriggerAction('hour', action.id, 'durationMs', Number($event.target.value))" />
                  </label>
                  <label>
                    <span>标签</span>
                    <input :value="action.label" @change="updateTriggerAction('hour', action.id, 'label', $event.target.value)" />
                  </label>
                </div>
                <label>
                  <span>动作说明</span>
                  <textarea :value="action.description" @change="updateTriggerAction('hour', action.id, 'description', $event.target.value)"></textarea>
                </label>
                <button class="secondary slim-button" @click="removeTriggerAction('hour', action.id)">删除</button>
              </div>
            </div>
          </div>

          <label>
            <span>项目备注</span>
            <textarea :value="studio.notes" @change="updateStudioNotes($event.target.value)"></textarea>
          </label>
        </section>
      </aside>
    </div>
  </section>
</template>

<style scoped>
.studio-view {
  --stage-scale: 7;
  --stage-size: calc(64px * var(--stage-scale));
  --pixel-unit: calc(var(--stage-scale) * 1px);
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 10px;
  min-height: calc(100svh - 126px);
}

.studio-header,
.studio-panel {
  background: rgba(8, 11, 14, 0.96);
  border-color: rgba(102, 234, 214, 0.14);
  box-shadow: none;
  border-radius: 8px;
}

.studio-header {
  padding: 10px 12px;
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.header-main {
  display: grid;
  gap: 6px;
  min-width: 0;
}

.project-name {
  width: min(520px, 100%);
  padding: 8px 10px;
  border-radius: 7px;
  font-size: 20px;
  font-weight: 700;
}

.summary-strip,
.header-actions,
.scene-stats,
.action-row,
.quick-metrics {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.summary-strip span,
.scene-stats span,
.quick-metrics span {
  display: inline-flex;
  align-items: center;
  min-height: 24px;
  padding: 0 8px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.06);
  color: var(--text-soft);
  font-size: 12px;
}

.studio-grid {
  min-height: 0;
  display: grid;
  grid-template-columns: 252px minmax(0, 1fr) 336px;
  gap: 10px;
}

.studio-panel {
  min-height: 0;
  padding: 10px;
  display: grid;
  gap: 10px;
  align-content: start;
}

.left-panel,
.right-panel {
  grid-template-rows: auto minmax(0, 1fr);
  overflow: auto;
}

.center-panel {
  overflow: hidden;
  grid-template-rows: auto minmax(0, 1fr) auto;
}

.panel-section {
  display: grid;
  gap: 8px;
  padding-top: 10px;
  border-top: 1px solid rgba(255, 255, 255, 0.06);
}

.panel-section:first-child {
  padding-top: 0;
  border-top: 0;
}

.section-headline,
.subhead-row,
.widget-card-head {
  display: flex;
  justify-content: space-between;
  gap: 8px;
  align-items: center;
}

.section-headline strong,
.subhead-row strong,
.widget-card-head strong {
  font-size: 13px;
}

.section-headline span,
.subhead-row span,
.inline-note,
.sequence-copy small,
.asset-row small,
.object-row small {
  color: var(--text-dim);
  font-size: 12px;
}

button {
  border-radius: 7px;
  padding: 8px 10px;
  font-size: 12px;
}

.slim-button {
  padding: 7px 10px;
}

.segmented {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  align-items: flex-start;
}

.segmented .active {
  border-color: rgba(102, 234, 214, 0.4);
  background: rgba(102, 234, 214, 0.12);
}

.compact-tabs {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.upload-box {
  display: grid;
  gap: 4px;
  padding: 10px;
  border: 1px dashed rgba(255, 255, 255, 0.14);
  background: rgba(255, 255, 255, 0.02);
  cursor: pointer;
}

.upload-box small {
  color: var(--text-dim);
}

.hidden-input {
  display: none;
}

.asset-list,
.object-list,
.sequence-list,
.trigger-list {
  display: grid;
  gap: 6px;
}

.asset-row,
.object-row {
  width: 100%;
  display: grid;
  gap: 8px;
  padding: 8px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.03);
  text-align: left;
  border-radius: 7px;
}

.asset-row {
  grid-template-columns: 44px minmax(0, 1fr);
}

.object-row {
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: center;
}

.asset-row.active,
.object-row.active {
  border-color: rgba(102, 234, 214, 0.4);
  background: rgba(102, 234, 214, 0.1);
}

.asset-row img,
.asset-preview {
  width: 100%;
  aspect-ratio: 1;
  object-fit: contain;
  image-rendering: pixelated;
  background: rgba(0, 0, 0, 0.3);
}

.asset-row span,
.object-row span,
.sequence-copy,
.selected-asset-copy {
  display: grid;
  gap: 3px;
}

.object-row em {
  color: var(--accent-amber);
  font-style: normal;
  font-size: 11px;
}

.selected-asset-card {
  display: grid;
  grid-template-columns: 72px minmax(0, 1fr);
  gap: 8px;
  align-items: center;
}

.asset-preview {
  border: 1px solid rgba(255, 255, 255, 0.08);
}

.center-toolbar {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, auto));
  gap: 10px;
  align-items: end;
}

.toolbar-group,
.time-select {
  display: grid;
  gap: 6px;
}

.toolbar-label {
  color: var(--text-dim);
  font-size: 11px;
}

.time-select select {
  min-width: 112px;
}

.stage-shell {
  min-height: 0;
  display: grid;
  place-items: center;
  overflow: auto;
}

.stage-frame {
  width: var(--stage-size);
  height: var(--stage-size);
  flex: none;
}

.scene-stage {
  position: relative;
  width: var(--stage-size);
  height: var(--stage-size);
  border: 1px solid rgba(255, 255, 255, 0.12);
  background:
    linear-gradient(rgba(255, 255, 255, 0.06) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255, 255, 255, 0.06) 1px, transparent 1px),
    #040607;
  background-size: var(--pixel-unit) var(--pixel-unit), var(--pixel-unit) var(--pixel-unit), auto;
  image-rendering: pixelated;
  overflow: hidden;
  cursor: crosshair;
}

.scene-background,
.scene-object,
.scene-overlay,
.scene-widget {
  position: absolute;
}

.scene-background,
.scene-overlay {
  inset: 0;
  width: 100%;
  height: 100%;
}

.scene-background,
.scene-object {
  image-rendering: pixelated;
  object-fit: contain;
  transition: transform 0.15s ease, opacity 0.15s ease;
}

.scene-object.selected {
  outline: 1px solid rgba(255, 191, 105, 0.7);
}

.scene-widget {
  white-space: nowrap;
  line-height: 1;
  text-shadow: 0 0 6px rgba(0, 0, 0, 0.5);
}

.path-line {
  fill: none;
  stroke: rgba(102, 234, 214, 0.36);
  stroke-width: 0.7;
}

.path-line.selected {
  stroke: rgba(255, 191, 105, 0.84);
}

.path-node {
  fill: rgba(102, 234, 214, 0.88);
}

.path-node.selected {
  fill: rgba(255, 191, 105, 0.98);
}

.scene-footer {
  display: grid;
  gap: 8px;
}

.feedback-line {
  min-height: 30px;
  display: flex;
  align-items: center;
  padding: 0 10px;
  color: var(--text-soft);
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.06);
  font-size: 12px;
}

.inspector-tabs {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 6px;
  align-self: start;
  position: sticky;
  top: 0;
  z-index: 2;
  background: rgba(8, 11, 14, 0.96);
  padding-bottom: 2px;
}

.inspector-section {
  padding-top: 0;
  border-top: 0;
}

.field-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.compact-fields {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.sequence-row,
.trigger-card,
.widget-card {
  display: grid;
  gap: 8px;
  padding: 8px;
  border: 1px solid rgba(255, 255, 255, 0.06);
  background: rgba(255, 255, 255, 0.025);
}

.sequence-row {
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: center;
}

.checkbox-row {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  width: auto;
}

.inline-switch {
  min-height: 38px;
  padding: 0 10px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.04);
}

.checkbox-row input {
  width: auto;
}

.path-chip-list {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.mini-chip {
  display: inline-flex;
  align-items: center;
  min-height: 24px;
  padding: 0 8px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.06);
  color: var(--text-soft);
  font-size: 12px;
}

.wrap-row {
  flex-wrap: wrap;
}

.trigger-group {
  display: grid;
  gap: 8px;
}

label span {
  display: block;
  margin-bottom: 4px;
  color: var(--text-dim);
  font-size: 12px;
}

textarea {
  min-height: 84px;
}

@media (max-width: 1180px) {
  .studio-view {
    --stage-scale: 6;
    min-height: auto;
  }

  .studio-grid {
    grid-template-columns: 1fr;
  }

  .left-panel,
  .right-panel,
  .center-panel {
    overflow: visible;
  }

  .center-toolbar {
    grid-template-columns: 1fr;
  }

  .scene-stage {
    width: var(--stage-size);
  }
}

@media (max-width: 760px) {
  .studio-view {
    --stage-scale: 5;
  }

  .studio-header {
    align-items: flex-start;
    flex-direction: column;
  }

  .compact-tabs,
  .field-grid,
  .compact-fields,
  .selected-asset-card {
    grid-template-columns: 1fr;
  }
}
</style>

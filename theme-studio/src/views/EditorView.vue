<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import PreviewDisplay from '../components/editor/PreviewDisplay.vue'
import PixelCanvas from '../components/editor/PixelCanvas.vue'
import TimelineStrip from '../components/editor/TimelineStrip.vue'
import { BACKGROUND_PRESETS, MOTION_PRESETS } from '../data/studioCatalog'
import {
  COLOR_PALETTES,
  FONT_OPTIONS,
  LAYER_LABELS,
  LAYER_ORDER,
  PIXEL_LAYER_KEYS,
  PIXEL_TOOLS,
  TIME_PRESET_OPTIONS,
} from '../lib/studioSchema'
import { composePixelFrameMap } from '../lib/studioProject'
import { useEditorStore } from '../stores/editorStore'
import { useProjectStore } from '../stores/projectStore'
import { buildValidationReport } from '../utils/exporters'
import { FRAME_TYPE_DIFF, FRAME_TYPE_FULL } from '../utils/themeContract'

const route = useRoute()
const router = useRouter()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const TOOL_HINTS = {
  brush: '写入当前层当前帧的像素',
  eraser: '擦除当前层当前帧的像素',
  fill: '按连通区域填充同色块',
  picker: '从当前画面读取颜色',
}

const LAYER_HINTS = {
  background: '整张主题的动态底面',
  motion: '主要动态元素和路径变化',
  accent: '高亮点缀和发光细节',
  time: '时间文本的位置和颜色',
  date: '日期文本的位置和颜色',
  week: '星期文本的位置和颜色',
}

const FRAME_TYPE_LABELS = {
  [FRAME_TYPE_FULL]: '全量帧',
  [FRAME_TYPE_DIFF]: '差分帧',
}

const selectedBackgroundPresetKey = ref(BACKGROUND_PRESETS[0].key)
const selectedMotionPresetKey = ref(MOTION_PRESETS[0].key)
const tweenStepCount = ref(2)
const hoverPoint = ref(null)
const feedbackMessage = ref('先选中一层，再围绕当前帧连续编辑。')

const project = computed(() => projectStore.activeProject)
const validation = computed(() => {
  if (!project.value) {
    return null
  }
  return buildValidationReport(project.value.themePackage)
})

const selectedFrame = computed(() => {
  if (!project.value) {
    return null
  }
  return project.value.timeline.frames[editorStore.selectedFrameIndex]
})

const isPixelLayer = computed(() => PIXEL_LAYER_KEYS.includes(editorStore.selectedLayerKey))
const selectedLayerLabel = computed(() => LAYER_LABELS[editorStore.selectedLayerKey])
const selectedLayerHint = computed(() => LAYER_HINTS[editorStore.selectedLayerKey])
const activeTool = computed(() => PIXEL_TOOLS.find((tool) => tool.key === editorStore.selectedTool) || PIXEL_TOOLS[0])
const activeBackgroundPreset = computed(() => {
  return BACKGROUND_PRESETS.find((preset) => preset.key === selectedBackgroundPresetKey.value)
})
const activeMotionPreset = computed(() => {
  return MOTION_PRESETS.find((preset) => preset.key === selectedMotionPresetKey.value)
})
const selectedTextLayer = computed(() => {
  if (!project.value || isPixelLayer.value) {
    return null
  }
  return project.value.clock[editorStore.selectedLayerKey]
})
const currentColorHex = computed(() => rgbToHex(editorStore.selectedColor))
const currentFrameVisiblePixels = computed(() => {
  if (!project.value) {
    return 0
  }
  return composePixelFrameMap(project.value, editorStore.selectedFrameIndex).size
})
const currentLayerPixels = computed(() => {
  if (!project.value || !isPixelLayer.value) {
    return 0
  }
  return project.value.layers[editorStore.selectedLayerKey].frames[editorStore.selectedFrameIndex].length
})
const hoverLabel = computed(() => {
  if (!hoverPoint.value) {
    return '坐标 -'
  }
  return `坐标 ${hoverPoint.value.x}, ${hoverPoint.value.y}`
})
const introDuration = computed(() => {
  if (!project.value || !project.value.timeline.introEnabled) {
    return 0
  }

  let sum = 0
  for (let i = 0; i <= project.value.timeline.introEndIndex; i += 1) {
    sum += project.value.timeline.frames[i].delay
  }
  return sum
})
const loopDuration = computed(() => {
  if (!project.value) {
    return 0
  }

  let sum = 0
  for (let i = project.value.timeline.loopStartIndex; i < project.value.timeline.frames.length; i += 1) {
    sum += project.value.timeline.frames[i].delay
  }
  return sum
})
const canGenerateTween = computed(() => {
  if (!project.value || !isPixelLayer.value) {
    return false
  }
  return editorStore.selectedFrameIndex < project.value.timeline.frames.length - 1
})
const canCopyPreviousFrame = computed(() => {
  return isPixelLayer.value && editorStore.selectedFrameIndex > 0
})
const canRemoveFrame = computed(() => {
  if (!project.value) {
    return false
  }
  return project.value.timeline.frames.length > 1
})
const selectedFrameRole = computed(() => buildFrameRoleLabel(editorStore.selectedFrameIndex))

async function loadProject(projectId) {
  const loaded = await projectStore.loadProject(projectId)
  if (!loaded) {
    router.push('/')
    return
  }
  editorStore.resetForProject()
  feedbackMessage.value = '项目已载入，当前默认从背景层开始编辑。'
}

function rgbToHex(color) {
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

function buildFrameRoleLabel(frameIndex) {
  if (!project.value) {
    return ''
  }

  if (project.value.timeline.introEnabled && frameIndex <= project.value.timeline.introEndIndex) {
    if (frameIndex === project.value.timeline.introEndIndex) {
      return 'Intro 终点'
    }
    return 'Intro 段'
  }

  if (frameIndex === project.value.timeline.loopStartIndex) {
    return 'Loop 起点'
  }

  if (frameIndex > project.value.timeline.loopStartIndex) {
    return 'Loop 段'
  }

  return '普通帧'
}

function focusCurrentFrame() {
  editorStore.pausePreview()
  editorStore.setPreviewFrameIndex(editorStore.selectedFrameIndex)
}

function note(message) {
  feedbackMessage.value = message
}

async function handleProjectNameChange(event) {
  await projectStore.setProjectName(event.target.value)
  note('项目名称已更新并保存。')
}

function handleCanvasPaint(point) {
  focusCurrentFrame()
  projectStore.paintPixel(editorStore.selectedLayerKey, editorStore.selectedFrameIndex, point.x, point.y, editorStore.selectedColor)
  note(`正在 ${selectedLayerLabel.value} 的 F${editorStore.selectedFrameIndex + 1} 绘制像素。`)
}

function handleCanvasErase(point) {
  focusCurrentFrame()
  projectStore.erasePixel(editorStore.selectedLayerKey, editorStore.selectedFrameIndex, point.x, point.y)
  note(`正在 ${selectedLayerLabel.value} 的 F${editorStore.selectedFrameIndex + 1} 擦除像素。`)
}

function handleCanvasFill(point) {
  focusCurrentFrame()
  projectStore.fillPixel(editorStore.selectedLayerKey, editorStore.selectedFrameIndex, point.x, point.y, editorStore.selectedColor)
  note(`已在 ${selectedLayerLabel.value} 的 F${editorStore.selectedFrameIndex + 1} 填充连通区域。`)
}

function handleCanvasPick(point) {
  editorStore.setColor(projectStore.getCompositeColor(editorStore.selectedFrameIndex, point.x, point.y))
  note(`已从坐标 ${point.x}, ${point.y} 取色。`)
}

async function handleCanvasCommit() {
  await projectStore.commitPixelFrameChanges()
  note(`F${editorStore.selectedFrameIndex + 1} 的画布改动已提交到本地工程。`)
}

function handleCanvasHover(point) {
  hoverPoint.value = point
}

function handleLayerSelect(layerKey) {
  editorStore.setLayerKey(layerKey)
  focusCurrentFrame()
  note(`当前改的是 ${LAYER_LABELS[layerKey]}。${LAYER_HINTS[layerKey]}`)
}

function handleToolSelect(toolKey) {
  editorStore.setTool(toolKey)
  note(`当前工具切换为 ${PIXEL_TOOLS.find((tool) => tool.key === toolKey)?.label}。`)
}

function setPaletteColor(colorValue) {
  editorStore.setColor(hexToRgb(colorValue))
  note(`当前画笔颜色已切换为 ${colorValue.toUpperCase()}。`)
}

async function applyBackgroundPresetSelection() {
  await projectStore.applyBackgroundPreset(selectedBackgroundPresetKey.value)
  focusCurrentFrame()
  note(`背景层已替换为 ${activeBackgroundPreset.value?.name}。`)
}

async function applyMotionPresetSelection() {
  await projectStore.applyMotionPreset(selectedMotionPresetKey.value)
  focusCurrentFrame()
  note(`动效层已替换为 ${activeMotionPreset.value?.name}。`)
}

function handlePreviewFrameChange(frameIndex) {
  editorStore.setPreviewFrameIndex(frameIndex)
}

function handlePreviewScrub(frameIndex) {
  editorStore.pausePreview()
  editorStore.setFrameIndex(frameIndex)
  editorStore.setPreviewFrameIndex(frameIndex)
  note(`预览已锁定到 F${frameIndex + 1}。`)
}

function handleTimelineSelect(frameIndex) {
  editorStore.setFrameIndex(frameIndex)
  editorStore.pausePreview()
  editorStore.setPreviewFrameIndex(frameIndex)
  note(`当前选中 F${frameIndex + 1}，所有工具都将作用于这一帧。`)
}

async function handleFrameDelayChange(delay) {
  await projectStore.setFrameDelay(editorStore.selectedFrameIndex, delay)
  note(`F${editorStore.selectedFrameIndex + 1} 的延时已改为 ${delay}ms。`)
}

async function handleFrameTypeChange(type) {
  await projectStore.setFrameType(editorStore.selectedFrameIndex, type)
  note(`F${editorStore.selectedFrameIndex + 1} 已切换为 ${FRAME_TYPE_LABELS[type]}。`)
}

async function handleInsertFrame() {
  const sourceFrameNumber = editorStore.selectedFrameIndex + 1
  const nextFrameIndex = editorStore.selectedFrameIndex + 1
  await projectStore.insertFrameAfter(editorStore.selectedFrameIndex)
  editorStore.setFrameIndex(nextFrameIndex)
  editorStore.setPreviewFrameIndex(nextFrameIndex)
  note(`已在 F${sourceFrameNumber} 后插入新帧。`)
}

async function handleDuplicateFrame() {
  const sourceFrameNumber = editorStore.selectedFrameIndex + 1
  const nextFrameIndex = editorStore.selectedFrameIndex + 1
  await projectStore.duplicateFrame(editorStore.selectedFrameIndex)
  editorStore.setFrameIndex(nextFrameIndex)
  editorStore.setPreviewFrameIndex(nextFrameIndex)
  note(`已在 F${sourceFrameNumber} 后复制一帧。`)
}

async function handleCopyPreviousFrame() {
  if (!canCopyPreviousFrame.value) {
    return
  }

  await projectStore.copyPreviousFrameToCurrent(editorStore.selectedLayerKey, editorStore.selectedFrameIndex)
  focusCurrentFrame()
  note(`上一帧内容已复制到当前 ${selectedLayerLabel.value}。`)
}

async function handleClearCurrentFrame() {
  if (!isPixelLayer.value) {
    return
  }

  await projectStore.clearLayerFrame(editorStore.selectedLayerKey, editorStore.selectedFrameIndex)
  focusCurrentFrame()
  note(`当前帧的 ${selectedLayerLabel.value} 已清空。`)
}

async function handleRemoveFrame() {
  if (!canRemoveFrame.value) {
    return
  }

  const previousIndex = editorStore.selectedFrameIndex
  await projectStore.removeFrame(previousIndex)
  const nextIndex = Math.min(previousIndex, project.value.timeline.frames.length - 1)
  editorStore.setFrameIndex(nextIndex)
  editorStore.setPreviewFrameIndex(nextIndex)
  note(`已删除 F${previousIndex + 1}。`)
}

async function applyTweenFrames() {
  if (!canGenerateTween.value) {
    return
  }

  const insertedCount = Number(tweenStepCount.value)
  await projectStore.generateTweenFrames(
    editorStore.selectedLayerKey,
    editorStore.selectedFrameIndex,
    insertedCount,
  )
  note(`已在当前帧后补入 ${insertedCount} 帧过渡。`)
}

async function handleIntroToggle(enabled) {
  await projectStore.updateTimelineMeta({ introEnabled: enabled })
  note(enabled ? 'Intro 已启用。' : 'Intro 已关闭。')
}

async function markIntroEnd() {
  await projectStore.updateTimelineMeta({
    introEnabled: true,
    introEndIndex: editorStore.selectedFrameIndex,
  })
  note(`F${editorStore.selectedFrameIndex + 1} 已设为 Intro 终点。`)
}

async function markLoopStart() {
  await projectStore.updateTimelineMeta({ loopStartIndex: editorStore.selectedFrameIndex })
  note(`F${editorStore.selectedFrameIndex + 1} 已设为 Loop 起点。`)
}

async function updateSelectedTextLayer(field, value, label) {
  if (isPixelLayer.value) {
    return
  }

  await projectStore.updateTextLayer(editorStore.selectedLayerKey, { [field]: value })
  note(`${selectedLayerLabel.value} 的${label}已更新。`)
}

async function applyTimePreset(preset) {
  await projectStore.updatePreviewTime(preset.value)
  note(`预览时刻已切换到 ${preset.label}。`)
}

async function updatePreviewTime(value) {
  await projectStore.updatePreviewTime(value)
  note(`自定义预览时刻已更新为 ${value}。`)
}

async function updateClockMeta(patch, label) {
  await projectStore.updateClockMeta(patch)
  note(label)
}

async function saveLocalProject() {
  await projectStore.commitPixelFrameChanges()
  note('本地工程已保存。')
}

async function goValidate() {
  await projectStore.commitPixelFrameChanges()
  router.push(`/validate/${project.value.id}`)
}

async function goPublish() {
  await projectStore.commitPixelFrameChanges()
  router.push(`/publish/${project.value.id}`)
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
  <section v-if="project" class="editor-view">
    <header class="editor-header panel">
      <div class="editor-header-main">
        <input class="project-name" :value="project.name" @change="handleProjectNameChange" />
        <div class="header-meta">
          <span class="meta-tag">模板 {{ project.templateKey }}</span>
          <span class="meta-tag">当前 {{ selectedLayerLabel }}</span>
          <span class="meta-tag">F{{ editorStore.selectedFrameIndex + 1 }}</span>
          <span class="meta-tag">{{ selectedFrameRole }}</span>
          <span class="meta-tag" :class="{ pending: projectStore.hasPendingPixelEdits }">
            {{ projectStore.hasPendingPixelEdits ? '画布改动待提交' : '已同步' }}
          </span>
        </div>
      </div>
      <div class="header-actions">
        <button class="secondary" @click="saveLocalProject">保存</button>
        <button class="warn" @click="goValidate">验包</button>
        <button @click="goPublish">发布</button>
      </div>
    </header>

    <div class="editor-columns">
      <aside class="editor-column editor-left panel">
        <section class="tool-section">
          <div class="section-bar">
            <strong>图层</strong>
            <span>{{ selectedLayerHint }}</span>
          </div>
          <div class="stack-buttons">
            <button
              v-for="layerKey in LAYER_ORDER"
              :key="layerKey"
              class="secondary row-button"
              :class="{ active: editorStore.selectedLayerKey === layerKey }"
              @click="handleLayerSelect(layerKey)"
            >
              <strong>{{ LAYER_LABELS[layerKey] }}</strong>
              <span>{{ LAYER_HINTS[layerKey] }}</span>
            </button>
          </div>
        </section>

        <section class="tool-section" v-if="isPixelLayer">
          <div class="section-bar">
            <strong>工具</strong>
            <span>直接作用于当前帧</span>
          </div>
          <div class="stack-buttons">
            <button
              v-for="tool in PIXEL_TOOLS"
              :key="tool.key"
              class="secondary row-button dense"
              :class="{ active: editorStore.selectedTool === tool.key }"
              @click="handleToolSelect(tool.key)"
            >
              <strong>{{ tool.label }}</strong>
              <span>{{ TOOL_HINTS[tool.key] }}</span>
            </button>
          </div>
        </section>

        <section class="tool-section" v-if="isPixelLayer">
          <div class="section-bar">
            <strong>颜色</strong>
            <span>{{ currentColorHex.toUpperCase() }}</span>
          </div>
          <div class="color-field">
            <input :value="currentColorHex" type="color" @input="editorStore.setColor(hexToRgb($event.target.value))" />
            <span class="current-swatch" :style="{ background: currentColorHex }"></span>
          </div>
          <div class="palette-list">
            <div v-for="palette in COLOR_PALETTES" :key="palette.key" class="palette-row">
              <span>{{ palette.name }}</span>
              <div class="swatches">
                <button
                  v-for="color in palette.colors"
                  :key="color"
                  class="swatch"
                  :class="{ active: currentColorHex === color }"
                  :style="{ background: color }"
                  @click="setPaletteColor(color)"
                ></button>
              </div>
            </div>
          </div>
        </section>

        <section class="tool-section">
          <div class="section-bar">
            <strong>当前帧动作</strong>
            <span>{{ selectedFrameRole }}</span>
          </div>
          <div class="field-grid">
            <label>
              <span>延时</span>
              <input
                v-if="selectedFrame"
                type="number"
                min="40"
                step="10"
                :value="selectedFrame.delay"
                @change="handleFrameDelayChange(Number($event.target.value))"
              />
            </label>
            <label>
              <span>类型</span>
              <select
                v-if="selectedFrame"
                :value="selectedFrame.type"
                @change="handleFrameTypeChange(Number($event.target.value))"
              >
                <option :value="FRAME_TYPE_FULL">全量帧</option>
                <option :value="FRAME_TYPE_DIFF">差分帧</option>
              </select>
            </label>
          </div>
          <div class="field-grid" v-if="isPixelLayer">
            <label>
              <span>补帧数量</span>
              <select v-model="tweenStepCount">
                <option :value="1">1 帧</option>
                <option :value="2">2 帧</option>
                <option :value="3">3 帧</option>
              </select>
            </label>
            <label class="toggle-inline box-toggle">
              <input type="checkbox" :checked="project.timeline.introEnabled" @change="handleIntroToggle($event.target.checked)" />
              <span>启用 Intro</span>
            </label>
          </div>
          <div class="action-grid">
            <button class="secondary" @click="handleInsertFrame">插入下一帧</button>
            <button class="secondary" @click="handleDuplicateFrame">复制当前帧</button>
            <button class="secondary" :disabled="!canCopyPreviousFrame" @click="handleCopyPreviousFrame">复制上一帧</button>
            <button class="secondary" :disabled="!isPixelLayer" @click="handleClearCurrentFrame">清空当前层</button>
            <button class="secondary" :disabled="!canGenerateTween" @click="applyTweenFrames">补帧</button>
            <button class="danger" :disabled="!canRemoveFrame" @click="handleRemoveFrame">删除当前帧</button>
          </div>
          <div class="segment-actions">
            <button
              class="secondary"
              :class="{ active: project.timeline.introEnabled && project.timeline.introEndIndex === editorStore.selectedFrameIndex }"
              @click="markIntroEnd"
            >
              设为 Intro 终点
            </button>
            <button
              class="secondary"
              :class="{ active: project.timeline.loopStartIndex === editorStore.selectedFrameIndex }"
              @click="markLoopStart"
            >
              设为 Loop 起点
            </button>
          </div>
        </section>
      </aside>

      <section class="editor-column editor-center panel">
        <div class="stage-topbar">
          <div class="stage-readout">
            <strong>F{{ editorStore.selectedFrameIndex + 1 }}</strong>
            <span>{{ selectedLayerLabel }}</span>
            <span>{{ isPixelLayer ? activeTool.label : '文本定位模式' }}</span>
          </div>
          <div class="stage-readout align-right">
            <span>{{ hoverLabel }}</span>
            <span>本层 {{ currentLayerPixels }} px</span>
            <span>画面 {{ currentFrameVisiblePixels }} px</span>
            <span v-if="selectedFrame">{{ selectedFrame.delay }}ms</span>
          </div>
        </div>

        <div class="canvas-wrap">
          <PixelCanvas
            :project="project"
            :frame-index="editorStore.selectedFrameIndex"
            :editable="isPixelLayer"
            :active-tool="editorStore.selectedTool"
            :onion-skin="editorStore.onionSkin"
            @paint="handleCanvasPaint"
            @erase="handleCanvasErase"
            @fill="handleCanvasFill"
            @pick="handleCanvasPick"
            @commit="handleCanvasCommit"
            @hover="handleCanvasHover"
          />
        </div>

        <div class="stage-feedback">
          <div class="feedback-main">{{ feedbackMessage }}</div>
          <div class="feedback-meta">
            <span>播放头 F{{ editorStore.previewFrameIndex + 1 }}</span>
            <span>{{ editorStore.previewPlaying ? '预览播放中' : '预览锁定当前帧' }}</span>
            <span v-if="projectStore.hasPendingPixelEdits">等待提交</span>
          </div>
        </div>

        <section class="timeline-panel">
          <div class="timeline-header">
            <div>
              <strong>时间轴</strong>
              <span>{{ project.timeline.frames.length }} 帧 / Intro {{ introDuration }}ms / Loop {{ loopDuration }}ms</span>
            </div>
            <div class="timeline-header-actions">
              <button class="secondary" @click="editorStore.togglePreviewPlaying()">
                {{ editorStore.previewPlaying ? '暂停预览' : '播放预览' }}
              </button>
              <button class="secondary" v-if="isPixelLayer" @click="editorStore.toggleOnionSkin()">
                {{ editorStore.onionSkin ? '关闭洋葱皮' : '打开洋葱皮' }}
              </button>
            </div>
          </div>
          <TimelineStrip
            :project="project"
            :selected-frame-index="editorStore.selectedFrameIndex"
            :playhead-frame-index="editorStore.previewFrameIndex"
            @select="handleTimelineSelect"
          />
        </section>
      </section>

      <aside class="editor-column editor-right panel">
        <section class="tool-section">
          <div class="section-bar">
            <strong>输出预览</strong>
            <span>跟随时钟状态</span>
          </div>
          <PreviewDisplay
            :project="project"
            :playing="editorStore.previewPlaying"
            :selected-frame-index="editorStore.selectedFrameIndex"
            @frame-change="handlePreviewFrameChange"
            @scrub="handlePreviewScrub"
          />
        </section>

        <section class="tool-section">
          <div class="section-bar">
            <strong>时钟预览</strong>
            <span>{{ project.preview.currentTime }}</span>
          </div>
          <div class="time-presets">
            <button
              v-for="preset in TIME_PRESET_OPTIONS"
              :key="preset.key"
              class="secondary time-chip"
              :class="{ active: project.preview.currentTime === preset.value }"
              @click="applyTimePreset(preset)"
            >
              {{ preset.label }}
            </button>
          </div>
          <div class="field-grid">
            <label>
              <span>自定义时刻</span>
              <input type="time" step="1" :value="project.preview.currentTime" @change="updatePreviewTime($event.target.value)" />
            </label>
            <label>
              <span>小时制</span>
              <select :value="project.clock.hourFormat" @change="updateClockMeta({ hourFormat: Number($event.target.value) }, '小时制已更新。')">
                <option :value="24">24 小时制</option>
                <option :value="12">12 小时制</option>
              </select>
            </label>
          </div>
          <label class="toggle-inline box-toggle">
            <input type="checkbox" :checked="project.clock.showSeconds" @change="updateClockMeta({ showSeconds: $event.target.checked }, '秒显示状态已更新。')" />
            <span>显示秒</span>
          </label>
        </section>

        <section class="tool-section" v-if="!isPixelLayer && selectedTextLayer">
          <div class="section-bar">
            <strong>文本层属性</strong>
            <span>{{ selectedLayerLabel }}</span>
          </div>
          <label class="toggle-inline box-toggle">
            <input type="checkbox" :checked="selectedTextLayer.visible" @change="updateSelectedTextLayer('visible', $event.target.checked, '显示状态')" />
            <span>显示该文本层</span>
          </label>
          <div class="field-grid">
            <label>
              <span>X</span>
              <input type="number" min="0" max="63" :value="selectedTextLayer.x" @change="updateSelectedTextLayer('x', Number($event.target.value), 'X 坐标')" />
            </label>
            <label>
              <span>Y</span>
              <input type="number" min="0" max="63" :value="selectedTextLayer.y" @change="updateSelectedTextLayer('y', Number($event.target.value), 'Y 坐标')" />
            </label>
          </div>
          <div class="field-grid">
            <label>
              <span>字体</span>
              <select :value="selectedTextLayer.font" @change="updateSelectedTextLayer('font', Number($event.target.value), '字体')">
                <option v-for="font in FONT_OPTIONS" :key="font.id" :value="font.id">{{ font.label }}</option>
              </select>
            </label>
            <label>
              <span>颜色</span>
              <input type="color" :value="rgbToHex(selectedTextLayer.color)" @input="updateSelectedTextLayer('color', hexToRgb($event.target.value), '颜色')" />
            </label>
          </div>
        </section>

        <section class="tool-section" v-else>
          <div class="section-bar">
            <strong>骨架预设</strong>
            <span>整层替换</span>
          </div>
          <label>
            <span>背景骨架</span>
            <select v-model="selectedBackgroundPresetKey">
              <option v-for="preset in BACKGROUND_PRESETS" :key="preset.key" :value="preset.key">{{ preset.name }}</option>
            </select>
          </label>
          <p class="inline-note">{{ activeBackgroundPreset?.description }}</p>
          <button class="secondary" @click="applyBackgroundPresetSelection">应用到背景层</button>
          <label>
            <span>动效骨架</span>
            <select v-model="selectedMotionPresetKey">
              <option v-for="preset in MOTION_PRESETS" :key="preset.key" :value="preset.key">{{ preset.name }}</option>
            </select>
          </label>
          <p class="inline-note">{{ activeMotionPreset?.description }}</p>
          <button class="secondary" @click="applyMotionPresetSelection">应用到动效层</button>
        </section>

        <section class="tool-section" v-if="validation">
          <div class="section-bar">
            <strong>预算与校验</strong>
            <span>{{ validation.errors.length ? '存在问题' : '合同通过' }}</span>
          </div>
          <div class="budget-grid">
            <div class="budget-item">
              <span>总帧</span>
              <strong>{{ validation.stats.frameCount }}</strong>
            </div>
            <div class="budget-item">
              <span>全量 / 差分</span>
              <strong>{{ validation.stats.fullFrameCount }} / {{ validation.stats.diffFrameCount }}</strong>
            </div>
            <div class="budget-item">
              <span>总像素</span>
              <strong>{{ validation.stats.totalPixels }}</strong>
            </div>
            <div class="budget-item">
              <span>估算字节</span>
              <strong>{{ validation.stats.estimatedBinaryBytes }}</strong>
            </div>
          </div>
          <ul class="inline-list" v-if="validation.errors.length">
            <li v-for="error in validation.errors.slice(0, 4)" :key="`${error.path}-${error.code}`">
              {{ error.message }}（{{ error.path }}）
            </li>
          </ul>
          <ul class="inline-list" v-else>
            <li>合同字段完整，可继续编辑或直接导出。</li>
          </ul>
          <ul class="inline-list">
            <li v-for="hint in validation.hints.slice(0, 3)" :key="hint.title">{{ hint.message }}</li>
          </ul>
        </section>
      </aside>
    </div>
  </section>
</template>

<style scoped>
.editor-view {
  display: grid;
  gap: 10px;
}

.editor-header,
.editor-column {
  border-radius: 8px;
  box-shadow: none;
  background: rgba(11, 16, 19, 0.94);
  border-color: rgba(120, 147, 142, 0.18);
}

.editor-header {
  padding: 10px 12px;
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: center;
}

.editor-header-main {
  min-width: 0;
  display: grid;
  gap: 8px;
}

.project-name {
  width: min(520px, 100%);
  padding: 8px 10px;
  border-radius: 6px;
  font-size: 20px;
  font-weight: 700;
  background: rgba(255, 255, 255, 0.03);
}

.header-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.meta-tag {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.03);
  color: var(--text-soft);
  font-size: 12px;
}

.meta-tag.pending {
  color: var(--accent-amber);
  border-color: rgba(255, 191, 105, 0.28);
}

.header-actions {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.editor-header button,
.editor-column button {
  border-radius: 6px;
  padding: 8px 12px;
}

.editor-columns {
  display: grid;
  grid-template-columns: 272px minmax(0, 1fr) 320px;
  gap: 10px;
  align-items: start;
}

.editor-column {
  padding: 10px;
  display: grid;
  gap: 10px;
}

.tool-section,
.timeline-panel {
  display: grid;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid rgba(255, 255, 255, 0.06);
}

.tool-section:first-child,
.timeline-panel {
  padding-top: 0;
  border-top: 0;
}

.section-bar {
  display: flex;
  justify-content: space-between;
  gap: 8px;
  align-items: baseline;
}

.section-bar strong {
  font-size: 13px;
}

.section-bar span,
.inline-note {
  color: var(--text-dim);
  font-size: 12px;
}

.stack-buttons,
.palette-list,
.action-grid,
.segment-actions {
  display: grid;
  gap: 6px;
}

.row-button {
  width: 100%;
  display: grid;
  justify-items: start;
  gap: 2px;
  text-align: left;
  padding: 8px 10px;
}

.row-button strong {
  font-size: 13px;
}

.row-button span {
  color: var(--text-dim);
  font-size: 11px;
}

.row-button.active,
.time-chip.active,
.segment-actions .active,
.action-grid .active {
  border-color: rgba(102, 234, 214, 0.42);
  background: rgba(102, 234, 214, 0.1);
}

.field-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 6px;
}

.field-grid label,
.tool-section label {
  display: grid;
  gap: 4px;
  color: var(--text-soft);
  font-size: 12px;
}

.field-grid input,
.field-grid select,
.tool-section input,
.tool-section select {
  padding: 8px 9px;
  border-radius: 6px;
}

.color-field {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 26px;
  gap: 8px;
  align-items: center;
}

.color-field input[type='color'] {
  min-height: 40px;
  padding: 4px;
}

.current-swatch {
  width: 26px;
  height: 26px;
  border: 1px solid rgba(255, 255, 255, 0.14);
}

.palette-row {
  display: grid;
  gap: 4px;
}

.palette-row span {
  color: var(--text-dim);
  font-size: 11px;
}

.swatches {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.swatch {
  width: 18px;
  height: 18px;
  min-width: 18px;
  padding: 0;
  border-radius: 4px;
  box-shadow: none;
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.swatch.active {
  outline: 1px solid var(--accent-amber);
  outline-offset: 1px;
}

.action-grid {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.segment-actions {
  grid-template-columns: 1fr 1fr;
}

.toggle-inline {
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.box-toggle {
  min-height: 38px;
  padding: 0 10px;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.03);
}

.editor-center {
  grid-template-rows: auto auto auto minmax(0, 1fr);
}

.stage-topbar,
.stage-feedback,
.timeline-header {
  display: flex;
  justify-content: space-between;
  gap: 10px;
  align-items: center;
}

.stage-readout,
.feedback-meta,
.timeline-header > div,
.timeline-header-actions,
.time-presets {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  align-items: center;
}

.stage-readout span,
.feedback-meta span,
.timeline-header span {
  min-height: 28px;
  display: inline-flex;
  align-items: center;
  padding: 0 8px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.06);
  color: var(--text-soft);
  font-size: 12px;
}

.stage-readout strong {
  min-height: 28px;
  display: inline-flex;
  align-items: center;
  padding: 0 8px;
  background: rgba(255, 191, 105, 0.08);
  border: 1px solid rgba(255, 191, 105, 0.22);
  color: var(--accent-amber);
  font-size: 13px;
}

.align-right {
  justify-content: flex-end;
}

.canvas-wrap {
  display: grid;
  justify-items: center;
  padding: 6px 0;
}

.feedback-main {
  color: var(--text-strong);
  font-size: 13px;
}

.time-chip {
  min-width: 60px;
}

.budget-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 6px;
}

.budget-item {
  display: grid;
  gap: 3px;
  padding: 8px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.06);
}

.budget-item span {
  color: var(--text-dim);
  font-size: 11px;
}

.budget-item strong {
  font-size: 15px;
}

.inline-list {
  margin: 0;
  padding-left: 16px;
  color: var(--text-soft);
  font-size: 12px;
}

@media (max-width: 1320px) {
  .editor-columns {
    grid-template-columns: 248px minmax(0, 1fr) 300px;
  }
}

@media (max-width: 1120px) {
  .editor-columns {
    grid-template-columns: 1fr;
  }

  .align-right {
    justify-content: flex-start;
  }

  .editor-header {
    align-items: stretch;
    flex-direction: column;
  }
}

@media (max-width: 720px) {
  .field-grid,
  .action-grid,
  .segment-actions,
  .budget-grid {
    grid-template-columns: 1fr;
  }

  .project-name {
    width: 100%;
  }
}
</style>

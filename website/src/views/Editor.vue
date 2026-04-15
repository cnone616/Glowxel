<template>
  <div class="editor-page">
    <div class="editor-header">
      <div class="header-left">
        <button class="icon-btn" @click="$router.back()">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="15 18 9 12 15 6"/></svg>
        </button>
        <span class="project-name">{{ projectName }}</span>
        <span class="project-size">{{ width }}×{{ height }}</span>
      </div>
      <div class="header-right">
        <button class="icon-btn" @click="triggerImageUpload" title="导入图片">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
        </button>
        <input ref="fileInputRef" type="file" accept="image/*" style="display:none" @change="handleImageUpload" />
        <button class="icon-btn" @click="handleUndo" :disabled="historyIndex <= 0" title="撤销 Ctrl+Z">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="9 14 4 9 9 4"/><path d="M20 20v-7a4 4 0 0 0-4-4H4"/></svg>
        </button>
        <button class="icon-btn" @click="handleRedo" :disabled="historyIndex >= history.length - 1" title="重做">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="15 14 20 9 15 4"/><path d="M4 20v-7a4 4 0 0 1 4-4h12"/></svg>
        </button>
        <button class="icon-btn" :class="{ active: gridVisible }" @click="gridVisible = !gridVisible" title="网格">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>
        </button>
        <button class="icon-btn" @click="handleSave" title="保存">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
        </button>
        <button class="btn-publish" @click="handlePublish">发布</button>
      </div>
    </div>

    <div class="editor-body">
      <div class="tool-sidebar">
        <button v-for="t in tools" :key="t.id" class="tool-btn"
          :class="{ active: tool === t.id }" @click="tool = t.id"
          :title="t.label" v-html="t.icon">
        </button>
        <div class="tool-divider"></div>
        <div class="color-dot" :style="{ background: currentColor }"></div>
      </div>

      <div class="canvas-area" ref="canvasAreaRef">
        <canvas ref="canvasRef" class="pixel-canvas"
          @mousedown="onMouseDown" @mousemove="onMouseMove"
          @mouseup="onMouseUp" @mouseleave="onMouseUp"
          @wheel.prevent="onWheel" @contextmenu.prevent />
      </div>

      <div class="color-sidebar">
        <div class="color-sidebar-title">Artkal 颜色</div>
        <input class="color-search" v-model="colorSearch" placeholder="搜索颜色..." />
        <div class="color-grid">
          <div v-for="c in filteredColors" :key="c.code"
            class="color-swatch" :class="{ active: currentColor === c.hex }"
            :style="{ background: c.hex }" :title="`${c.code} ${c.name}`"
            @click="currentColor = c.hex" />
        </div>
      </div>
    </div>
  </div>

  <!-- 图片导入对话框 -->
  <div class="modal-overlay" v-if="showImportModal" @click.self="showImportModal = false">
    <div class="modal">
      <h3>图片像素化设置</h3>
      <div class="preview-row">
        <img :src="importPreviewUrl" class="import-preview" />
        <canvas ref="importCanvasRef" class="import-result"></canvas>
      </div>
      <div class="modal-form">
        <label>目标宽度（格）
          <input type="number" v-model.number="importWidth" min="8" max="64" @input="previewImport" />
        </label>
        <label>目标高度（格）
          <input type="number" v-model.number="importHeight" min="8" max="64" @input="previewImport" />
        </label>
      </div>
      <div class="modal-actions">
        <button class="btn-cancel" @click="showImportModal = false">取消</button>
        <button class="btn-confirm" @click="confirmImport">应用</button>
      </div>
    </div>
  </div>

</template>



<script setup>
import { ref, computed, onMounted, onUnmounted, nextTick } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ARTKAL_COLORS_FULL as ARTKAL_COLORS, findClosestColor } from '@/data/artkal-colors.js'
import { imageToPixels } from '@/utils/imageProcessor.js'
import { artworkAPI, challengeAPI, projectAPI, templateAPI } from '@/api/index.js'

const route = useRoute()
const router = useRouter()

// canvas refs
const canvasRef = ref(null)
const canvasAreaRef = ref(null)
const fileInputRef = ref(null)
const importCanvasRef = ref(null)

// project state
const projectName = ref('未命名项目')
const width = ref(32)
const height = ref(32)
const cellSize = ref(16)
const pixels = ref(new Map()) // key: "x,y" value: hex color
const gridVisible = ref(true)
const tool = ref('pen')
const currentColor = ref('#FF0000')
const colorSearch = ref('')
const isDrawing = ref(false)
const history = ref([new Map()])
const historyIndex = ref(0)

// image import
const showImportModal = ref(false)
const importPreviewUrl = ref('')
const importWidth = ref(32)
const importHeight = ref(32)
let importImage = null
const projectId = ref(route.params.id || '')
const challengeId = ref(route.query.challengeId || '')

const tools = [
  { id: 'pen', label: '画笔', icon: '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 19l7-7-3-3-7 7v3h3z"/><path d="M18 13l1.5-1.5a2.121 2.121 0 0 0-3-3L15 10"/></svg>' },
  { id: 'eraser', label: '橡皮', icon: '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 20H7L3 16l10-10 7 7-3.5 3.5"/><path d="M6.5 17.5l5-5"/></svg>' },
  { id: 'fill', label: '填充', icon: '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 11l-8-8-8.5 8.5a5.5 5.5 0 0 0 7.78 7.78L19 11z"/><path d="M20 23a2 2 0 0 0 2-2c0-1.1-2-4-2-4s-2 2.9-2 4a2 2 0 0 0 2 2z"/></svg>' },
  { id: 'eyedropper', label: '取色', icon: '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 22l1-1h3l9-9"/><path d="M3 21v-3l9-9"/><path d="M15 6l3.4-3.4a2.1 2.1 0 0 1 3 3L18 9l.4.4a2.1 2.1 0 0 1-3 3L12 9"/></svg>' },
]

const filteredColors = computed(() => {
  const q = colorSearch.value.toLowerCase()
  if (!q) return ARTKAL_COLORS
  return ARTKAL_COLORS.filter(c => c.code.toLowerCase().includes(q) || c.name.toLowerCase().includes(q))
})

// ---- canvas rendering ----
function render() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  const W = width.value * cellSize.value
  const H = height.value * cellSize.value
  canvas.width = W; canvas.height = H
  ctx.clearRect(0, 0, W, H)
  ctx.fillStyle = '#fff'
  ctx.fillRect(0, 0, W, H)
  pixels.value.forEach((color, key) => {
    const [x, y] = key.split(',').map(Number)
    ctx.fillStyle = color
    ctx.fillRect(x * cellSize.value, y * cellSize.value, cellSize.value, cellSize.value)
  })
  if (gridVisible.value) {
    ctx.strokeStyle = 'rgba(0,0,0,0.08)'
    ctx.lineWidth = 0.5
    for (let x = 0; x <= width.value; x++) {
      ctx.beginPath(); ctx.moveTo(x * cellSize.value, 0); ctx.lineTo(x * cellSize.value, H); ctx.stroke()
    }
    for (let y = 0; y <= height.value; y++) {
      ctx.beginPath(); ctx.moveTo(0, y * cellSize.value); ctx.lineTo(W, y * cellSize.value); ctx.stroke()
    }
  }
}

function getCell(e) {
  const rect = canvasRef.value.getBoundingClientRect()
  const scaleX = canvasRef.value.width / rect.width
  const scaleY = canvasRef.value.height / rect.height
  return {
    x: Math.floor((e.clientX - rect.left) * scaleX / cellSize.value),
    y: Math.floor((e.clientY - rect.top) * scaleY / cellSize.value)
  }
}

function paintCell(x, y) {
  if (x < 0 || x >= width.value || y < 0 || y >= height.value) return
  if (tool.value === 'pen') pixels.value.set(`${x},${y}`, currentColor.value)
  else if (tool.value === 'eraser') pixels.value.delete(`${x},${y}`)
  else if (tool.value === 'eyedropper') {
    const c = pixels.value.get(`${x},${y}`)
    if (c) { currentColor.value = c; tool.value = 'pen' }
    return
  } else if (tool.value === 'fill') { floodFill(x, y, currentColor.value); return }
  render()
}

function floodFill(sx, sy, newColor) {
  const target = pixels.value.get(`${sx},${sy}`) || null
  if (target === newColor) return
  const stack = [[sx, sy]]
  while (stack.length) {
    const [x, y] = stack.pop()
    if (x < 0 || x >= width.value || y < 0 || y >= height.value) continue
    const cur = pixels.value.get(`${x},${y}`) || null
    if (cur !== target) continue
    if (newColor === null) pixels.value.delete(`${x},${y}`)
    else pixels.value.set(`${x},${y}`, newColor)
    stack.push([x+1,y],[x-1,y],[x,y+1],[x,y-1])
  }
  render()
}

function pushHistory() {
  history.value = history.value.slice(0, historyIndex.value + 1)
  history.value.push(new Map(pixels.value))
  historyIndex.value = history.value.length - 1
}

function onMouseDown(e) {
  isDrawing.value = true
  pushHistory()
  const { x, y } = getCell(e)
  paintCell(x, y)
}
function onMouseMove(e) {
  if (!isDrawing.value) return
  const { x, y } = getCell(e)
  paintCell(x, y)
}
function onMouseUp() { isDrawing.value = false }
function onWheel(e) {
  cellSize.value = Math.max(8, Math.min(40, cellSize.value - Math.sign(e.deltaY) * 2))
  render()
}
function handleUndo() {
  if (historyIndex.value <= 0) return
  historyIndex.value--
  pixels.value = new Map(history.value[historyIndex.value])
  render()
}
function handleRedo() {
  if (historyIndex.value >= history.value.length - 1) return
  historyIndex.value++
  pixels.value = new Map(history.value[historyIndex.value])
  render()
}

// ---- image import ----
function triggerImageUpload() { fileInputRef.value?.click() }

function handleImageUpload(e) {
  const file = e.target.files[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = (ev) => {
    importPreviewUrl.value = ev.target.result
    importImage = new Image()
    importImage.onload = () => { showImportModal.value = true; nextTick(previewImport) }
    importImage.src = ev.target.result
  }
  reader.readAsDataURL(file)
  e.target.value = ''
}

function previewImport() {
  if (!importImage || !importCanvasRef.value) return
  const canvas = importCanvasRef.value
  canvas.width = importWidth.value
  canvas.height = importHeight.value
  const ctx = canvas.getContext('2d')
  ctx.imageSmoothingEnabled = false
  ctx.drawImage(importImage, 0, 0, importWidth.value, importHeight.value)
}

function confirmImport() {
  if (!importImage) return
  const colorCodes = ARTKAL_COLORS.map(c => c.code)
  const pixelMap = imageToPixels(importImage, importWidth.value, importHeight.value, colorCodes)
  width.value = importWidth.value
  height.value = importHeight.value
  pixels.value = new Map()
  pixelMap.forEach((code, key) => {
    const color = ARTKAL_COLORS.find(c => c.code === code)
    if (color) pixels.value.set(key, color.hex)
  })
  pushHistory()
  showImportModal.value = false
  render()
}

// ---- save / publish ----
async function handleSave() {
  const canvas = canvasRef.value
  const thumbnail = canvas.toDataURL('image/png')
  const pixelData = Object.fromEntries(pixels.value)
  if (!projectId.value) {
    projectId.value = globalThis.crypto?.randomUUID?.() || `web-${Date.now()}`
  }
  const project = {
    id: projectId.value,
    name: projectName.value,
    width: width.value,
    height: height.value,
    thumbnail,
    progress: 0,
    status: 'draft',
    palette: Array.from(new Set(pixels.value.values()))
  }
  const res = await projectAPI.sync(project, pixelData)
  if (res.success) {
    localStorage.setItem('editor_draft', JSON.stringify({ ...project, pixelData, challengeId: challengeId.value }))
    alert('已保存到云端草稿')
  } else {
    alert(res.message || '保存失败')
  }
}

async function handlePublish() {
  if (!localStorage.getItem('auth_token')) { router.push('/login'); return }
  if (!projectId.value) {
    await handleSave()
  }
  const canvas = canvasRef.value
  const thumbnail = canvas.toDataURL('image/png')
  const pixelData = Object.fromEntries(pixels.value)
  const res = await artworkAPI.publish({
    projectId: projectId.value || null,
    title: projectName.value, width: width.value, height: height.value,
    pixelData, thumbnail, colorCount: new Set(pixels.value.values()).size
  })
  if (res.success) {
    if (challengeId.value && res.data?.artworkId) {
      const submitRes = await challengeAPI.submit(challengeId.value, res.data.artworkId)
      if (!submitRes.success) {
        alert(submitRes.message || '作品已发布，但挑战投稿失败')
      }
    }
    localStorage.removeItem('editor_draft')
    alert('发布成功！')
    router.push('/community')
  }
  else alert(res.message || '发布失败')
}

// keyboard shortcuts
function onKeyDown(e) {
  if ((e.ctrlKey || e.metaKey) && e.key === 'z') { e.preventDefault(); handleUndo() }
  if ((e.ctrlKey || e.metaKey) && e.key === 'y') { e.preventDefault(); handleRedo() }
  if ((e.ctrlKey || e.metaKey) && e.key === 's') { e.preventDefault(); handleSave() }
}

onMounted(() => {
  // load draft or project detail
  const loadEditorState = async () => {
    challengeId.value = route.query.challengeId || ''
    let loaded = false

    if (route.params.id) {
      const res = await projectAPI.getDetail(route.params.id)
      if (res.success) {
        const project = res.data?.project || {}
        projectId.value = project.id || route.params.id
        projectName.value = project.name || '未命名项目'
        width.value = project.width || 32
        height.value = project.height || 32
        if (res.data?.pixels) pixels.value = new Map(Object.entries(res.data.pixels))
        loaded = true
      } else {
        return
      }
    }

    if (!loaded && route.query.templateId) {
      const res = await templateAPI.getDetail(route.query.templateId)
      if (res.success) {
        const template = res.data?.template || {}
        const size = `${template.size || ''}`.match(/^(\d+)x(\d+)$/i)
        projectName.value = template.name || '未命名模板'
        if (size) {
          width.value = parseInt(size[1], 10) || width.value
          height.value = parseInt(size[2], 10) || height.value
        }
        if (res.data?.pixels) {
          pixels.value = new Map(Object.entries(res.data.pixels))
        }
        loaded = true
      }
    }

    if (!loaded) {
      const draft = localStorage.getItem('editor_draft')
      if (draft) {
        try {
          const d = JSON.parse(draft)
          projectId.value = d.id || ''
          projectName.value = d.title || d.name || '未命名项目'
          width.value = d.width || 32
          height.value = d.height || 32
          if (d.pixelData) pixels.value = new Map(Object.entries(d.pixelData))
          if (d.challengeId) challengeId.value = d.challengeId
        } catch {}
      }
    }
    nextTick(render)
    if (route.query.mode === 'image') {
      showImportModal.value = true
    }
  }

  window.addEventListener('keydown', onKeyDown)
  loadEditorState()
})
onUnmounted(() => window.removeEventListener('keydown', onKeyDown))
</script>

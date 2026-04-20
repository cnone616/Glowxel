<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { composePixelFrameMap } from '../../lib/studioProject'

const props = defineProps({
  project: {
    type: Object,
    required: true,
  },
  frameIndex: {
    type: Number,
    required: true,
  },
  editable: {
    type: Boolean,
    required: true,
  },
  activeTool: {
    type: String,
    required: true,
  },
  onionSkin: {
    type: Boolean,
    required: true,
  },
})

const emit = defineEmits(['paint', 'erase', 'fill', 'pick', 'commit', 'hover'])

const canvasRef = ref(null)
const CELL_SIZE = 9
const painting = ref(false)
const lastKey = ref('')
const hoveredPoint = ref(null)
const recentPoint = ref(null)
let recentPointTimer = 0

const displaySize = computed(() => 64 * CELL_SIZE)

function getPoint(event) {
  const canvas = canvasRef.value
  const rect = canvas.getBoundingClientRect()
  const x = Math.max(0, Math.min(63, Math.floor(((event.clientX - rect.left) / rect.width) * 64)))
  const y = Math.max(0, Math.min(63, Math.floor(((event.clientY - rect.top) / rect.height) * 64)))
  return { x, y, key: `${x}:${y}` }
}

function drawMap(ctx, map, alpha) {
  ctx.save()
  ctx.globalAlpha = alpha
  map.forEach((color, key) => {
    const split = key.split(':').map((value) => Number(value))
    ctx.fillStyle = `rgb(${color[0]}, ${color[1]}, ${color[2]})`
    ctx.fillRect(split[0] * CELL_SIZE, split[1] * CELL_SIZE, CELL_SIZE, CELL_SIZE)
  })
  ctx.restore()
}

function drawGrid(ctx) {
  ctx.lineWidth = 1
  for (let i = 0; i <= 64; i += 1) {
    const point = i * CELL_SIZE + 0.5
    const isMajor = i % 8 === 0
    ctx.strokeStyle = isMajor ? 'rgba(255, 255, 255, 0.16)' : 'rgba(255, 255, 255, 0.06)'

    ctx.beginPath()
    ctx.moveTo(point, 0)
    ctx.lineTo(point, displaySize.value)
    ctx.stroke()

    ctx.beginPath()
    ctx.moveTo(0, point)
    ctx.lineTo(displaySize.value, point)
    ctx.stroke()
  }
}

function drawPointFrame(ctx, point, strokeStyle) {
  if (!point) {
    return
  }

  ctx.save()
  ctx.strokeStyle = strokeStyle
  ctx.lineWidth = 2
  ctx.strokeRect(point.x * CELL_SIZE + 1, point.y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2)
  ctx.restore()
}

function renderCanvas() {
  const canvas = canvasRef.value
  if (!canvas) {
    return
  }

  const ctx = canvas.getContext('2d')
  ctx.clearRect(0, 0, displaySize.value, displaySize.value)
  ctx.fillStyle = '#040607'
  ctx.fillRect(0, 0, displaySize.value, displaySize.value)

  if (props.onionSkin && props.frameIndex > 0) {
    const previousMap = composePixelFrameMap(props.project, props.frameIndex - 1)
    drawMap(ctx, previousMap, 0.15)
  }

  const currentMap = composePixelFrameMap(props.project, props.frameIndex)
  drawMap(ctx, currentMap, 1)
  drawGrid(ctx)
  drawPointFrame(ctx, recentPoint.value, 'rgba(255, 191, 105, 0.92)')
  drawPointFrame(ctx, hoveredPoint.value, 'rgba(102, 234, 214, 0.92)')
}

function setRecentPoint(point) {
  recentPoint.value = { x: point.x, y: point.y }
  if (recentPointTimer) {
    window.clearTimeout(recentPointTimer)
  }
  recentPointTimer = window.setTimeout(() => {
    recentPoint.value = null
    renderCanvas()
  }, 220)
}

function applyTool(point) {
  if (props.activeTool === 'picker') {
    emit('pick', point)
    return
  }

  setRecentPoint(point)

  if (props.activeTool === 'fill') {
    emit('fill', point)
    emit('commit')
    return
  }

  if (props.activeTool === 'brush') {
    emit('paint', point)
    return
  }

  emit('erase', point)
}

function updateHover(point) {
  hoveredPoint.value = point
  emit('hover', point)
  renderCanvas()
}

function handlePointerDown(event) {
  if (!props.editable) {
    return
  }

  const point = getPoint(event)
  lastKey.value = point.key
  updateHover(point)
  applyTool(point)

  if (props.activeTool === 'brush' || props.activeTool === 'eraser') {
    painting.value = true
  }
}

function handleCanvasPointerMove(event) {
  const point = getPoint(event)
  updateHover(point)

  if (!painting.value) {
    return
  }

  if (point.key === lastKey.value) {
    return
  }

  lastKey.value = point.key
  applyTool(point)
}

function handleWindowPointerMove(event) {
  if (!painting.value) {
    return
  }

  const point = getPoint(event)
  if (point.key === lastKey.value) {
    return
  }

  lastKey.value = point.key
  applyTool(point)
}

function handlePointerLeave() {
  hoveredPoint.value = null
  emit('hover', null)
  renderCanvas()
}

function stopPaint() {
  if (!painting.value) {
    return
  }

  painting.value = false
  lastKey.value = ''
  emit('commit')
}

watch(
  () => [props.project, props.frameIndex, props.onionSkin],
  () => {
    renderCanvas()
  },
  { deep: true, immediate: true },
)

onMounted(() => {
  window.addEventListener('pointermove', handleWindowPointerMove)
  window.addEventListener('pointerup', stopPaint)
  window.addEventListener('pointercancel', stopPaint)
  renderCanvas()
})

onBeforeUnmount(() => {
  window.removeEventListener('pointermove', handleWindowPointerMove)
  window.removeEventListener('pointerup', stopPaint)
  window.removeEventListener('pointercancel', stopPaint)
  if (recentPointTimer) {
    window.clearTimeout(recentPointTimer)
  }
})
</script>

<template>
  <div class="canvas-stage">
    <canvas
      ref="canvasRef"
      class="pixel-canvas"
      :width="displaySize"
      :height="displaySize"
      @pointerdown.prevent="handlePointerDown"
      @pointermove="handleCanvasPointerMove"
      @pointerleave="handlePointerLeave"
    ></canvas>
    <div class="canvas-caption">
      <span>{{ editable ? '当前画布直接编辑当前帧。' : '当前是文本层，画布只读。' }}</span>
      <span v-if="editable && onionSkin">洋葱皮已开启</span>
    </div>
  </div>
</template>

<style scoped>
.canvas-stage {
  display: grid;
  gap: 6px;
  justify-items: center;
}

.pixel-canvas {
  width: min(100%, 640px);
  aspect-ratio: 1;
  border-radius: 2px;
  border: 1px solid rgba(255, 255, 255, 0.12);
  background: #040607;
  box-shadow: none;
  image-rendering: pixelated;
  touch-action: none;
}

.canvas-caption {
  width: 100%;
  display: flex;
  justify-content: space-between;
  gap: 8px;
  color: var(--text-dim);
  font-size: 12px;
}

@media (max-width: 720px) {
  .canvas-caption {
    flex-direction: column;
  }
}
</style>

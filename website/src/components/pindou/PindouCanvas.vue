<template>
  <div class="pindou-canvas-container" ref="containerRef">
    <canvas
      ref="canvasRef"
      class="pindou-canvas"
      :data-tool="selectedTool"
      @mousedown="handleMouseDown"
      @mousemove="handleMouseMove"
      @mouseup="handleMouseUp"
      @mouseleave="handleMouseUp"
      @wheel="handleWheel"
      @touchstart="handleTouchStart"
      @touchmove="handleTouchMove"
      @touchend="handleTouchEnd"
    ></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { getColorByCode } from '@/data/artkal-colors'

const props = defineProps({
  width: Number,
  height: Number,
  pixels: Map,
  gridVisible: Boolean,
  showNumbers: Boolean,
  selectedTool: String,
  selectedColor: String,
  zoom: Number,
  pan: Object
})

const emit = defineEmits(['pixel-change', 'zoom-change', 'pan-change', 'color-picked'])

const containerRef = ref(null)
const canvasRef = ref(null)
const ctx = ref(null)

// 交互状态
const isDragging = ref(false)
const lastPos = ref({ x: 0, y: 0 })
const isDrawing = ref(false)

// 触摸状态
const touches = ref([])
const lastTouchDistance = ref(0)

onMounted(async () => {
  await nextTick()
  initCanvas()
  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
})

// 监听属性变化重绘
watch([() => props.pixels, () => props.gridVisible, () => props.showNumbers, () => props.zoom, () => props.pan], () => {
  drawCanvas()
})

function initCanvas() {
  if (!canvasRef.value || !containerRef.value) return
  
  ctx.value = canvasRef.value.getContext('2d')
  resizeCanvas()
  drawCanvas()
}

function resizeCanvas() {
  if (!canvasRef.value || !containerRef.value) return
  
  // 固定Canvas尺寸为容器大小
  canvasRef.value.width = 600
  canvasRef.value.height = 600
  
  drawCanvas()
}

function handleResize() {
  resizeCanvas()
}

function drawCanvas() {
  if (!ctx.value || !canvasRef.value) return
  
  const canvas = canvasRef.value
  const c = ctx.value
  
  // 清空画布
  c.clearRect(0, 0, canvas.width, canvas.height)
  
  // 绘制背景
  c.fillStyle = '#ffffff'
  c.fillRect(0, 0, canvas.width, canvas.height)
  
  // 固定单元格大小
  const cellSize = 12 * props.zoom
  
  // 计算居中偏移
  const totalWidth = props.width * cellSize
  const totalHeight = props.height * cellSize
  const offsetX = (canvas.width - totalWidth) / 2
  const offsetY = (canvas.height - totalHeight) / 2
  
  // 绘制网格
  if (props.gridVisible) {
    drawGrid(offsetX, offsetY, cellSize)
  }
  
  // 绘制拼豆
  drawBeads(offsetX, offsetY, cellSize)
  
  // 绘制编号
  if (props.showNumbers) {
    drawNumbers(offsetX, offsetY, cellSize)
  }
}

function drawGrid(offsetX, offsetY, cellSize) {
  const c = ctx.value
  c.strokeStyle = 'rgba(0, 0, 0, 0.1)'
  c.lineWidth = 1
  
  // 垂直线
  for (let x = 0; x <= props.width; x++) {
    const px = offsetX + x * cellSize
    c.beginPath()
    c.moveTo(px, offsetY)
    c.lineTo(px, offsetY + props.height * cellSize)
    c.stroke()
  }
  
  // 水平线
  for (let y = 0; y <= props.height; y++) {
    const py = offsetY + y * cellSize
    c.beginPath()
    c.moveTo(offsetX, py)
    c.lineTo(offsetX + props.width * cellSize, py)
    c.stroke()
  }
}
function drawBeads(offsetX, offsetY, cellSize) {
  const c = ctx.value
  
  props.pixels.forEach((colorCode, key) => {
    const [x, y] = key.split(',').map(Number)
    const color = getColorByCode(colorCode)
    if (!color) return
    
    const px = offsetX + x * cellSize
    const py = offsetY + y * cellSize
    
    // 绘制拼豆（圆形）
    c.fillStyle = color.hex
    c.beginPath()
    c.arc(px + cellSize / 2, py + cellSize / 2, cellSize * 0.4, 0, Math.PI * 2)
    c.fill()
    
    // 绘制边框
    c.strokeStyle = 'rgba(0, 0, 0, 0.2)'
    c.lineWidth = 1
    c.stroke()
  })
}

function drawNumbers(offsetX, offsetY, cellSize) {
  if (cellSize < 15) return // 缩放太小时不显示编号
  
  const c = ctx.value
  c.fillStyle = 'rgba(0, 0, 0, 0.7)'
  c.font = `${Math.min(cellSize * 0.3, 12)}px Arial`
  c.textAlign = 'center'
  c.textBaseline = 'middle'
  
  props.pixels.forEach((colorCode, key) => {
    const [x, y] = key.split(',').map(Number)
    const px = offsetX + x * cellSize + cellSize / 2
    const py = offsetY + y * cellSize + cellSize / 2
    
    c.fillText(colorCode, px, py)
  })
}

// 鼠标事件
function handleMouseDown(e) {
  const rect = canvasRef.value.getBoundingClientRect()
  const x = e.clientX - rect.left
  const y = e.clientY - rect.top
  
  isDrawing.value = true
  handlePixelInteraction(x, y)
}

function handleMouseMove(e) {
  if (!isDrawing.value) return
  
  const rect = canvasRef.value.getBoundingClientRect()
  const x = e.clientX - rect.left
  const y = e.clientY - rect.top
  
  handlePixelInteraction(x, y)
}

function handleMouseUp() {
  isDrawing.value = false
}

function handleWheel(e) {
  e.preventDefault()
  // 禁用滚轮缩放，改为使用按钮控制
}

// 触摸事件
function handleTouchStart(e) {
  e.preventDefault()
  touches.value = Array.from(e.touches)
  
  if (touches.value.length === 1) {
    const touch = touches.value[0]
    const rect = canvasRef.value.getBoundingClientRect()
    const x = touch.clientX - rect.left
    const y = touch.clientY - rect.top
    
    isDrawing.value = true
    handlePixelInteraction(x, y)
  }
}

function handleTouchMove(e) {
  e.preventDefault()
  touches.value = Array.from(e.touches)
  
  if (touches.value.length === 1 && isDrawing.value) {
    const touch = touches.value[0]
    const rect = canvasRef.value.getBoundingClientRect()
    const x = touch.clientX - rect.left
    const y = touch.clientY - rect.top
    
    handlePixelInteraction(x, y)
  }
}

function handleTouchEnd(e) {
  e.preventDefault()
  touches.value = Array.from(e.touches)
  
  if (touches.value.length === 0) {
    isDrawing.value = false
  }
}

function getTouchDistance(touch1, touch2) {
  const dx = touch1.clientX - touch2.clientX
  const dy = touch1.clientY - touch2.clientY
  return Math.sqrt(dx * dx + dy * dy)
}

function handlePixelInteraction(x, y) {
  const canvas = canvasRef.value
  const cellSize = 12 * props.zoom
  
  // 计算居中偏移
  const totalWidth = props.width * cellSize
  const totalHeight = props.height * cellSize
  const offsetX = (canvas.width - totalWidth) / 2
  const offsetY = (canvas.height - totalHeight) / 2
  
  const gridX = Math.floor((x - offsetX) / cellSize)
  const gridY = Math.floor((y - offsetY) / cellSize)
  
  if (gridX < 0 || gridX >= props.width || gridY < 0 || gridY >= props.height) return
  
  const key = `${gridX},${gridY}`
  const newPixels = new Map(props.pixels)
  
  if (props.selectedTool === 'eraser') {
    newPixels.delete(key)
  } else if (props.selectedTool === 'pencil' && props.selectedColor) {
    newPixels.set(key, props.selectedColor)
  } else if (props.selectedTool === 'eyedropper') {
    const existingColor = props.pixels.get(key)
    if (existingColor) {
      emit('color-picked', existingColor)
    }
  }
  
  emit('pixel-change', newPixels)
}
</script>

<style scoped>
.pindou-canvas-container {
  width: 600px;
  height: 600px;
  overflow: hidden;
  position: relative;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.12);
}

.pindou-canvas {
  display: block;
  cursor: crosshair;
  touch-action: none;
}

.pindou-canvas[data-tool="eyedropper"] {
  cursor: crosshair;
}

.pindou-canvas[data-tool="eraser"] {
  cursor: crosshair;
}
</style>
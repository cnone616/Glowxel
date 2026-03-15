<template>
  <div class="canvas-wrapper">
    <div class="canvas-header">
      <span class="canvas-size">{{ width }}×{{ height }}</span>
      <div class="canvas-actions">
        <button @click="$emit('fit')" class="icon-btn" title="适应屏幕">适应</button>
        <button @click="$emit('zoom', -1)" class="icon-btn" title="缩小">-</button>
        <button @click="$emit('zoom', 1)" class="icon-btn" title="放大">+</button>
        <button @click="$emit('toggle-grid')" :class="['icon-btn', { active: gridVisible }]" title="网格">网格</button>
      </div>
    </div>
    
    <div class="canvas-container" ref="containerRef">
      <canvas 
        ref="canvasRef" 
        :width="width * pixelSize" 
        :height="height * pixelSize"
        class="pixel-canvas"
        @mousedown="handleMouseDown"
        @mousemove="handleMouseMove"
        @mouseup="handleMouseUp"
        @mouseleave="handleMouseUp"
        @wheel.prevent="handleWheel"
      ></canvas>
    </div>
    
    <div class="canvas-info">
      <span>当前位置: {{ currentPixel.x }}, {{ currentPixel.y }}</span>
      <span>缩放: {{ Math.round(zoom * 100) }}%</span>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onMounted, nextTick } from 'vue'
import { getColorByCode } from '@/data/artkal-colors'

const props = defineProps({
  width: { type: Number, required: true },
  height: { type: Number, required: true },
  pixels: { type: Map, required: true },
  tool: { type: String, default: 'pencil' },
  selectedColor: { type: String, default: '' },
  gridVisible: { type: Boolean, default: true }
})

const emit = defineEmits(['pixel-click', 'zoom', 'fit', 'toggle-grid'])

const canvasRef = ref(null)
const containerRef = ref(null)
const pixelSize = ref(10)
const zoom = ref(1)
const pan = ref({ x: 0, y: 0 })
const isDrawing = ref(false)
const currentPixel = ref({ x: 0, y: 0 })

const drawCanvas = () => {
  const canvas = canvasRef.value
  if (!canvas) return
  
  const ctx = canvas.getContext('2d')
  const scale = pixelSize.value
  
  // 清空画布
  ctx.fillStyle = '#111'
  ctx.fillRect(0, 0, canvas.width, canvas.height)
  
  // 绘制网格
  if (props.gridVisible) {
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.1)'
    ctx.lineWidth = 1
    
    for (let x = 0; x <= props.width; x++) {
      ctx.beginPath()
      ctx.moveTo(x * scale, 0)
      ctx.lineTo(x * scale, props.height * scale)
      ctx.stroke()
    }
    
    for (let y = 0; y <= props.height; y++) {
      ctx.beginPath()
      ctx.moveTo(0, y * scale)
      ctx.lineTo(props.width * scale, y * scale)
      ctx.stroke()
    }
  }
  
  // 绘制像素
  props.pixels.forEach((colorCode, key) => {
    const [x, y] = key.split(',').map(Number)
    const color = getColorByCode(colorCode)
    if (color) {
      ctx.fillStyle = color.hex
      ctx.fillRect(x * scale, y * scale, scale, scale)
    }
  })
}

const getCanvasCoords = (event) => {
  const canvas = canvasRef.value
  const rect = canvas.getBoundingClientRect()
  const x = Math.floor((event.clientX - rect.left) / pixelSize.value)
  const y = Math.floor((event.clientY - rect.top) / pixelSize.value)
  return { x, y }
}

const handleMouseDown = (event) => {
  if (props.tool === 'move') return
  isDrawing.value = true
  handleMouseMove(event)
}

const handleMouseMove = (event) => {
  const coords = getCanvasCoords(event)
  currentPixel.value = coords
  
  if (isDrawing.value && props.tool !== 'move') {
    emit('pixel-click', coords.x, coords.y)
  }
}

const handleMouseUp = () => {
  isDrawing.value = false
}

const handleWheel = (event) => {
  const delta = event.deltaY > 0 ? -1 : 1
  emit('zoom', delta)
}

watch(() => props.pixels, drawCanvas, { deep: true })
watch(() => props.gridVisible, drawCanvas)

onMounted(() => {
  nextTick(() => {
    drawCanvas()
  })
})

defineExpose({ drawCanvas })
</script>

<style scoped>
.canvas-wrapper {
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.canvas-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
}

.canvas-size {
  color: #00f3ff;
  font-family: monospace;
  font-weight: 600;
}

.canvas-actions {
  display: flex;
  gap: 8px;
}

.icon-btn {
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: rgba(255, 255, 255, 0.7);
  border-radius: 6px;
  cursor: pointer;
  font-size: 12px;
  transition: all 0.3s;
}

.icon-btn:hover {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
}

.icon-btn.active {
  background: rgba(0, 243, 255, 0.2);
  border-color: #00f3ff;
  color: #00f3ff;
}

.canvas-container {
  background: #000;
  border: 2px solid #00f3ff;
  border-radius: 12px;
  overflow: auto;
  max-height: 600px;
}

.pixel-canvas {
  display: block;
  cursor: crosshair;
  image-rendering: pixelated;
}

.canvas-info {
  display: flex;
  justify-content: space-between;
  padding: 8px 16px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.6);
  font-family: monospace;
}
</style>

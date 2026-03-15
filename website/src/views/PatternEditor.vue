<template>
  <div class="pattern-editor">
    <div class="container">
      <h1 class="page-title">图案编辑器</h1>
      
      <!-- 步骤指示器 -->
      <div class="steps-indicator">
        <div 
          v-for="(stepInfo, index) in steps"
          :key="index"
          class="step-item"
          :class="{ active: step === index, completed: step > index }"
        >
          <div class="step-number">{{ index + 1 }}</div>
          <div class="step-label">{{ stepInfo }}</div>
        </div>
      </div>
      
      <!-- 步骤内容 -->
      <div class="step-content">
        <!-- 步骤 0: 选择模式 -->
        <StepMode 
          v-if="step === 0"
          v-model="mode"
        />
        
        <!-- 步骤 1: 设置尺寸 -->
        <StepSize
          v-if="step === 1"
          :mode="mode"
          v-model:width="width"
          v-model:height="height"
          v-model:imageUrl="imageUrl"
          v-model:imageFile="imageFile"
        />
        
        <!-- 步骤 2: 选择颜色套装 -->
        <StepColorSet
          v-if="step === 2"
          v-model="selectedPreset"
        />
        
        <!-- 步骤 3: 编辑器 -->
        <div v-if="step === 3" class="editor-container">
          <PatternToolbar
            v-model:tool="tool"
            v-model:gridVisible="gridVisible"
            :canUndo="historyIndex > 0"
            :canRedo="historyIndex < history.length - 1"
            @undo="undo"
            @redo="redo"
            @fit="fitCanvas"
            @export="exportPattern"
          />
          
          <div class="canvas-wrapper" ref="canvasWrapper">
            <canvas
              ref="canvas"
              class="pattern-canvas"
              :data-tool="tool"
              @mousedown="handleMouseDown"
              @mousemove="handleMouseMove"
              @mouseup="handleMouseUp"
              @mouseleave="handleMouseUp"
              @wheel="handleWheel"
            ></canvas>
          </div>
          
          <div class="color-palette-wrapper">
            <h3>颜色选择</h3>
            <div class="color-grid">
              <div
                v-for="color in paletteColors"
                :key="color.code"
                class="color-item"
                :class="{ active: selectedColor === color.code }"
                :style="{ backgroundColor: color.hex }"
                :title="`${color.code} - ${color.name}`"
                @click="selectedColor = color.code"
              >
                <span class="color-code">{{ color.code }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <!-- 导航按钮 -->
      <div class="navigation">
        <button
          v-if="step > 0"
          class="btn-secondary"
          @click="prevStep"
        >
          上一步
        </button>
        <button
          v-if="step < 3"
          class="btn-primary"
          :disabled="!canProceed"
          @click="nextStep"
        >
          {{ step === 2 && mode === 'image' ? '生成预览' : '下一步' }}
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import StepMode from '@/components/pattern/StepMode.vue'
import StepSize from '@/components/pattern/StepSize.vue'
import StepColorSet from '@/components/pattern/StepColorSet.vue'
import PatternToolbar from '@/components/pattern/PatternToolbar.vue'
import { ARTKAL_OFFICIAL_SETS, getColorByCode } from '@/data/artkal-colors'
import { imageToPixels, getUsedColors } from '@/utils/imageProcessor.js'
import { exportPattern as exportPatternImage, downloadImage } from '@/utils/patternExporter.js'

// 步骤
const steps = ['选择模式', '设置尺寸', '选择颜色', '编辑创作']
const step = ref(0)

// 模式和尺寸
const mode = ref('blank')
const width = ref(52)
const height = ref(52)
const imageUrl = ref(null)
const imageFile = ref(null)

// 颜色套装
const selectedPreset = ref('set24')

// 编辑器状态
const tool = ref('pencil')
const gridVisible = ref(true)
const selectedColor = ref('')
const pixels = ref(new Map())
const history = ref([])
const historyIndex = ref(-1)

// Canvas
const canvas = ref(null)
const canvasWrapper = ref(null)
const ctx = ref(null)
const zoom = ref(10)
const pan = ref({ x: 0, y: 0 })
const isDragging = ref(false)
const lastPos = ref({ x: 0, y: 0 })

// 计算属性
const paletteColors = computed(() => {
  const colorCodes = ARTKAL_OFFICIAL_SETS[selectedPreset.value]?.colors || []
  return colorCodes.map(code => getColorByCode(code)).filter(c => c)
})

const canProceed = computed(() => {
  if (step.value === 0) return true
  if (step.value === 1) {
    if (mode.value === 'image' && !imageFile.value) return false
    return width.value > 0 && height.value > 0 && width.value <= 520 && height.value <= 520
  }
  if (step.value === 2) return true
  return false
})

// 初始化
onMounted(async () => {
  await nextTick()
  // Canvas 会在步骤3时才渲染，所以这里不需要初始化
  window.addEventListener('resize', handleResize)
})

// 清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
})

// 处理窗口大小变化
function handleResize() {
  if (step.value === 3 && canvas.value) {
    resizeCanvas()
  }
}

// 监听步骤变化
watch(step, async (newStep) => {
  if (newStep === 3) {
    await nextTick()
    initEditor()
  }
})

// 监听像素变化，自动重绘
watch(pixels, () => {
  if (step.value === 3 && ctx.value) {
    console.log('像素数据变化，重新绘制')
    drawCanvas()
  }
})

// 步骤导航
function prevStep() {
  if (step.value > 0) step.value--
}

async function nextStep() {
  if (!canProceed.value) return
  
  if (step.value === 2 && mode.value === 'image') {
    // 图片模式：生成预览
    await processImage()
  }
  
  if (step.value < 3) {
    step.value++
  }
}

// 处理图片
async function processImage() {
  if (!imageFile.value) {
    console.error('没有图片文件')
    return
  }
  
  console.log('开始处理图片...')
  const img = new Image()
  img.src = imageUrl.value
  
  await new Promise((resolve, reject) => {
    img.onload = () => {
      console.log('图片加载完成:', img.width, 'x', img.height)
      const colorCodes = ARTKAL_OFFICIAL_SETS[selectedPreset.value]?.colors || []
      console.log('使用颜色套装:', selectedPreset.value, '颜色数量:', colorCodes.length)
      console.log('目标尺寸:', width.value, 'x', height.value)
      
      const pixelMap = imageToPixels(img, width.value, height.value, colorCodes)
      console.log('图片转换完成，生成像素数量:', pixelMap.size)
      
      pixels.value = pixelMap
      // 图片生成后默认使用拖动工具
      tool.value = 'move'
      saveHistory()
      resolve()
    }
    img.onerror = (err) => {
      console.error('图片加载失败:', err)
      reject(err)
    }
  })
}

// 初始化编辑器
async function initEditor() {
  console.log('初始化编辑器, 当前像素数量:', pixels.value.size)
  
  // 等待 DOM 更新
  await nextTick()
  
  // 检查 canvas 元素
  if (!canvas.value) {
    console.error('Canvas 元素未找到')
    return
  }
  
  if (!canvasWrapper.value) {
    console.error('Canvas wrapper 未找到')
    return
  }
  
  // 初始化 canvas context
  if (!ctx.value) {
    ctx.value = canvas.value.getContext('2d')
    console.log('Canvas context 已初始化')
  }
  
  // 设置默认颜色
  if (!selectedColor.value && paletteColors.value.length > 0) {
    selectedColor.value = paletteColors.value[0].code
    console.log('设置默认颜色:', selectedColor.value)
  }
  
  // 空白模式初始化
  if (mode.value === 'blank' && pixels.value.size === 0) {
    pixels.value = new Map()
    saveHistory()
    console.log('空白模式初始化')
  }
  
  // 再次等待确保 canvas 完全渲染
  await nextTick()
  
  // 调整 canvas 大小
  const wrapper = canvasWrapper.value
  canvas.value.width = wrapper.clientWidth
  canvas.value.height = wrapper.clientHeight
  console.log('Canvas 尺寸设置为:', canvas.value.width, 'x', canvas.value.height)
  
  // 计算合适的缩放和位置
  fitCanvas()
  
  // 强制绘制一次
  console.log('强制绘制 canvas')
  drawCanvas()
}

// Canvas 操作
function resizeCanvas() {
  if (!canvas.value || !canvasWrapper.value) {
    console.warn('Canvas 或 wrapper 未就绪')
    return
  }
  
  const wrapper = canvasWrapper.value
  const newWidth = wrapper.clientWidth
  const newHeight = wrapper.clientHeight
  
  if (newWidth === 0 || newHeight === 0) {
    console.warn('Canvas wrapper 尺寸为 0')
    return
  }
  
  canvas.value.width = newWidth
  canvas.value.height = newHeight
  
  console.log('Canvas 已调整大小:', newWidth, 'x', newHeight)
  drawCanvas()
}

function fitCanvas() {
  if (!canvas.value) return
  
  const padding = 40
  const availableWidth = canvas.value.width - padding * 2
  const availableHeight = canvas.value.height - padding * 2
  
  const zoomX = availableWidth / width.value
  const zoomY = availableHeight / height.value
  zoom.value = Math.min(zoomX, zoomY, 20)
  
  pan.value = {
    x: (canvas.value.width - width.value * zoom.value) / 2,
    y: (canvas.value.height - height.value * zoom.value) / 2
  }
  
  drawCanvas()
}

function drawCanvas() {
  if (!ctx.value || !canvas.value) {
    console.warn('Canvas context 未初始化')
    return
  }
  
  console.log('开始绘制 canvas, 像素数量:', pixels.value.size)
  
  const c = ctx.value
  c.clearRect(0, 0, canvas.value.width, canvas.value.height)
  
  // 绘制背景 - 改为白色
  c.fillStyle = '#ffffff'
  c.fillRect(0, 0, canvas.value.width, canvas.value.height)
  
  // 绘制网格 - 改为浅灰色
  if (gridVisible.value) {
    c.strokeStyle = 'rgba(0, 0, 0, 0.08)'
    c.lineWidth = 1
    
    for (let x = 0; x <= width.value; x++) {
      const px = pan.value.x + x * zoom.value
      c.beginPath()
      c.moveTo(px, pan.value.y)
      c.lineTo(px, pan.value.y + height.value * zoom.value)
      c.stroke()
    }
    
    for (let y = 0; y <= height.value; y++) {
      const py = pan.value.y + y * zoom.value
      c.beginPath()
      c.moveTo(pan.value.x, py)
      c.lineTo(pan.value.x + width.value * zoom.value, py)
      c.stroke()
    }
  }
  
  // 绘制像素
  let drawnCount = 0
  pixels.value.forEach((colorCode, key) => {
    const [x, y] = key.split(',').map(Number)
    const color = getColorByCode(colorCode)
    if (color) {
      c.fillStyle = color.hex
      c.fillRect(
        pan.value.x + x * zoom.value,
        pan.value.y + y * zoom.value,
        zoom.value,
        zoom.value
      )
      drawnCount++
    }
  })
  
  console.log('已绘制像素数量:', drawnCount)
}

// 鼠标事件
function handleMouseDown(e) {
  const rect = canvas.value.getBoundingClientRect()
  const x = e.clientX - rect.left
  const y = e.clientY - rect.top
  
  if (tool.value === 'move') {
    isDragging.value = true
    lastPos.value = { x, y }
  } else {
    const gridX = Math.floor((x - pan.value.x) / zoom.value)
    const gridY = Math.floor((y - pan.value.y) / zoom.value)
    
    if (gridX >= 0 && gridX < width.value && gridY >= 0 && gridY < height.value) {
      drawPixel(gridX, gridY)
    }
  }
}

function handleMouseMove(e) {
  if (!isDragging.value) return
  
  const rect = canvas.value.getBoundingClientRect()
  const x = e.clientX - rect.left
  const y = e.clientY - rect.top
  
  if (tool.value === 'move') {
    pan.value.x += x - lastPos.value.x
    pan.value.y += y - lastPos.value.y
    lastPos.value = { x, y }
    drawCanvas()
  }
}

function handleMouseUp() {
  isDragging.value = false
}

function handleWheel(e) {
  e.preventDefault()
  
  const rect = canvas.value.getBoundingClientRect()
  const mouseX = e.clientX - rect.left
  const mouseY = e.clientY - rect.top
  
  const delta = e.deltaY > 0 ? 0.9 : 1.1
  const newZoom = Math.max(2, Math.min(50, zoom.value * delta))
  
  pan.value.x = mouseX - (mouseX - pan.value.x) * (newZoom / zoom.value)
  pan.value.y = mouseY - (mouseY - pan.value.y) * (newZoom / zoom.value)
  zoom.value = newZoom
  
  drawCanvas()
}

function drawPixel(x, y) {
  const key = `${x},${y}`
  const newPixels = new Map(pixels.value)
  
  if (tool.value === 'eraser') {
    newPixels.delete(key)
  } else if (tool.value === 'pencil' && selectedColor.value) {
    newPixels.set(key, selectedColor.value)
  }
  
  pixels.value = newPixels
  saveHistory()
  drawCanvas()
}

// 历史记录
function saveHistory() {
  const newHistory = history.value.slice(0, historyIndex.value + 1)
  newHistory.push(new Map(pixels.value))
  if (newHistory.length > 50) newHistory.shift()
  
  history.value = newHistory
  historyIndex.value = newHistory.length - 1
}

function undo() {
  if (historyIndex.value > 0) {
    historyIndex.value--
    pixels.value = new Map(history.value[historyIndex.value])
    drawCanvas()
  }
}

function redo() {
  if (historyIndex.value < history.value.length - 1) {
    historyIndex.value++
    pixels.value = new Map(history.value[historyIndex.value])
    drawCanvas()
  }
}

// 导出
async function exportPattern() {
  if (pixels.value.size === 0) {
    alert('画布为空，无法导出')
    return
  }
  
  try {
    // 获取使用的颜色统计
    const usedColors = getUsedColors(pixels.value)
    
    // 生成图纸
    const dataUrl = await exportPatternImage(
      pixels.value,
      width.value,
      height.value,
      '像素图案',
      usedColors
    )
    
    // 下载图片
    const filename = `pattern_${width.value}x${height.value}_${Date.now()}.png`
    downloadImage(dataUrl, filename)
  } catch (error) {
    console.error('导出失败:', error)
    alert('导出失败，请重试')
  }
}
</script>

<style scoped>
.pattern-editor {
  padding: 40px 0 80px;
  min-height: 100vh;
}

.container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 20px;
}

.page-title {
  font-size: 48px;
  font-weight: 800;
  margin-bottom: 48px;
  text-align: center;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.steps-indicator {
  display: flex;
  justify-content: center;
  gap: 40px;
  margin-bottom: 60px;
  flex-wrap: wrap;
}

.step-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  opacity: 0.4;
  transition: all 0.3s;
}

.step-item.active,
.step-item.completed {
  opacity: 1;
}

.step-number {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.9);
  border: 3px solid rgba(255, 107, 107, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 700;
  color: #a0aec0;
  box-shadow: 0 4px 12px rgba(255, 107, 107, 0.1);
  transition: all 0.3s ease;
}

.step-item.active .step-number {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
  box-shadow: 0 6px 20px rgba(255, 107, 107, 0.4);
  transform: scale(1.1);
}

.step-item.completed .step-number {
  background: rgba(255, 107, 107, 0.15);
  border-color: #ff6b6b;
  color: #ff6b6b;
}

.step-label {
  font-size: 14px;
  font-weight: 500;
  color: #718096;
}

.step-item.active .step-label {
  color: #2d3748;
  font-weight: 600;
}

.step-content {
  min-height: 400px;
  margin-bottom: 40px;
}

.editor-container {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.canvas-wrapper {
  width: 100%;
  height: 650px;
  background: #ffffff;
  border-radius: 20px;
  overflow: hidden;
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.12);
  border: 2px solid rgba(255, 107, 107, 0.15);
}

.pattern-canvas {
  display: block;
  cursor: crosshair;
}

.pattern-canvas[data-tool="move"] {
  cursor: grab;
}

.pattern-canvas[data-tool="move"]:active {
  cursor: grabbing;
}

.color-palette-wrapper {
  padding: 28px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.color-palette-wrapper h3 {
  font-size: 20px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 20px;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(60px, 1fr));
  gap: 8px;
}

.color-item {
  aspect-ratio: 1;
  border-radius: 12px;
  border: 3px solid rgba(255, 107, 107, 0.15);
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.color-item:hover {
  transform: scale(1.12);
  border-color: #ff6b6b;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.color-item.active {
  border-color: #ff6b6b;
  border-width: 4px;
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.5);
  transform: scale(1.15);
}

.color-code {
  font-size: 11px;
  font-weight: 800;
  color: #2d3748;
  text-shadow: 0 1px 3px rgba(255, 255, 255, 0.9);
  background: rgba(255, 255, 255, 0.85);
  padding: 2px 6px;
  border-radius: 4px;
}

.navigation {
  display: flex;
  justify-content: center;
  gap: 20px;
  margin-top: 20px;
}

.btn-primary,
.btn-secondary {
  padding: 18px 56px;
  border: none;
  border-radius: 16px;
  font-size: 17px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  min-width: 160px;
}

.btn-primary {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  color: #ffffff;
  box-shadow: 0 6px 20px rgba(255, 107, 107, 0.3);
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-3px);
  box-shadow: 0 10px 32px rgba(255, 107, 107, 0.4);
}

.btn-primary:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  transform: none;
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.95);
  color: #ff6b6b;
  border: 2px solid rgba(255, 107, 107, 0.3);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.1);
}

.btn-secondary:hover {
  background: #ffffff;
  border-color: #ff6b6b;
  transform: translateY(-3px);
  box-shadow: 0 8px 24px rgba(255, 107, 107, 0.2);
}

@media (max-width: 768px) {
  .pattern-editor {
    padding: 24px 0 60px;
  }
  
  .page-title {
    font-size: 36px;
    margin-bottom: 32px;
  }
  
  .steps-indicator {
    gap: 20px;
    margin-bottom: 40px;
  }
  
  .step-number {
    width: 40px;
    height: 40px;
  }
  
  .step-label {
    font-size: 12px;
  }
  
  .canvas-wrapper {
    height: 400px;
  }
  
  .color-grid {
    grid-template-columns: repeat(auto-fill, minmax(50px, 1fr));
    gap: 6px;
  }
  
  .color-item {
    border-width: 2px;
  }
  
  .color-code {
    font-size: 9px;
    padding: 1px 4px;
  }
  
  .navigation {
    flex-direction: column;
    gap: 12px;
  }
  
  .btn-primary,
  .btn-secondary {
    width: 100%;
    padding: 16px 32px;
  }
}
</style>

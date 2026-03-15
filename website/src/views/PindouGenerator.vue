<template>
  <div class="pindou-generator">
    <div class="container">
      <div class="main-content" :class="{ 'mobile-layout': isMobile }">
        <!-- 左侧工具栏 -->
        <div class="left-sidebar" v-if="!isMobile">
          <ImageUploadSection 
            v-model:imageUrl="imageUrl"
            v-model:imageFile="imageFile"
            @image-processed="handleImageProcessed"
          />
          
          <ToolsPanel 
            v-model:selectedTool="selectedTool"
            v-model:gridVisible="gridVisible"
            v-model:showNumbers="showNumbers"
            :canUndo="historyIndex > 0"
            :canRedo="historyIndex < history.length - 1"
            :zoom="zoom * 100"
            @undo="undo"
            @redo="redo"
            @clear-background="clearBackground"
            @zoom-in="zoomIn"
            @zoom-out="zoomOut"
            @fit-canvas="fitCanvas"
          />
          
          <SettingsPanel 
            v-model:gridSize="gridSize"
            v-model:unit="selectedUnit"
            v-model:difficulty="difficulty"
            v-model:brand="selectedBrand"
            @difficulty-changed="handleDifficultyChanged"
          />
        </div>

        <!-- 中央画布区域 -->
        <div class="canvas-area">
          <!-- 原来的容器保持不变，但是隐藏 -->
        </div>

        <!-- 屏幕居中的Canvas -->
        <div class="canvas-screen-center">
          <PindouCanvas
            ref="canvasRef"
            :width="canvasWidth"
            :height="canvasHeight"
            :pixels="pixels"
            :gridVisible="gridVisible"
            :showNumbers="showNumbers"
            :selectedTool="selectedTool"
            :selectedColor="selectedColor"
            :zoom="zoom"
            :pan="pan"
            @pixel-change="handlePixelChange"
            @zoom-change="handleZoomChange"
            @pan-change="handlePanChange"
            @color-picked="handleColorPicked"
          />
        </div>

        <!-- 右侧面板 -->
        <div class="right-sidebar" v-if="!isMobile">
          <ColorPalette 
            v-model:selectedColor="selectedColor"
            :colors="paletteColors"
            :usedColors="usedColors"
          />
          
          <ExportPanel 
            :pixels="pixels"
            :width="canvasWidth"
            :height="canvasHeight"
            :colors="paletteColors"
            :usedColors="usedColors"
            @export="handleExport"
          />
        </div>
      </div>

      <!-- 移动端底部工具栏 -->
      <div class="mobile-toolbar" v-if="isMobile">
        <button class="mobile-btn" @click="showMobilePanel = 'upload'">
          上传
        </button>
        <button class="mobile-btn" @click="showMobilePanel = 'tools'">
          工具
        </button>
        <button class="mobile-btn" @click="showMobilePanel = 'colors'">
          颜色
        </button>
        <button class="mobile-btn" @click="showMobilePanel = 'export'">
          导出
        </button>
      </div>
    </div>

    <!-- 移动端弹窗 -->
    <MobilePanel 
      v-if="isMobile && showMobilePanel"
      :type="showMobilePanel"
      @close="showMobilePanel = null"
    >
      <ImageUploadSection 
        v-if="showMobilePanel === 'upload'"
        v-model:imageUrl="imageUrl"
        v-model:imageFile="imageFile"
        @image-processed="handleImageProcessed"
      />
      
      <ToolsPanel 
        v-if="showMobilePanel === 'tools'"
        v-model:selectedTool="selectedTool"
        v-model:gridVisible="gridVisible"
        v-model:showNumbers="showNumbers"
        :canUndo="historyIndex > 0"
        :canRedo="historyIndex < history.length - 1"
        @undo="undo"
        @redo="redo"
        @clear-background="clearBackground"
      />
      
      <ColorPalette 
        v-if="showMobilePanel === 'colors'"
        v-model:selectedColor="selectedColor"
        :colors="paletteColors"
        :usedColors="usedColors"
      />
      
      <ExportPanel 
        v-if="showMobilePanel === 'export'"
        :pixels="pixels"
        :width="canvasWidth"
        :height="canvasHeight"
        :colors="paletteColors"
        :usedColors="usedColors"
        @export="handleExport"
      />
    </MobilePanel>
  </div>
</template>
<script setup>
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { ARTKAL_OFFICIAL_SETS, getColorByCode, findClosestColor } from '@/data/artkal-colors'
import { imageToPixels, getUsedColors } from '@/utils/imageProcessor.js'
import { exportToPDF } from '@/utils/pindouExporter.js'

// 导入组件
import ImageUploadSection from '@/components/pindou/ImageUploadSection.vue'
import ToolsPanel from '@/components/pindou/ToolsPanel.vue'
import SettingsPanel from '@/components/pindou/SettingsPanel.vue'
import PindouCanvas from '@/components/pindou/PindouCanvas.vue'
import ColorPalette from '@/components/pindou/ColorPalette.vue'
import ExportPanel from '@/components/pindou/ExportPanel.vue'
import MobilePanel from '@/components/pindou/MobilePanel.vue'

// 响应式检测
const isMobile = ref(window.innerWidth <= 768)

// 图片相关
const imageUrl = ref(null)
const imageFile = ref(null)

// 画布设置
const canvasWidth = ref(50)
const canvasHeight = ref(50)
const gridSize = ref(17)
const selectedUnit = ref('pixel')
const difficulty = ref('medium')
const selectedBrand = ref('artkal')

// 工具状态
const selectedTool = ref('pencil')
const gridVisible = ref(true)
const showNumbers = ref(true)
const selectedColor = ref('')

// 画布状态
const pixels = ref(new Map())
const zoom = ref(1)
const pan = ref({ x: 0, y: 0 })

// 历史记录
const history = ref([])
const historyIndex = ref(-1)

// 移动端状态
const showMobilePanel = ref(null)

// Canvas 引用
const canvasRef = ref(null)

// 计算属性
const paletteColors = computed(() => {
  const colorSet = selectedBrand.value === 'artkal' ? 'set221' : 'set24'
  const colorCodes = ARTKAL_OFFICIAL_SETS[colorSet]?.colors || []
  return colorCodes.map(code => getColorByCode(code)).filter(c => c)
})

const usedColors = computed(() => {
  return getUsedColors(pixels.value)
})

// 生命周期
onMounted(() => {
  window.addEventListener('resize', handleResize)
  handleResize()
  
  // 设置默认颜色
  if (paletteColors.value.length > 0) {
    selectedColor.value = paletteColors.value[0].code
  }
  
  // 初始化历史记录
  saveHistory()
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
})

// 方法
function handleResize() {
  isMobile.value = window.innerWidth <= 768
}

function handleImageProcessed(processedPixels) {
  pixels.value = processedPixels
  saveHistory()
}

function handlePixelChange(newPixels) {
  pixels.value = newPixels
  saveHistory()
}

function handleZoomChange(newZoom) {
  zoom.value = newZoom
}

function handlePanChange(newPan) {
  pan.value = newPan
}

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
  }
}

function redo() {
  if (historyIndex.value < history.value.length - 1) {
    historyIndex.value++
    pixels.value = new Map(history.value[historyIndex.value])
  }
}

function clearBackground() {
  pixels.value = new Map()
  saveHistory()
}

function handleDifficultyChanged(sizeConfig) {
  canvasWidth.value = sizeConfig.width
  canvasHeight.value = sizeConfig.height
  
  // 清空当前像素数据
  pixels.value = new Map()
  saveHistory()
  
  // 如果有图片，重新处理
  if (imageFile.value) {
    processImageToPixels(imageUrl.value)
  }
}

function zoomIn() {
  zoom.value = Math.min(zoom.value * 1.2, 5)
}

function zoomOut() {
  zoom.value = Math.max(zoom.value / 1.2, 0.1)
}

function fitCanvas() {
  zoom.value = 1
  pan.value = { x: 0, y: 0 }
}

function processImageToPixels(url) {
  const img = new Image()
  img.onload = () => {
    // 使用当前画布尺寸进行转换
    const colorCodes = ARTKAL_OFFICIAL_SETS.set221?.colors || []
    const processedPixels = imageToPixels(img, canvasWidth.value, canvasHeight.value, colorCodes)
    pixels.value = processedPixels
    saveHistory()
  }
  img.src = url
}

function handleColorPicked(colorCode) {
  selectedColor.value = colorCode
}

async function handleExport(options) {
  try {
    await exportToPDF(pixels.value, canvasWidth.value, canvasHeight.value, {
      ...options,
      colors: paletteColors.value,
      usedColors: usedColors.value
    })
  } catch (error) {
    console.error('导出失败:', error)
    alert('导出失败，请重试')
  }
}
</script>
<style scoped>
.pindou-generator {
  min-height: 100vh;
  background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 50%, #ff9a9e 100%);
  background-attachment: fixed;
}

.security-banner {
  background: rgba(255, 193, 7, 0.1);
  border: 2px solid rgba(255, 193, 7, 0.3);
  border-radius: 12px;
  padding: 16px 20px;
  margin: 20px;
  display: flex;
  align-items: center;
  gap: 12px;
  backdrop-filter: blur(10px);
}

.banner-icon {
  font-size: 24px;
  flex-shrink: 0;
}

.banner-text {
  font-size: 14px;
  color: #856404;
  line-height: 1.5;
}

.container {
  max-width: 1600px;
  margin: 0 auto;
  padding: 0 20px;
}

.page-title {
  font-size: 48px;
  font-weight: 800;
  margin: 40px 0 16px;
  text-align: center;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.page-subtitle {
  font-size: 18px;
  color: #666;
  text-align: center;
  margin-bottom: 40px;
  line-height: 1.6;
}

.main-content {
  display: grid;
  grid-template-columns: 300px 1fr 320px;
  gap: 24px;
  margin-bottom: 40px;
}

.main-content.mobile-layout {
  grid-template-columns: 1fr;
  gap: 16px;
}

.left-sidebar,
.right-sidebar {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.canvas-area {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.12);
  overflow: hidden;
  min-height: 600px;
}

.canvas-screen-center {
  position: fixed;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 600px;
  height: 600px;
  z-index: 100;
  overflow: hidden;
}

.mobile-toolbar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(20px);
  border-top: 2px solid rgba(255, 107, 107, 0.15);
  padding: 12px 20px;
  display: flex;
  justify-content: space-around;
  gap: 8px;
  z-index: 1000;
}

.mobile-btn {
  flex: 1;
  padding: 12px 8px;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  color: white;
  border: none;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.mobile-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.4);
}

@media (max-width: 768px) {
  .security-banner {
    margin: 12px;
    padding: 12px 16px;
  }
  
  .banner-text {
    font-size: 13px;
  }
  
  .page-title {
    font-size: 36px;
    margin: 24px 0 12px;
  }
  
  .page-subtitle {
    font-size: 16px;
    margin-bottom: 24px;
    padding: 0 16px;
  }
  
  .container {
    padding: 0 12px;
  }
  
  .canvas-area {
    min-height: 400px;
    border-radius: 16px;
  }
  
  .canvas-screen-center {
    position: relative;
    top: auto;
    left: auto;
    transform: none;
    width: 100%;
    height: 400px;
    z-index: auto;
  }
  
  .main-content {
    margin-bottom: 80px; /* 为底部工具栏留空间 */
  }
}

@media (max-width: 1200px) and (min-width: 769px) {
  .main-content {
    grid-template-columns: 280px 1fr 300px;
    gap: 20px;
  }
}

@media (max-width: 1024px) and (min-width: 769px) {
  .main-content {
    grid-template-columns: 260px 1fr 280px;
    gap: 16px;
  }
  
  .page-title {
    font-size: 42px;
  }
}
</style>
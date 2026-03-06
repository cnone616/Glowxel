<template>
  <div class="clock-designer">
    <div class="container">
      <h1 class="page-title">时钟设计器</h1>
      
      <div class="designer-layout">
        <div class="preview-section">
          <div class="preview-header">
            <h3>实时预览</h3>
            <div class="preview-actions">
              <button @click="saveConfig" class="btn-action">保存</button>
              <button @click="exportConfig" class="btn-action">导出</button>
            </div>
          </div>
          
          <div class="canvas-container">
            <div class="axis-wrapper">
              <div class="y-axis">
                <div>0</div>
                <div>16</div>
                <div>32</div>
                <div>48</div>
                <div>63</div>
              </div>
              <div>
                <canvas ref="canvasRef" width="640" height="640" class="preview-canvas"></canvas>
                <div class="x-axis">
                  <div>0</div>
                  <div>16</div>
                  <div>32</div>
                  <div>48</div>
                  <div>63</div>
                </div>
              </div>
            </div>
          </div>
          
          <div class="templates">
            <h4>快速模板</h4>
            <div class="template-grid">
              <div 
                v-for="template in templates" 
                :key="template.id"
                class="template-item"
                @click="applyTemplate(template)"
              >
                <div class="template-preview">{{ template.name }}</div>
              </div>
            </div>
          </div>
        </div>

        <div class="control-section">
          <div class="tabs">
            <button 
              v-for="tab in tabs" 
              :key="tab.id"
              :class="['tab', { active: activeTab === tab.id }]"
              @click="activeTab = tab.id"
            >
              {{ tab.name }}
            </button>
          </div>

          <!-- 时间设置 -->
          <div v-show="activeTab === 'time'" class="tab-content">
            <div class="control-group">
              <label>当前时间</label>
              <input v-model="currentTime" type="time" @input="updatePreview">
            </div>
            
            <div class="control-group">
              <label>字体大小 (1-3)</label>
              <div class="number-input">
                <button @click="adjustValue('timeFontSize', -1)">-</button>
                <input v-model.number="config.time.fontSize" type="number" min="1" max="3" @input="updatePreview">
                <button @click="adjustValue('timeFontSize', 1)">+</button>
              </div>
            </div>
            
            <div class="control-row">
              <div class="control-group">
                <label>X 位置</label>
                <div class="number-input">
                  <button @click="adjustValue('timeX', -1)">-</button>
                  <input v-model.number="config.time.x" type="number" min="0" max="60" @input="updatePreview">
                  <button @click="adjustValue('timeX', 1)">+</button>
                </div>
              </div>
              
              <div class="control-group">
                <label>Y 位置</label>
                <div class="number-input">
                  <button @click="adjustValue('timeY', -1)">-</button>
                  <input v-model.number="config.time.y" type="number" min="0" max="60" @input="updatePreview">
                  <button @click="adjustValue('timeY', 1)">+</button>
                </div>
              </div>
            </div>

            <div class="control-group">
              <label>颜色</label>
              <div class="color-picker">
                <div 
                  v-for="color in presetColors" 
                  :key="color.name"
                  :class="['color-option', { active: isColorActive(config.time, color) }]"
                  :style="{ background: color.hex }"
                  :title="color.name"
                  @click="setColor(config.time, color)"
                ></div>
              </div>
            </div>
          </div>

          <!-- 日期设置 -->
          <div v-show="activeTab === 'date'" class="tab-content">
            <div class="control-group">
              <label class="checkbox-label">
                <input v-model="config.date.show" type="checkbox" @change="updatePreview">
                <span>显示日期</span>
              </label>
            </div>

            <div class="control-group">
              <label>当前日期</label>
              <input v-model="currentDate" type="date" @change="updatePreview">
            </div>
            
            <template v-if="config.date.show">
              <div class="control-group">
                <label>字体大小 (1-2)</label>
                <div class="number-input">
                  <button @click="adjustValue('dateFontSize', -1)">-</button>
                  <input v-model.number="config.date.fontSize" type="number" min="1" max="2" @input="updatePreview">
                  <button @click="adjustValue('dateFontSize', 1)">+</button>
                </div>
              </div>
              
              <div class="control-row">
                <div class="control-group">
                  <label>X 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('dateX', -1)">-</button>
                    <input v-model.number="config.date.x" type="number" min="0" max="60" @input="updatePreview">
                    <button @click="adjustValue('dateX', 1)">+</button>
                  </div>
                </div>
                
                <div class="control-group">
                  <label>Y 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('dateY', -1)">-</button>
                    <input v-model.number="config.date.y" type="number" min="0" max="60" @input="updatePreview">
                    <button @click="adjustValue('dateY', 1)">+</button>
                  </div>
                </div>
              </div>

              <div class="control-group">
                <label>颜色</label>
                <div class="color-picker">
                  <div 
                    v-for="color in presetColors" 
                    :key="color.name"
                    :class="['color-option', { active: isColorActive(config.date, color) }]"
                    :style="{ background: color.hex }"
                    :title="color.name"
                    @click="setColor(config.date, color)"
                  ></div>
                </div>
              </div>
            </template>
          </div>

          <!-- 星期设置 -->
          <div v-show="activeTab === 'week'" class="tab-content">
            <div class="control-group">
              <label class="checkbox-label">
                <input v-model="config.week.show" type="checkbox" @change="updatePreview">
                <span>显示星期</span>
              </label>
            </div>
            
            <template v-if="config.week.show">
              <div class="control-row">
                <div class="control-group">
                  <label>X 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('weekX', -1)">-</button>
                    <input v-model.number="config.week.x" type="number" min="0" max="60" @input="updatePreview">
                    <button @click="adjustValue('weekX', 1)">+</button>
                  </div>
                </div>
                
                <div class="control-group">
                  <label>Y 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('weekY', -1)">-</button>
                    <input v-model.number="config.week.y" type="number" min="0" max="60" @input="updatePreview">
                    <button @click="adjustValue('weekY', 1)">+</button>
                  </div>
                </div>
              </div>

              <div class="control-group">
                <label>颜色</label>
                <div class="color-picker">
                  <div 
                    v-for="color in presetColors" 
                    :key="color.name"
                    :class="['color-option', { active: isColorActive(config.week, color) }]"
                    :style="{ background: color.hex }"
                    :title="color.name"
                    @click="setColor(config.week, color)"
                  ></div>
                </div>
              </div>
            </template>
          </div>

          <!-- 图片设置 -->
          <div v-show="activeTab === 'image'" class="tab-content">
            <div class="control-group">
              <label class="checkbox-label">
                <input v-model="config.image.show" type="checkbox" @change="updatePreview">
                <span>显示图片</span>
              </label>
            </div>

            <div class="upload-area" @click="$refs.fileInput?.click()">
              <div v-if="!uploadedImageData" class="upload-placeholder">
                <div class="upload-icon">+</div>
                <div class="upload-text">点击上传图片</div>
                <div class="upload-hint">建议尺寸 64x64 像素</div>
              </div>
              <div v-else class="upload-preview">
                <img :src="uploadedImageData" alt="上传的图片">
                <div class="upload-actions">
                  <button @click.stop="$refs.fileInput?.click()" class="btn-small">更换</button>
                  <button @click.stop="removeImage" class="btn-small secondary">删除</button>
                </div>
              </div>
            </div>
            <input 
              ref="fileInput" 
              type="file" 
              accept="image/*" 
              style="display: none"
              @change="handleImageUpload"
            >

            <template v-if="uploadedImage">
              <div class="control-group">
                <div class="control-header">
                  <label>尺寸设置</label>
                  <button @click="setSquareSize" class="btn-small">长宽一致</button>
                </div>
              </div>

              <div class="control-group">
                <label>宽度 (最大64)</label>
                <div class="number-input">
                  <button @click="adjustValue('imageWidth', -1)">-</button>
                  <input 
                    v-model.number="config.image.width" 
                    type="number" 
                    min="8" 
                    max="128" 
                    :class="{ warning: checkImageSize('width') }"
                    @input="updatePreview"
                  >
                  <button @click="adjustValue('imageWidth', 1)">+</button>
                </div>
                <div v-if="checkImageSize('width')" class="warning-text">
                  宽度超过64可能显示不完整
                </div>
              </div>

              <div class="control-group">
                <label>高度 (最大64)</label>
                <div class="number-input">
                  <button @click="adjustValue('imageHeight', -1)">-</button>
                  <input 
                    v-model.number="config.image.height" 
                    type="number" 
                    min="8" 
                    max="128" 
                    :class="{ warning: checkImageSize('height') }"
                    @input="updatePreview"
                  >
                  <button @click="adjustValue('imageHeight', 1)">+</button>
                </div>
                <div v-if="checkImageSize('height')" class="warning-text">
                  高度超过64可能显示不完整
                </div>
              </div>

              <div class="control-row">
                <div class="control-group">
                  <label>X 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('imageX', -1)">-</button>
                    <input v-model.number="config.image.x" type="number" min="-32" max="64" @input="updatePreview">
                    <button @click="adjustValue('imageX', 1)">+</button>
                  </div>
                </div>

                <div class="control-group">
                  <label>Y 位置</label>
                  <div class="number-input">
                    <button @click="adjustValue('imageY', -1)">-</button>
                    <input v-model.number="config.image.y" type="number" min="-32" max="64" @input="updatePreview">
                    <button @click="adjustValue('imageY', 1)">+</button>
                  </div>
                </div>
              </div>
            </template>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { clockApi } from '@/api/clock'
import { clearCanvas, drawText, hexToRgb } from '@/utils/ledRenderer'

const canvasRef = ref(null)
const templates = ref([])
const presetColors = ref([])
const activeTab = ref('time')
const currentTime = ref('12:34')
const currentDate = ref('')

const tabs = [
  { id: 'time', name: '时间' },
  { id: 'date', name: '日期' },
  { id: 'week', name: '星期' },
  { id: 'image', name: '图片' }
]

const config = reactive({
  time: {
    fontSize: 1,
    x: 17,
    y: 18,
    r: 100, g: 200, b: 255
  },
  date: {
    show: true,
    fontSize: 1,
    x: 3,
    y: 30,
    r: 120, g: 120, b: 120
  },
  week: {
    show: true,
    x: 23,
    y: 44,
    r: 100, g: 100, b: 100
  },
  image: {
    show: false,
    width: 64,
    height: 64,
    x: 0,
    y: 0
  }
})

const uploadedImage = ref(null)
const uploadedImageData = ref(null)
const fileInput = ref(null)

const updatePreview = () => {
  const canvas = canvasRef.value
  if (!canvas) return
  
  const ctx = canvas.getContext('2d')
  clearCanvas(ctx, 640, 640)
  
  // 绘制图片（如果有）
  if (config.image.show && uploadedImage.value) {
    drawImage(ctx, uploadedImage.value, config.image.x, config.image.y, config.image.width, config.image.height)
  }
  
  // 绘制时间
  const timeText = currentTime.value.substring(0, 5)
  drawText(ctx, timeText, config.time.x, config.time.y, config.time.r, config.time.g, config.time.b, config.time.fontSize)
  
  // 绘制日期
  if (config.date.show && currentDate.value) {
    const date = new Date(currentDate.value)
    const month = String(date.getMonth() + 1).padStart(2, '0')
    const day = String(date.getDate()).padStart(2, '0')
    const dateText = `${month}-${day}`
    drawText(ctx, dateText, config.date.x, config.date.y, config.date.r, config.date.g, config.date.b, config.date.fontSize)
  }
  
  // 绘制星期
  if (config.week.show && currentDate.value) {
    const date = new Date(currentDate.value)
    const weekDays = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']
    const weekText = weekDays[date.getDay()]
    drawText(ctx, weekText, config.week.x, config.week.y, config.week.r, config.week.g, config.week.b, 1)
  }
}

const drawImage = (ctx, img, x, y, width, height) => {
  if (!img) return
  
  const tempCanvas = document.createElement('canvas')
  tempCanvas.width = width
  tempCanvas.height = height
  const tempCtx = tempCanvas.getContext('2d')
  tempCtx.drawImage(img, 0, 0, width, height)
  const imageData = tempCtx.getImageData(0, 0, width, height)
  const pixels = imageData.data
  
  for (let py = 0; py < height; py++) {
    for (let px = 0; px < width; px++) {
      const idx = (py * width + px) * 4
      const r = pixels[idx]
      const g = pixels[idx + 1]
      const b = pixels[idx + 2]
      const a = pixels[idx + 3]
      
      if (a > 128) {
        const finalX = x + px
        const finalY = y + py
        
        if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
          const scale = 10
          ctx.fillStyle = `rgb(${r}, ${g}, ${b})`
          ctx.fillRect(finalX * scale, finalY * scale, scale, scale)
        }
      }
    }
  }
}

const handleImageUpload = (event) => {
  const file = event.target.files[0]
  if (!file) return
  
  const reader = new FileReader()
  reader.onload = (e) => {
    const img = new Image()
    img.onload = () => {
      uploadedImage.value = img
      uploadedImageData.value = e.target.result
      config.image.show = true
      updatePreview()
    }
    img.src = e.target.result
  }
  reader.readAsDataURL(file)
}

const removeImage = () => {
  uploadedImage.value = null
  uploadedImageData.value = null
  config.image.show = false
  if (fileInput.value) {
    fileInput.value.value = ''
  }
  updatePreview()
}

const setSquareSize = () => {
  const size = Math.min(config.image.width, config.image.height, 64)
  config.image.width = size
  config.image.height = size
  updatePreview()
}

const checkImageSize = (dimension) => {
  return config.image[dimension] > 64
}

const isColorActive = (target, color) => {
  return target.r === color.rgb.r && target.g === color.rgb.g && target.b === color.rgb.b
}

const setColor = (target, color) => {
  target.r = color.rgb.r
  target.g = color.rgb.g
  target.b = color.rgb.b
  updatePreview()
}

const adjustValue = (field, delta) => {
  const fieldMap = {
    timeFontSize: () => config.time.fontSize = Math.max(1, Math.min(3, config.time.fontSize + delta)),
    timeX: () => config.time.x = Math.max(0, Math.min(60, config.time.x + delta)),
    timeY: () => config.time.y = Math.max(0, Math.min(60, config.time.y + delta)),
    dateFontSize: () => config.date.fontSize = Math.max(1, Math.min(2, config.date.fontSize + delta)),
    dateX: () => config.date.x = Math.max(0, Math.min(60, config.date.x + delta)),
    dateY: () => config.date.y = Math.max(0, Math.min(60, config.date.y + delta)),
    weekX: () => config.week.x = Math.max(0, Math.min(60, config.week.x + delta)),
    weekY: () => config.week.y = Math.max(0, Math.min(60, config.week.y + delta)),
    imageWidth: () => config.image.width = Math.max(8, Math.min(128, config.image.width + delta)),
    imageHeight: () => config.image.height = Math.max(8, Math.min(128, config.image.height + delta)),
    imageX: () => config.image.x = Math.max(-32, Math.min(64, config.image.x + delta)),
    imageY: () => config.image.y = Math.max(-32, Math.min(64, config.image.y + delta))
  }
  
  if (fieldMap[field]) {
    fieldMap[field]()
    updatePreview()
  }
}

const applyTemplate = (template) => {
  Object.assign(config, JSON.parse(JSON.stringify(template.config)))
  updatePreview()
}

const saveConfig = async () => {
  await clockApi.saveClockConfig(config)
  alert('配置已保存')
}

const exportConfig = () => {
  clockApi.exportConfig(config)
}

onMounted(async () => {
  const now = new Date()
  currentTime.value = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
  currentDate.value = now.toISOString().split('T')[0]
  
  templates.value = await clockApi.getTemplates()
  presetColors.value = await clockApi.getPresetColors()
  
  const savedConfig = await clockApi.loadClockConfig()
  Object.assign(config, savedConfig)
  
  updatePreview()
})
</script>

<style scoped>
.clock-designer {
  padding: 40px 0 80px;
  min-height: calc(100vh - 200px);
}

.container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 20px;
}

.page-title {
  font-size: 42px;
  margin-bottom: 40px;
  text-align: center;
  background: linear-gradient(135deg, #64c8ff 0%, #c864ff 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.designer-layout {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 40px;
}

.preview-section {
  position: sticky;
  top: 90px;
  height: fit-content;
}

.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.preview-header h3 {
  color: #fff;
  font-size: 20px;
}

.preview-actions {
  display: flex;
  gap: 10px;
}

.btn-action {
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 8px;
  padding: 8px 16px;
  color: #fff;
  cursor: pointer;
  transition: all 0.3s;
}

.btn-action:hover {
  background: rgba(255, 255, 255, 0.15);
  transform: translateY(-2px);
}

.canvas-container {
  background: #000;
  padding: 10px;
  border-radius: 12px;
  border: 2px solid #00f3ff;
}

.axis-wrapper {
  display: flex;
  gap: 5px;
}

.y-axis {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  padding-top: 30px;
  padding-bottom: 10px;
  color: #666;
  font-size: 10px;
  text-align: right;
  width: 20px;
}

.x-axis {
  display: flex;
  justify-content: space-between;
  padding-left: 30px;
  padding-right: 10px;
  color: #666;
  font-size: 10px;
  margin-top: 5px;
}

.preview-canvas {
  display: block;
  width: 100%;
  max-width: 640px;
  height: auto;
  image-rendering: pixelated;
}

.templates {
  margin-top: 30px;
}

.templates h4 {
  color: #fff;
  margin-bottom: 15px;
}

.template-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
  gap: 10px;
}

.template-item {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  padding: 15px;
  cursor: pointer;
  transition: all 0.3s;
  text-align: center;
}

.template-item:hover {
  background: rgba(255, 255, 255, 0.1);
  border-color: #64c8ff;
}

.template-preview {
  font-size: 12px;
  color: rgba(255, 255, 255, 0.7);
}

.control-section {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.tabs {
  display: flex;
  gap: 8px;
  background: rgba(255, 255, 255, 0.05);
  padding: 8px;
  border-radius: 8px;
}

.tab {
  flex: 1;
  padding: 12px;
  background: transparent;
  border: none;
  color: rgba(255, 255, 255, 0.6);
  cursor: pointer;
  border-radius: 6px;
  transition: all 0.3s;
}

.tab:hover {
  background: rgba(255, 255, 255, 0.05);
  color: rgba(255, 255, 255, 0.8);
}

.tab.active {
  background: #00f3ff;
  color: #000;
  font-weight: 600;
}

.tab-content {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 24px;
}

.control-group {
  margin-bottom: 20px;
}

.control-group:last-child {
  margin-bottom: 0;
}

.control-group label {
  display: block;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  margin-bottom: 8px;
}

.checkbox-label {
  display: flex;
  align-items: center;
  cursor: pointer;
}

.checkbox-label input {
  margin-right: 8px;
  width: 18px;
  height: 18px;
}

.control-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
}

.number-input {
  display: flex;
  gap: 8px;
  align-items: center;
}

.number-input button {
  width: 40px;
  height: 40px;
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
  color: #00f3ff;
  font-size: 20px;
  font-weight: bold;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.3s;
}

.number-input button:hover {
  background: rgba(255, 255, 255, 0.15);
}

.number-input input {
  flex: 1;
  padding: 10px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: #fff;
  border-radius: 6px;
  text-align: center;
}

input[type="time"],
input[type="date"] {
  width: 100%;
  padding: 10px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: #fff;
  border-radius: 6px;
}

.color-picker {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.color-option {
  width: 36px;
  height: 36px;
  border-radius: 6px;
  cursor: pointer;
  border: 2px solid transparent;
  transition: all 0.3s;
}

.color-option:hover {
  transform: scale(1.1);
}

.color-option.active {
  border-color: #fff;
  box-shadow: 0 0 10px rgba(255, 255, 255, 0.5);
}

.upload-area {
  border: 2px dashed rgba(255, 255, 255, 0.2);
  border-radius: 8px;
  padding: 20px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s;
  margin-bottom: 15px;
}

.upload-area:hover {
  border-color: #00f3ff;
  background: rgba(0, 243, 255, 0.05);
}

.upload-placeholder {
  padding: 20px 0;
}

.upload-icon {
  font-size: 40px;
  color: rgba(255, 255, 255, 0.3);
  margin-bottom: 10px;
}

.upload-text {
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
}

.upload-hint {
  color: rgba(255, 255, 255, 0.4);
  font-size: 12px;
  margin-top: 5px;
}

.upload-preview {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.upload-preview img {
  max-width: 100%;
  max-height: 150px;
  border-radius: 8px;
}

.upload-actions {
  display: flex;
  gap: 8px;
  width: 100%;
}

.btn-small {
  flex: 1;
  padding: 8px;
  background: #00f3ff;
  border: none;
  border-radius: 6px;
  color: #000;
  cursor: pointer;
  font-size: 12px;
  transition: all 0.3s;
}

.btn-small:hover {
  background: #00d4dd;
}

.btn-small.secondary {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
}

.btn-small.secondary:hover {
  background: rgba(255, 255, 255, 0.15);
}

.control-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.control-header label {
  margin-bottom: 0;
}

.warning-text {
  color: #ff6464;
  font-size: 12px;
  margin-top: 5px;
}

.number-input input.warning {
  border-color: #ff6464;
  background: rgba(255, 100, 100, 0.1);
}

@media (max-width: 1024px) {
  .designer-layout {
    grid-template-columns: 1fr;
  }
  
  .preview-section {
    position: static;
  }
}
</style>

<template>
  <div class="export-panel">
    <h3 class="section-title">导出设置</h3>
    
    <!-- 导出选项 -->
    <div class="export-options">
      <label class="checkbox-label">
        <input 
          type="checkbox" 
          v-model="showNumbers"
        />
        <span class="checkmark"></span>
        在图片上显示拼豆编号
      </label>
      
      <label class="checkbox-label">
        <input 
          type="checkbox" 
          v-model="showStats"
          checked
        />
        <span class="checkmark"></span>
        显示拼豆统计数据
      </label>
    </div>
    
    <!-- 拼豆样式 -->
    <div class="style-group">
      <label class="setting-label">拼豆样式</label>
      <div class="style-buttons">
        <button
          v-for="style in beadStyles"
          :key="style.value"
          class="style-btn"
          :class="{ active: selectedStyle === style.value }"
          @click="selectedStyle = style.value"
        >
          {{ style.label }}
        </button>
      </div>
    </div>
    
    <!-- 网格间距 -->
    <div class="spacing-group">
      <label class="setting-label">网格间距</label>
      <div class="spacing-buttons">
        <button
          v-for="spacing in spacingOptions"
          :key="spacing.value"
          class="spacing-btn"
          :class="{ active: selectedSpacing === spacing.value }"
          @click="selectedSpacing = spacing.value"
        >
          {{ spacing.label }}
        </button>
      </div>
    </div>
    
    <!-- 导出按钮 -->
    <div class="export-actions">
      <button 
        class="export-btn primary"
        @click="handleExport('pdf')"
        :disabled="!canExport"
      >
        下载 PDF
      </button>
      
      <button 
        class="export-btn secondary"
        @click="handleExport('png')"
        :disabled="!canExport"
      >
        导出图片
      </button>
    </div>
    
    <!-- 预览信息 -->
    <div v-if="usedColors.size > 0" class="preview-info">
      <h4>预览信息</h4>
      <div class="info-grid">
        <div class="info-item">
          <span class="info-label">尺寸</span>
          <span class="info-value">{{ width }}×{{ height }}</span>
        </div>
        <div class="info-item">
          <span class="info-label">颜色数</span>
          <span class="info-value">{{ usedColors.size }} 种</span>
        </div>
        <div class="info-item">
          <span class="info-label">拼豆数</span>
          <span class="info-value">{{ totalBeads }} 颗</span>
        </div>
        <div class="info-item">
          <span class="info-label">预估时间</span>
          <span class="info-value">{{ estimatedTime }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  pixels: Map,
  width: Number,
  height: Number,
  colors: Array,
  usedColors: Map
})

const emit = defineEmits(['export'])

// 导出设置
const showNumbers = ref(true)
const showStats = ref(true)
const selectedStyle = ref('grid-circle')
const selectedSpacing = ref('small')

// 样式选项
const beadStyles = [
  { value: 'grid-square', label: '网格方块' },
  { value: 'grid-circle', label: '网格圆珠' },
  { value: 'grid-hollow', label: '网格圆中空珠' }
]

const spacingOptions = [
  { value: 'none', label: '无' },
  { value: 'small', label: '小' },
  { value: 'large', label: '大' }
]

// 计算属性
const canExport = computed(() => {
  return props.pixels.size > 0
})

const totalBeads = computed(() => {
  let total = 0
  props.usedColors.forEach(count => {
    total += count
  })
  return total
})

const estimatedTime = computed(() => {
  const beads = totalBeads.value
  if (beads < 100) return '< 30分钟'
  if (beads < 500) return '1-2小时'
  if (beads < 1000) return '2-4小时'
  if (beads < 2000) return '4-8小时'
  return '> 8小时'
})

function handleExport(format) {
  if (!canExport.value) return
  
  const options = {
    format,
    showNumbers: showNumbers.value,
    showStats: showStats.value,
    style: selectedStyle.value,
    spacing: selectedSpacing.value
  }
  
  emit('export', options)
}
</script>

<style scoped>
.export-panel {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  padding: 24px;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.section-title {
  font-size: 18px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 20px;
}

.export-options {
  margin-bottom: 24px;
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  color: #4a5568;
}

.checkbox-label input[type="checkbox"] {
  display: none;
}

.checkmark {
  width: 20px;
  height: 20px;
  border: 2px solid rgba(255, 107, 107, 0.3);
  border-radius: 6px;
  background: white;
  position: relative;
  transition: all 0.3s ease;
}

.checkbox-label input[type="checkbox"]:checked + .checkmark {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
}

.checkbox-label input[type="checkbox"]:checked + .checkmark::after {
  content: '✓';
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  color: white;
  font-size: 12px;
  font-weight: bold;
}

.style-group,
.spacing-group {
  margin-bottom: 24px;
}

.setting-label {
  display: block;
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 12px;
}

.style-buttons,
.spacing-buttons {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.style-btn,
.spacing-btn {
  padding: 10px 16px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 10px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.3s ease;
  text-align: left;
}

.style-btn:hover,
.spacing-btn:hover {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
}

.style-btn.active,
.spacing-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.3);
}

.export-actions {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 24px;
}

.export-btn {
  padding: 16px 24px;
  border: none;
  border-radius: 12px;
  font-size: 16px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.export-btn.primary {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  color: #fff;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.export-btn.primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.4);
}

.export-btn.secondary {
  background: rgba(255, 255, 255, 0.9);
  color: #ff6b6b;
  border: 2px solid rgba(255, 107, 107, 0.3);
}

.export-btn.secondary:hover:not(:disabled) {
  background: white;
  border-color: #ff6b6b;
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.2);
}

.export-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  transform: none;
}

.preview-info {
  padding-top: 20px;
  border-top: 2px solid rgba(255, 107, 107, 0.15);
}

.preview-info h4 {
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 16px;
}

.info-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.info-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.info-label {
  font-size: 12px;
  color: #718096;
  font-weight: 500;
}

.info-value {
  font-size: 14px;
  font-weight: 700;
  color: #ff6b6b;
}
</style>
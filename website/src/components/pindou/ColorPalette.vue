<template>
  <div class="color-palette">
    <h3 class="section-title">
      色系
      <span class="color-count">291 种颜色</span>
    </h3>
    
    <!-- 颜色搜索 -->
    <div class="search-container">
      <input 
        v-model="searchQuery"
        type="text"
        placeholder="搜索颜色编号或名称..."
        class="search-input"
      />
    </div>
    
    <!-- 色系分类 -->
    <div class="series-tabs">
      <button
        v-for="series in colorSeries"
        :key="series.key"
        class="series-tab"
        :class="{ active: selectedSeries === series.key }"
        @click="selectedSeries = series.key"
      >
        {{ series.name }}
        <span class="series-count">{{ series.count }}</span>
      </button>
    </div>
    
    <!-- 颜色网格 -->
    <div class="color-grid" ref="colorGridRef">
      <div
        v-for="color in filteredColors"
        :key="color.code"
        class="color-item"
        :class="{ 
          active: selectedColor === color.code,
          used: usedColors.has(color.code)
        }"
        :style="{ backgroundColor: color.hex }"
        :title="`${color.code} - ${color.name}`"
        @click="selectColor(color.code)"
      >
        <span class="color-code">{{ color.code }}</span>
        <span v-if="usedColors.has(color.code)" class="usage-count">
          {{ usedColors.get(color.code) }}
        </span>
      </div>
    </div>
    
    <!-- 使用统计 -->
    <div v-if="usedColors.size > 0" class="usage-stats">
      <h4>使用统计</h4>
      <div class="stats-summary">
        <div class="stat-item">
          <span class="stat-label">使用颜色</span>
          <span class="stat-value">{{ usedColors.size }} 种</span>
        </div>
        <div class="stat-item">
          <span class="stat-label">总拼豆数</span>
          <span class="stat-value">{{ totalBeads }} 颗</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import { ARTKAL_COLORS_FULL } from '@/data/artkal-colors'

const props = defineProps({
  selectedColor: String,
  colors: Array,
  usedColors: Map
})

const emit = defineEmits(['update:selectedColor'])

const searchQuery = ref('')
const selectedSeries = ref('all')
const colorGridRef = ref(null)

// 色系分类
const colorSeries = [
  { key: 'all', name: '全部', count: 291 },
  { key: 'A', name: '系列 A', count: 26 },
  { key: 'B', name: '系列 B', count: 32 },
  { key: 'C', name: '系列 C', count: 29 },
  { key: 'D', name: '系列 D', count: 26 },
  { key: 'E', name: '系列 E', count: 24 },
  { key: 'F', name: '系列 F', count: 25 },
  { key: 'G', name: '系列 G', count: 21 },
  { key: 'H', name: '系列 H', count: 23 },
  { key: 'M', name: '系列 M', count: 15 },
  { key: 'P', name: '系列 P', count: 23 },
  { key: 'Q', name: '系列 Q', count: 5 },
  { key: 'R', name: '系列 R', count: 28 },
  { key: 'T', name: '系列 T', count: 1 },
  { key: 'Y', name: '系列 Y', count: 5 },
  { key: 'ZG', name: '系列 Z', count: 8 }
]

// 过滤后的颜色
const filteredColors = computed(() => {
  let colors = ARTKAL_COLORS_FULL
  
  // 按系列过滤
  if (selectedSeries.value !== 'all') {
    colors = colors.filter(color => color.group === selectedSeries.value)
  }
  
  // 按搜索关键词过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    colors = colors.filter(color => 
      color.code.toLowerCase().includes(query) ||
      color.name.toLowerCase().includes(query)
    )
  }
  
  return colors
})

// 总拼豆数
const totalBeads = computed(() => {
  let total = 0
  props.usedColors.forEach(count => {
    total += count
  })
  return total
})

function selectColor(colorCode) {
  emit('update:selectedColor', colorCode)
}

// 监听选中颜色变化，滚动到对应位置
watch(() => props.selectedColor, (newColor) => {
  if (newColor && colorGridRef.value) {
    const colorElement = colorGridRef.value.querySelector(`[title*="${newColor}"]`)
    if (colorElement) {
      colorElement.scrollIntoView({ behavior: 'smooth', block: 'nearest' })
    }
  }
})
</script>

<style scoped>
.color-palette {
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  padding: 24px;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
  height: fit-content;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
}

.section-title {
  font-size: 18px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.color-count {
  font-size: 14px;
  font-weight: 500;
  color: #718096;
}

.search-container {
  margin-bottom: 16px;
}

.search-input {
  width: 100%;
  padding: 10px 16px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 12px;
  color: #4a5568;
  font-size: 14px;
  outline: none;
  transition: all 0.3s ease;
}

.search-input:focus {
  border-color: #ff6b6b;
  box-shadow: 0 0 0 3px rgba(255, 107, 107, 0.1);
}

.series-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-bottom: 16px;
  max-height: 120px;
  overflow-y: auto;
}

.series-tab {
  padding: 6px 12px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 8px;
  color: #4a5568;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
  gap: 4px;
}

.series-tab:hover {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
}

.series-tab.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
}

.series-count {
  font-size: 10px;
  opacity: 0.8;
}

.color-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(50px, 1fr));
  gap: 6px;
  max-height: 400px;
  overflow-y: auto;
  padding: 4px;
  flex: 1;
}

.color-item {
  aspect-ratio: 1;
  border-radius: 10px;
  border: 2px solid rgba(255, 107, 107, 0.15);
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  position: relative;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.color-item:hover {
  transform: scale(1.1);
  border-color: #ff6b6b;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
  z-index: 10;
}

.color-item.active {
  border-color: #ff6b6b;
  border-width: 3px;
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.5);
  transform: scale(1.15);
  z-index: 20;
}

.color-item.used {
  border-color: #10b981;
  border-width: 3px;
}

.color-item.used.active {
  border-color: #ff6b6b;
}

.color-code {
  font-size: 9px;
  font-weight: 800;
  color: #2d3748;
  text-shadow: 0 1px 3px rgba(255, 255, 255, 0.9);
  background: rgba(255, 255, 255, 0.85);
  padding: 1px 4px;
  border-radius: 3px;
  line-height: 1;
}

.usage-count {
  position: absolute;
  top: -6px;
  right: -6px;
  background: #10b981;
  color: white;
  font-size: 10px;
  font-weight: 700;
  padding: 2px 6px;
  border-radius: 10px;
  min-width: 18px;
  text-align: center;
  box-shadow: 0 2px 8px rgba(16, 185, 129, 0.3);
}

.usage-stats {
  margin-top: 16px;
  padding-top: 16px;
  border-top: 2px solid rgba(255, 107, 107, 0.15);
}

.usage-stats h4 {
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 12px;
}

.stats-summary {
  display: flex;
  gap: 16px;
}

.stat-item {
  flex: 1;
  text-align: center;
}

.stat-label {
  display: block;
  font-size: 12px;
  color: #718096;
  margin-bottom: 4px;
}

.stat-value {
  font-size: 16px;
  font-weight: 700;
  color: #ff6b6b;
}

/* 滚动条样式 */
.series-tabs::-webkit-scrollbar,
.color-grid::-webkit-scrollbar {
  width: 6px;
}

.series-tabs::-webkit-scrollbar-track,
.color-grid::-webkit-scrollbar-track {
  background: rgba(255, 107, 107, 0.1);
  border-radius: 3px;
}

.series-tabs::-webkit-scrollbar-thumb,
.color-grid::-webkit-scrollbar-thumb {
  background: rgba(255, 107, 107, 0.3);
  border-radius: 3px;
}

.series-tabs::-webkit-scrollbar-thumb:hover,
.color-grid::-webkit-scrollbar-thumb:hover {
  background: rgba(255, 107, 107, 0.5);
}
</style>
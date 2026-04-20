<template>
  <div class="settings-panel">
    <h3 class="section-title">设置</h3>
    
    <!-- 拼豆难度 -->
    <div class="setting-group">
      <label class="setting-label">拼豆难度</label>
      <div class="difficulty-buttons">
        <button
          v-for="difficultyOption in difficulties"
          :key="difficultyOption.value"
          class="difficulty-btn"
          :class="{ active: difficulty === difficultyOption.value }"
          @click="selectDifficulty(difficultyOption.value)"
        >
          <div class="difficulty-label">{{ difficultyOption.label }}</div>
          <div class="difficulty-size">{{ difficultyOption.size }}</div>
        </button>
      </div>
    </div>
    
    <!-- 网格大小设置 -->
    <div class="setting-group">
      <label class="setting-label">网格大小（像素）</label>
      <div class="slider-container">
        <input 
          type="range"
          :value="gridSize"
          @input="$emit('update:gridSize', Number($event.target.value))"
          min="5"
          max="50"
          class="slider"
        />
        <span class="slider-value">{{ gridSize }}PX</span>
      </div>
    </div>
    
    <!-- 单位选择 -->
    <div class="setting-group">
      <label class="setting-label">单位</label>
      <div class="unit-buttons">
        <button
          v-for="unitOption in units"
          :key="unitOption.value"
          class="unit-btn"
          :class="{ active: unit === unitOption.value }"
          @click="$emit('update:unit', unitOption.value)"
        >
          {{ unitOption.label }}
        </button>
      </div>
    </div>
    
    <!-- 拼豆品牌 -->
    <div class="setting-group">
      <label class="setting-label">拼豆品牌</label>
      <select 
        :value="brand"
        @change="$emit('update:brand', $event.target.value)"
        class="brand-select"
      >
        <option value="artkal">Artkal (221色)</option>
        <option value="hama">Hama (基础色)</option>
        <option value="perler">Perler (基础色)</option>
      </select>
    </div>
  </div>
</template>

<script setup>
defineProps({
  gridSize: Number,
  unit: String,
  difficulty: String,
  brand: String
})

defineEmits([
  'update:gridSize',
  'update:unit',
  'update:difficulty',
  'update:brand',
  'difficulty-changed'
])

const units = [
  { value: 'pixel', label: '像素' },
  { value: '5mm', label: '5MM' },
  { value: '2.6mm', label: '2.6MM' }
]

const difficulties = [
  { value: 'simple', label: '简单', size: '30×30' },
  { value: 'medium', label: '中等', size: '50×50' },
  { value: 'hard', label: '困难', size: '80×80' }
]

function selectDifficulty(difficulty) {
  const difficultyMap = {
    'simple': { width: 30, height: 30 },
    'medium': { width: 50, height: 50 },
    'hard': { width: 80, height: 80 }
  }
  
  emit('update:difficulty', difficulty)
  emit('difficulty-changed', difficultyMap[difficulty])
}
</script>

<style scoped>
.settings-panel {
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

.setting-group {
  margin-bottom: 24px;
}

.setting-label {
  display: block;
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 12px;
}

.slider-container {
  display: flex;
  align-items: center;
  gap: 12px;
}

.slider {
  flex: 1;
  height: 6px;
  border-radius: 3px;
  background: rgba(255, 107, 107, 0.2);
  outline: none;
  -webkit-appearance: none;
}

.slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  cursor: pointer;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.3);
}

.slider::-moz-range-thumb {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  cursor: pointer;
  border: none;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.3);
}

.slider-value {
  font-size: 14px;
  font-weight: 600;
  color: #ff6b6b;
  min-width: 60px;
  text-align: right;
}

.unit-buttons {
  display: flex;
  gap: 6px;
}

.difficulty-buttons {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.unit-btn {
  flex: 1;
  padding: 10px 12px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 10px;
  color: #4a5568;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  text-align: center;
}

.difficulty-btn {
  padding: 12px 16px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 10px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.3s ease;
  text-align: left;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.difficulty-label {
  font-weight: 600;
}

.difficulty-size {
  font-size: 12px;
  color: #718096;
  font-family: 'SF Mono', 'Monaco', monospace;
}

.unit-btn:hover,
.difficulty-btn:hover {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
}

.unit-btn.active,
.difficulty-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: var(--nb-ink);
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.3);
}

.brand-select {
  width: 100%;
  padding: 12px 16px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 12px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.3s ease;
  outline: none;
}

.brand-select:focus {
  border-color: #ff6b6b;
  box-shadow: 0 0 0 3px rgba(255, 107, 107, 0.1);
}

.brand-select:hover {
  border-color: rgba(255, 107, 107, 0.3);
}
</style>
<template>
  <div class="tools-panel">
    <h3 class="section-title">工具</h3>
    
    <!-- 基础工具 -->
    <div class="tool-group">
      <button
        class="tool-btn"
        :class="{ active: selectedTool === 'pencil' }"
        @click="$emit('update:selectedTool', 'pencil')"
        title="画笔 - 绘制拼豆"
      >
        画笔
      </button>
      <button
        class="tool-btn"
        :class="{ active: selectedTool === 'eraser' }"
        @click="$emit('update:selectedTool', 'eraser')"
        title="橡皮擦 - 擦除拼豆"
      >
        橡皮擦
      </button>
      <button
        class="tool-btn"
        :class="{ active: selectedTool === 'eyedropper' }"
        @click="$emit('update:selectedTool', 'eyedropper')"
        title="吸管 - 取色"
      >
        吸管
      </button>
    </div>
    
    <!-- 显示选项 -->
    <div class="display-group">
      <label class="checkbox-label">
        <input 
          type="checkbox" 
          :checked="gridVisible"
          @change="$emit('update:gridVisible', $event.target.checked)"
        />
        <span class="checkmark"></span>
        显示网格线
      </label>
      
      <label class="checkbox-label">
        <input 
          type="checkbox" 
          :checked="showNumbers"
          @change="$emit('update:showNumbers', $event.target.checked)"
        />
        <span class="checkmark"></span>
        显示拼豆编号
      </label>
    </div>
    
    <!-- 操作按钮 -->
    <div class="action-group">
      <button
        class="action-btn"
        :disabled="!canUndo"
        @click="$emit('undo')"
        title="撤销"
      >
        ↶ 撤销
      </button>
      <button
        class="action-btn"
        :disabled="!canRedo"
        @click="$emit('redo')"
        title="重做"
      >
        ↷ 重做
      </button>
    </div>
    
    <!-- 高级功能 -->
    <div class="advanced-group">
      <button
        class="action-btn danger"
        @click="$emit('clear-background')"
        title="去背景色"
      >
        去背景色
      </button>
    </div>
    
    <!-- 缩放控制 -->
    <div class="zoom-group">
      <label class="setting-label">缩放控制</label>
      <div class="zoom-controls">
        <button class="zoom-btn" @click="$emit('zoom-out')" title="缩小">-</button>
        <span class="zoom-display">{{ Math.round(zoom) }}%</span>
        <button class="zoom-btn" @click="$emit('zoom-in')" title="放大">+</button>
      </div>
      <button class="fit-btn" @click="$emit('fit-canvas')" title="适应画布">适应</button>
    </div>
  </div>
</template>

<script setup>
defineProps({
  selectedTool: String,
  gridVisible: Boolean,
  showNumbers: Boolean,
  canUndo: Boolean,
  canRedo: Boolean,
  zoom: Number
})

defineEmits([
  'update:selectedTool',
  'update:gridVisible', 
  'update:showNumbers',
  'undo',
  'redo',
  'clear-background',
  'zoom-in',
  'zoom-out',
  'fit-canvas'
])
</script>

<style scoped>
.tools-panel {
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

.tool-group,
.action-group,
.advanced-group {
  display: grid;
  grid-template-columns: 1fr;
  gap: 8px;
  margin-bottom: 20px;
}

.zoom-group {
  margin-bottom: 20px;
}

.zoom-controls {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.zoom-btn {
  width: 32px;
  height: 32px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 8px;
  color: #4a5568;
  font-size: 18px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}

.zoom-btn:hover {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
}

.zoom-display {
  flex: 1;
  text-align: center;
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
}

.fit-btn {
  width: 100%;
  padding: 8px 12px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 8px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
}

.fit-btn:hover {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
}

.setting-label {
  display: block;
  font-size: 14px;
  font-weight: 600;
  color: #4a5568;
  margin-bottom: 8px;
}

.display-group {
  margin-bottom: 20px;
}

.tool-btn,
.action-btn {
  padding: 12px 16px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 12px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
}

.tool-btn:hover:not(:disabled),
.action-btn:hover:not(:disabled) {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.2);
}

.tool-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: var(--nb-ink);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.action-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
  transform: none;
}

.action-btn.danger {
  background: rgba(255, 107, 107, 0.1);
  color: #ff6b6b;
  border-color: rgba(255, 107, 107, 0.3);
}

.action-btn.danger:hover {
  background: rgba(255, 107, 107, 0.2);
  border-color: #ff6b6b;
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
  color: var(--nb-ink);
  font-size: 12px;
  font-weight: bold;
}

.checkbox-label:hover .checkmark {
  border-color: #ff6b6b;
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.2);
}
</style>
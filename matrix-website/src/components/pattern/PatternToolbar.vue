<template>
  <div class="pattern-toolbar">
    <div class="tool-group">
      <button
        class="tool-btn"
        :class="{ active: tool === 'pencil' }"
        @click="$emit('update:tool', 'pencil')"
        title="画笔"
      >
        画笔
      </button>
      <button
        class="tool-btn"
        :class="{ active: tool === 'eraser' }"
        @click="$emit('update:tool', 'eraser')"
        title="橡皮擦"
      >
        橡皮
      </button>
      <button
        class="tool-btn"
        :class="{ active: tool === 'move' }"
        @click="$emit('update:tool', 'move')"
        title="拖动"
      >
        拖动
      </button>
    </div>
    
    <div class="tool-group">
      <button
        class="tool-btn"
        :disabled="!canUndo"
        @click="$emit('undo')"
        title="撤销"
      >
        撤销
      </button>
      <button
        class="tool-btn"
        :disabled="!canRedo"
        @click="$emit('redo')"
        title="重做"
      >
        重做
      </button>
    </div>
    
    <div class="tool-group">
      <button
        class="tool-btn"
        :class="{ active: gridVisible }"
        @click="$emit('update:gridVisible', !gridVisible)"
        title="网格"
      >
        网格
      </button>
      <button
        class="tool-btn"
        @click="$emit('fit')"
        title="适应屏幕"
      >
        适应
      </button>
    </div>
    
    <div class="tool-group">
      <button
        class="tool-btn primary"
        @click="$emit('export')"
        title="导出图纸"
      >
        导出
      </button>
    </div>
  </div>
</template>

<script setup>
defineProps({
  tool: String,
  gridVisible: Boolean,
  canUndo: Boolean,
  canRedo: Boolean
})

defineEmits(['update:tool', 'update:gridVisible', 'undo', 'redo', 'fit', 'export'])
</script>

<style scoped>
.pattern-toolbar {
  display: flex;
  gap: 16px;
  padding: 20px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  flex-wrap: wrap;
  justify-content: center;
  border: 2px solid rgba(255, 107, 107, 0.15);
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.tool-group {
  display: flex;
  gap: 8px;
  padding: 6px;
  background: rgba(255, 107, 107, 0.08);
  border-radius: 12px;
}

.tool-btn {
  padding: 12px 24px;
  background: white;
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 10px;
  color: #4a5568;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 2px 8px rgba(255, 107, 107, 0.08);
}

.tool-btn:hover:not(:disabled) {
  background: #fff5f5;
  color: #ff6b6b;
  border-color: #ff6b6b;
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.2);
}

.tool-btn.active {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.tool-btn.primary {
  background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 100%);
  border-color: #ff6b6b;
  color: #fff;
  font-weight: 700;
  box-shadow: 0 4px 16px rgba(255, 107, 107, 0.3);
}

.tool-btn.primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 24px rgba(255, 107, 107, 0.4);
}

.tool-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
</style>

@media (max-width: 768px) {
  .pattern-toolbar {
    padding: 16px;
    gap: 12px;
  }
  
  .tool-group {
    width: 100%;
    justify-content: center;
  }
  
  .tool-btn {
    padding: 10px 16px;
    font-size: 13px;
  }
}

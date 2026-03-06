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
  padding: 16px;
  background: rgba(0, 0, 0, 0.3);
  border-radius: 12px;
  flex-wrap: wrap;
  justify-content: center;
}

.tool-group {
  display: flex;
  gap: 8px;
  padding: 4px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
}

.tool-btn {
  padding: 10px 20px;
  background: transparent;
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 6px;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.tool-btn:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
}

.tool-btn.active {
  background: rgba(0, 243, 255, 0.2);
  border-color: #00f3ff;
  color: #00f3ff;
}

.tool-btn.primary {
  background: #00f3ff;
  border-color: #00f3ff;
  color: #000;
  font-weight: 600;
}

.tool-btn.primary:hover {
  background: #00d4e0;
}

.tool-btn:disabled {
  opacity: 0.3;
  cursor: not-allowed;
}
</style>

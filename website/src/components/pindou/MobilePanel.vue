<template>
  <div class="mobile-panel-overlay" @click="handleOverlayClick">
    <div class="mobile-panel" @click.stop>
      <div class="panel-header">
        <h3 class="panel-title">{{ panelTitle }}</h3>
        <button class="close-btn" @click="$emit('close')">✕</button>
      </div>
      
      <div class="panel-content">
        <slot />
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  type: String
})

const emit = defineEmits(['close'])

const panelTitle = computed(() => {
  const titles = {
    upload: '上传图片',
    tools: '工具面板',
    colors: '颜色选择',
    export: '导出设置'
  }
  return titles[props.type] || '面板'
})

function handleOverlayClick() {
  emit('close')
}
</script>

<style scoped>
.mobile-panel-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(4px);
  z-index: 2000;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  padding: 0;
}

.mobile-panel {
  background: white;
  border-radius: 20px 20px 0 0;
  width: 100%;
  max-height: 80vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  box-shadow: 0 -8px 32px rgba(0, 0, 0, 0.2);
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20px 24px 16px;
  border-bottom: 2px solid rgba(255, 107, 107, 0.15);
  background: linear-gradient(135deg, rgba(255, 107, 107, 0.05) 0%, rgba(255, 165, 0, 0.05) 100%);
}

.panel-title {
  font-size: 18px;
  font-weight: 700;
  color: #2d3748;
  margin: 0;
}

.close-btn {
  width: 32px;
  height: 32px;
  border: none;
  background: rgba(255, 107, 107, 0.1);
  color: #ff6b6b;
  border-radius: 50%;
  font-size: 16px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}

.close-btn:hover {
  background: rgba(255, 107, 107, 0.2);
  transform: scale(1.1);
}

.panel-content {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
}

/* 移动端滚动条优化 */
.panel-content::-webkit-scrollbar {
  width: 4px;
}

.panel-content::-webkit-scrollbar-track {
  background: rgba(255, 107, 107, 0.1);
}

.panel-content::-webkit-scrollbar-thumb {
  background: rgba(255, 107, 107, 0.3);
  border-radius: 2px;
}

/* 动画效果 */
.mobile-panel-overlay {
  animation: fadeIn 0.3s ease;
}

.mobile-panel {
  animation: slideUp 0.3s ease;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

@keyframes slideUp {
  from {
    transform: translateY(100%);
  }
  to {
    transform: translateY(0);
  }
}
</style>
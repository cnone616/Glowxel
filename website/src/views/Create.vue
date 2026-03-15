<template>
  <div class="create">
    <div class="container">
      <h1 class="page-title">创作</h1>
      <p class="page-desc">选择一种方式开始你的像素创作</p>

      <div class="create-grid">
        <div class="create-card" @click="mode = 'draw'">
          <div class="card-icon">🎨</div>
          <h3>像素绘制</h3>
          <p>在 64×64 画布上自由绘制像素图案</p>
        </div>
        <div class="create-card" @click="mode = 'import'">
          <div class="card-icon">🖼️</div>
          <h3>图片导入</h3>
          <p>上传图片，智能转换为像素图案</p>
        </div>
        <div class="create-card" @click="mode = 'clock'">
          <div class="card-icon">⏰</div>
          <h3>时钟设计</h3>
          <p>自定义时钟显示样式和颜色</p>
        </div>
      </div>

      <div class="editor-area" v-if="mode">
        <div class="editor-toolbar">
          <span class="mode-label">{{ modeLabel }}</span>
          <button class="btn-sm" @click="mode = null">返回选择</button>
        </div>
        <div class="canvas-placeholder">
          <p>编辑器加载中...</p>
          <p class="hint">（像素编辑器将在此区域渲染）</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const mode = ref(null)
const modeLabel = computed(() => {
  const labels = { draw: '像素绘制', import: '图片导入', clock: '时钟设计' }
  return labels[mode.value] || ''
})
</script>

<style scoped>
.container { max-width: 1200px; margin: 0 auto; padding: 0 20px; }
.page-title { font-size: 28px; font-weight: 700; color: #1a1a1a; text-align: center; padding-top: 40px; }
.page-desc { text-align: center; color: #999; font-size: 14px; margin-top: 8px; }

.create-grid {
  display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; margin: 40px 0;
}

.create-card {
  background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px;
  padding: 32px 24px; text-align: center; cursor: pointer; transition: all 0.2s;
}
.create-card:hover { border-color: #ddd; background: #fff; }
.card-icon { font-size: 36px; margin-bottom: 12px; }
.create-card h3 { font-size: 16px; font-weight: 600; color: #1a1a1a; margin-bottom: 8px; }
.create-card p { font-size: 13px; color: #888; }

.editor-area { margin: 20px 0 60px; }
.editor-toolbar {
  display: flex; justify-content: space-between; align-items: center;
  padding: 12px 0; border-bottom: 1px solid #f0f0f0; margin-bottom: 20px;
}
.mode-label { font-size: 16px; font-weight: 600; color: #1a1a1a; }
.btn-sm {
  padding: 6px 16px; border-radius: 6px; border: 1px solid #e0e0e0;
  background: #fff; font-size: 13px; cursor: pointer;
}
.btn-sm:hover { border-color: #1a1a1a; }

.canvas-placeholder {
  background: #fafafa; border: 1px solid #f0f0f0; border-radius: 12px;
  min-height: 400px; display: flex; flex-direction: column;
  align-items: center; justify-content: center;
}
.canvas-placeholder p { color: #999; font-size: 15px; }
.hint { font-size: 13px; color: #ccc; margin-top: 8px; }

@media (max-width: 768px) {
  .create-grid { grid-template-columns: 1fr; }
  .page-title { font-size: 22px; }
}
</style>


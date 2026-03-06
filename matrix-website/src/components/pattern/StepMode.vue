<template>
  <div class="step-mode">
    <h2 class="step-title">选择创作模式</h2>
    <div class="mode-options">
      <div 
        class="mode-card"
        :class="{ active: modelValue === 'blank' }"
        @click="$emit('update:modelValue', 'blank')"
      >
        <div class="mode-icon">画布</div>
        <h3>空白画布</h3>
        <p>从零开始创作您的像素艺术</p>
      </div>
      
      <div 
        class="mode-card"
        :class="{ active: modelValue === 'image' }"
        @click="$emit('update:modelValue', 'image')"
      >
        <div class="mode-icon">图片</div>
        <h3>导入图片</h3>
        <p>上传图片并转换为像素艺术</p>
      </div>
    </div>
  </div>
</template>

<script setup>
defineProps({
  modelValue: {
    type: String,
    required: true
  }
})

defineEmits(['update:modelValue'])
</script>

<style scoped>
.step-mode {
  padding: 40px 0;
}

.step-title {
  font-size: 28px;
  color: #fff;
  margin-bottom: 40px;
  text-align: center;
}

.mode-options {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 24px;
  max-width: 800px;
  margin: 0 auto;
}

.mode-card {
  padding: 40px 30px;
  background: rgba(255, 255, 255, 0.05);
  border: 2px solid rgba(255, 255, 255, 0.1);
  border-radius: 16px;
  cursor: pointer;
  transition: all 0.3s;
  text-align: center;
}

.mode-card:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: rgba(0, 243, 255, 0.3);
  transform: translateY(-4px);
}

.mode-card.active {
  background: rgba(0, 243, 255, 0.1);
  border-color: #00f3ff;
  box-shadow: 0 0 30px rgba(0, 243, 255, 0.2);
}

.mode-icon {
  width: 80px;
  height: 80px;
  margin: 0 auto 20px;
  background: rgba(0, 243, 255, 0.1);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  color: #00f3ff;
  font-weight: 600;
}

.mode-card h3 {
  font-size: 22px;
  color: #fff;
  margin-bottom: 12px;
}

.mode-card p {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.6);
  line-height: 1.6;
}
</style>

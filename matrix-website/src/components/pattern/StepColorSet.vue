<template>
  <div class="step-color-set">
    <h2 class="step-title">选择 Artkal 拼豆套装</h2>
    
    <div class="preset-grid">
      <div
        v-for="preset in presets"
        :key="preset.key"
        class="preset-card"
        :class="{ active: modelValue === preset.key }"
        @click="$emit('update:modelValue', preset.key)"
      >
        <h3>{{ preset.name }}</h3>
        <p>{{ preset.count }} 种颜色</p>
      </div>
    </div>
    
    <div class="info-box">
      <p>套装包含精心挑选的 Artkal 拼豆颜色，适合各种创作需求</p>
    </div>
  </div>
</template>

<script setup>
import { ARTKAL_PRESETS } from '@/data/artkal-colors'

defineProps({
  modelValue: {
    type: String,
    required: true
  }
})

defineEmits(['update:modelValue'])

const presets = ARTKAL_PRESETS
</script>

<style scoped>
.step-color-set {
  padding: 40px 0;
  max-width: 900px;
  margin: 0 auto;
}

.step-title {
  font-size: 28px;
  color: #fff;
  margin-bottom: 40px;
  text-align: center;
}

.preset-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 20px;
  margin-bottom: 32px;
}

.preset-card {
  padding: 32px 24px;
  background: rgba(255, 255, 255, 0.05);
  border: 2px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.3s;
  text-align: center;
}

.preset-card:hover {
  background: rgba(255, 255, 255, 0.08);
  border-color: rgba(0, 243, 255, 0.3);
  transform: translateY(-2px);
}

.preset-card.active {
  background: rgba(0, 243, 255, 0.1);
  border-color: #00f3ff;
  box-shadow: 0 0 20px rgba(0, 243, 255, 0.2);
}

.preset-card h3 {
  font-size: 18px;
  color: #fff;
  margin-bottom: 8px;
  font-weight: 600;
}

.preset-card p {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.6);
}

.info-box {
  padding: 20px;
  background: rgba(0, 243, 255, 0.05);
  border: 1px solid rgba(0, 243, 255, 0.2);
  border-radius: 8px;
  text-align: center;
}

.info-box p {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.7);
  line-height: 1.6;
}
</style>

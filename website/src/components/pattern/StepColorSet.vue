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
  font-size: 32px;
  font-weight: 700;
  color: #2d3748;
  margin-bottom: 48px;
  text-align: center;
}

.preset-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 20px;
  margin-bottom: 40px;
}

.preset-card {
  padding: 36px 28px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 3px solid rgba(255, 107, 107, 0.15);
  border-radius: 20px;
  cursor: pointer;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  text-align: center;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.preset-card:hover {
  background: rgba(255, 255, 255, 0.95);
  border-color: rgba(255, 107, 107, 0.3);
  transform: translateY(-4px);
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.2);
}

.preset-card.active {
  background: linear-gradient(135deg, rgba(255, 107, 107, 0.15) 0%, rgba(255, 165, 0, 0.15) 100%);
  border-color: #ff6b6b;
  box-shadow: 0 8px 32px rgba(255, 107, 107, 0.3);
  transform: translateY(-4px);
}

.preset-card h3 {
  font-size: 20px;
  color: #2d3748;
  margin-bottom: 10px;
  font-weight: 700;
}

.preset-card p {
  font-size: 15px;
  color: #718096;
  font-weight: 500;
}

.info-box {
  padding: 24px;
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px);
  border: 2px solid rgba(255, 107, 107, 0.15);
  border-radius: 16px;
  text-align: center;
  box-shadow: 0 4px 20px rgba(255, 107, 107, 0.1);
}

.info-box p {
  font-size: 15px;
  color: #4a5568;
  line-height: 1.7;
}
</style>

@media (max-width: 768px) {
  .step-color-set {
    padding: 24px 0;
  }
  
  .step-title {
    font-size: 28px;
    margin-bottom: 32px;
  }
  
  .preset-grid {
    grid-template-columns: 1fr;
    gap: 16px;
  }
  
  .preset-card {
    padding: 28px 24px;
  }
  
  .info-box {
    padding: 20px;
  }
  
  .info-box p {
    font-size: 14px;
  }
}

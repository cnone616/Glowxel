<script setup>
defineProps({
  template: {
    type: Object,
    required: true,
  },
})

defineEmits(['create'])
</script>

<template>
  <article class="template-card panel">
    <div class="template-art" :data-template="template.key"></div>
    <div class="template-copy">
      <p class="section-kicker">{{ template.strap }}</p>
      <h3>{{ template.name }}</h3>
      <p class="muted">{{ template.description }}</p>
      <div class="template-meta">
        <span class="chip">{{ template.frameCount }} 张背景序列</span>
        <span class="chip">可继续加对象</span>
      </div>
    </div>
    <button @click="$emit('create')">基于此模板创建</button>
  </article>
</template>

<style scoped>
.template-card {
  overflow: hidden;
  display: grid;
  gap: 18px;
  padding: 18px;
}

.template-art {
  height: 140px;
  border-radius: 16px;
  border: 1px solid rgba(255, 255, 255, 0.06);
  background-size: cover;
  background-position: center;
}

.template-art[data-template="retro-digital"] {
  background:
    linear-gradient(180deg, rgba(255, 191, 105, 0.2), transparent),
    linear-gradient(135deg, #25160f, #140d09);
}

.template-art[data-template="neon-hud"] {
  background:
    radial-gradient(circle at 30% 40%, rgba(0, 245, 212, 0.28), transparent 30%),
    linear-gradient(135deg, #0f1c22, #091014);
}

.template-art[data-template="pixel-pet"] {
  background:
    radial-gradient(circle at 50% 50%, rgba(255, 191, 105, 0.2), transparent 36%),
    linear-gradient(135deg, #22140f, #120d0a);
}

.template-art[data-template="weather-pulse"] {
  background:
    radial-gradient(circle at 70% 30%, rgba(249, 199, 79, 0.2), transparent 25%),
    linear-gradient(135deg, #132330, #0b1014);
}

.template-art[data-template="festive-marquee"] {
  background:
    linear-gradient(90deg, rgba(249, 65, 68, 0.18), rgba(249, 199, 79, 0.12)),
    linear-gradient(135deg, #2a0e13, #13090b);
}

.template-art[data-template="minimal-calendar"] {
  background:
    linear-gradient(135deg, rgba(214, 246, 255, 0.08), rgba(123, 223, 242, 0.1)),
    linear-gradient(135deg, #0b1621, #091014);
}

.template-copy h3 {
  margin: 6px 0 10px;
  font-size: 24px;
}

.template-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 12px;
}
</style>

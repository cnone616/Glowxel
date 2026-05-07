<template>
  <article class="glx-section-card glx-section-card--stack device-params-section">
    <div class="glx-section-head device-params-section__head">
      <div class="device-params-section__copy">
        <h2 class="glx-section-title">{{ title }}</h2>
        <p v-if="description.length > 0" class="device-params-section__desc">{{ description }}</p>
      </div>
      <span v-if="meta.length > 0" class="glx-section-meta">{{ meta }}</span>
    </div>
    <div class="device-params-section__body">
      <slot />
    </div>
  </article>
</template>

<script setup>
defineProps({
  title: {
    type: String,
    required: true,
  },
  meta: {
    type: String,
    default: "",
  },
  description: {
    type: String,
    default: "",
  },
});
</script>

<style scoped>
.device-params-section {
  gap: 18px;
}

.device-params-section__head {
  margin-bottom: 0;
  align-items: flex-start;
}

.device-params-section__copy {
  display: grid;
  gap: 6px;
}

.device-params-section__desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.7;
}

.device-params-section__body {
  display: grid;
  gap: 14px;
}
</style>

<template>
  <div :class="fieldClass">
    <div class="device-params-field__copy">
      <span class="device-params-field__label">{{ label }}</span>
      <span v-if="description.length > 0" class="device-params-field__desc">{{ description }}</span>
    </div>
    <div class="device-params-field__control">
      <slot />
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  label: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    default: "",
  },
  stack: {
    type: Boolean,
    default: false,
  },
});

const fieldClass = computed(() => {
  return {
    "device-params-field": true,
    "device-params-field--stack": props.stack,
  };
});
</script>

<style scoped>
.device-params-field {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(180px, 260px);
  gap: 16px;
  align-items: center;
}

.device-params-field--stack {
  grid-template-columns: minmax(0, 1fr);
}

.device-params-field__copy {
  display: grid;
  gap: 4px;
}

.device-params-field__label {
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 900;
}

.device-params-field__desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.6;
}

.device-params-field__control {
  min-width: 0;
}

@media (max-width: 860px) {
  .device-params-field {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>

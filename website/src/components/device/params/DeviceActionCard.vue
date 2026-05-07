<template>
  <button
    type="button"
    :class="buttonClass"
    :disabled="disabled"
    @click="$emit('trigger')"
  >
    <span class="device-action-card__mark">{{ mark }}</span>
    <span class="device-action-card__copy">
      <span class="device-action-card__title">{{ title }}</span>
      <span class="device-action-card__desc">{{ description }}</span>
    </span>
    <span class="device-action-card__arrow">></span>
  </button>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  title: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    required: true,
  },
  mark: {
    type: String,
    required: true,
  },
  tone: {
    type: String,
    default: "paper",
  },
  disabled: {
    type: Boolean,
    default: false,
  },
});

defineEmits(["trigger"]);

const buttonClass = computed(() => {
  return {
    "device-action-card": true,
    "device-action-card--primary": props.tone === "primary",
    "device-action-card--accent": props.tone === "accent",
    "device-action-card--danger": props.tone === "danger",
  };
});
</script>

<style scoped>
.device-action-card {
  width: 100%;
  display: grid;
  grid-template-columns: 56px minmax(0, 1fr) 16px;
  gap: 14px;
  align-items: center;
  padding: 16px;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  text-align: left;
  box-shadow: none;
  cursor: pointer;
}

.device-action-card--primary {
  background: #fff6d1;
}

.device-action-card--accent {
  background: #dff0ff;
}

.device-action-card--danger {
  background: #ffe0dd;
}

.device-action-card__mark {
  width: 56px;
  height: 56px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: var(--glx-shell-border);
  background: #ffffff;
  color: var(--nb-ink);
  font-size: 16px;
  font-weight: 900;
}

.device-action-card__copy {
  min-width: 0;
  display: grid;
  gap: 4px;
}

.device-action-card__title {
  color: var(--nb-ink);
  font-size: 16px;
  font-weight: 900;
}

.device-action-card__desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.6;
}

.device-action-card__arrow {
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
}

.device-action-card:disabled {
  opacity: 0.48;
  cursor: not-allowed;
}

@media (max-width: 640px) {
  .device-action-card {
    grid-template-columns: 48px minmax(0, 1fr) 16px;
  }

  .device-action-card__mark {
    width: 48px;
    height: 48px;
  }
}
</style>

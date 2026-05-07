<template>
  <div class="device-brightness-field">
    <div class="device-brightness-field__head">
      <div class="device-brightness-field__copy">
        <span class="device-brightness-field__label">{{ label }}</span>
        <span class="device-brightness-field__desc">{{ description }}</span>
      </div>
      <span class="device-brightness-field__value">{{ displayValue }}</span>
    </div>
    <div class="device-brightness-field__controls">
      <input
        :value="displayValue"
        class="device-brightness-field__range"
        type="range"
        :min="min"
        :max="max"
        step="1"
        @input="handleRangeInput"
      />
      <input
        :value="displayValue"
        class="glx-input device-brightness-field__input"
        type="number"
        :min="min"
        :max="max"
        step="1"
        @input="handleNumberInput"
      />
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
    required: true,
  },
  modelValue: {
    type: Number,
    required: true,
  },
  min: {
    type: Number,
    default: 0,
  },
  max: {
    type: Number,
    required: true,
  },
});

const emit = defineEmits(["update:modelValue"]);

const displayValue = computed(() => {
  return sanitizeValue(props.modelValue);
});

function sanitizeValue(rawValue) {
  const numericValue = Number(rawValue);
  if (!Number.isFinite(numericValue)) {
    return props.min;
  }
  if (numericValue < props.min) {
    return props.min;
  }
  if (numericValue > props.max) {
    return props.max;
  }
  return Math.round(numericValue);
}

function updateValue(rawValue) {
  emit("update:modelValue", sanitizeValue(rawValue));
}

function handleRangeInput(event) {
  if (event.target == null) {
    return;
  }
  updateValue(event.target.value);
}

function handleNumberInput(event) {
  if (event.target == null) {
    return;
  }
  updateValue(event.target.value);
}
</script>

<style scoped>
.device-brightness-field {
  display: grid;
  gap: 12px;
  padding: 16px;
  border: var(--glx-shell-border);
  background: #ffffff;
}

.device-brightness-field__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.device-brightness-field__copy {
  display: grid;
  gap: 4px;
}

.device-brightness-field__label {
  color: var(--nb-ink);
  font-size: 15px;
  font-weight: 900;
}

.device-brightness-field__desc {
  color: var(--nb-text-secondary);
  font-size: 13px;
  line-height: 1.6;
}

.device-brightness-field__value {
  min-width: 58px;
  min-height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 0 12px;
  border: var(--glx-shell-border);
  background: var(--nb-yellow);
  color: var(--nb-ink);
  font-size: 18px;
  font-weight: 900;
  flex-shrink: 0;
}

.device-brightness-field__controls {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 92px;
  gap: 12px;
  align-items: center;
}

.device-brightness-field__range {
  width: 100%;
  margin: 0;
  accent-color: #111111;
}

.device-brightness-field__input {
  text-align: center;
  font-weight: 900;
}

@media (max-width: 680px) {
  .device-brightness-field__controls {
    grid-template-columns: minmax(0, 1fr);
  }

  .device-brightness-field__input {
    max-width: 120px;
  }
}
</style>

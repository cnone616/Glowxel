<template>
  <div class="device-mode-stepper">
    <button type="button" class="device-mode-stepper__btn" :disabled="disabled || modelValue <= min" @click="adjust(-step)">-</button>
    <div class="device-mode-stepper__value">{{ modelValue }}</div>
    <button type="button" class="device-mode-stepper__btn" :disabled="disabled || modelValue >= max" @click="adjust(step)">+</button>
  </div>
</template>

<script setup>
const props = defineProps({
  modelValue: {
    type: Number,
    required: true,
  },
  min: {
    type: Number,
    required: true,
  },
  max: {
    type: Number,
    required: true,
  },
  step: {
    type: Number,
    default: 1,
  },
  disabled: {
    type: Boolean,
    default: false,
  },
});

const emit = defineEmits(["update:modelValue"]);

function adjust(delta) {
  const next = Math.max(props.min, Math.min(props.max, props.modelValue + delta));
  emit("update:modelValue", next);
}
</script>

<style scoped>
.device-mode-stepper {
  display: inline-grid;
  grid-template-columns: 44px minmax(52px, auto) 44px;
  gap: 8px;
  align-items: center;
}

.device-mode-stepper__btn,
.device-mode-stepper__value {
  height: 40px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font-weight: 700;
}

.device-mode-stepper__btn {
  cursor: pointer;
}

.device-mode-stepper__btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.device-mode-stepper__value {
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 52px;
}
</style>

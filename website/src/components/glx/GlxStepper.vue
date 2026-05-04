<template>
  <div class="glx-stepper">
    <button type="button" class="glx-stepper__button" :disabled="disabled || currentValue <= min" @click="stepDown">-</button>
    <span class="glx-stepper__value">{{ currentValue }}</span>
    <button type="button" class="glx-stepper__button" :disabled="disabled || currentValue >= max" @click="stepUp">+</button>
  </div>
</template>

<script setup>
import { computed } from "vue";

const props = defineProps({
  value: {
    type: Number,
    required: true,
  },
  min: {
    type: Number,
    default: 0,
  },
  max: {
    type: Number,
    default: 100,
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

const emit = defineEmits(["change"]);

const currentValue = computed(() => {
  return Number.isFinite(props.value) ? props.value : props.min;
});

function clampValue(value) {
  return Math.max(props.min, Math.min(props.max, value));
}

function stepDown() {
  emit("change", clampValue(currentValue.value - props.step));
}

function stepUp() {
  emit("change", clampValue(currentValue.value + props.step));
}
</script>

<style scoped>
.glx-stepper {
  display: inline-flex;
  align-items: center;
  border: 2px solid #111111;
  background: #ffffff;
  box-shadow: 2px 2px 0 #111111;
}

.glx-stepper__button,
.glx-stepper__value {
  min-width: 42px;
  min-height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 15px;
  font-weight: 900;
}

.glx-stepper__button {
  border: 0;
  background: #ffffff;
  cursor: pointer;
}

.glx-stepper__button + .glx-stepper__value,
.glx-stepper__value + .glx-stepper__button {
  border-left: 2px solid #111111;
}

.glx-stepper__button[disabled] {
  cursor: not-allowed;
  opacity: 0.4;
}

.glx-stepper__value {
  min-width: 58px;
  padding: 0 10px;
}
</style>

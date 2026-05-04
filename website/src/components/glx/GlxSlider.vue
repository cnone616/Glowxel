<template>
  <div class="glx-slider">
    <input
      class="glx-slider__input"
      type="range"
      :value="value"
      :min="min"
      :max="max"
      :step="step"
      :disabled="disabled"
      @input="handleInput"
    />
    <span class="glx-slider__value">{{ value }}</span>
  </div>
</template>

<script setup>
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

function handleInput(event) {
  emit("change", Number(event.target.value));
}
</script>

<style scoped>
.glx-slider {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
}

.glx-slider__input {
  width: 100%;
}

.glx-slider__value {
  min-width: 48px;
  min-height: 32px;
  padding: 0 10px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 2px solid #111111;
  background: #ffffff;
  font-size: 12px;
  font-weight: 900;
}
</style>

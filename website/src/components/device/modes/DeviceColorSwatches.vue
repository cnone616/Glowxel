<template>
  <div class="device-color-swatches">
    <button
      v-for="item in items"
      :key="item.value"
      type="button"
      class="device-color-swatches__item"
      :class="{ 'device-color-swatches__item--active': modelValue === item.value }"
      @click="$emit('update:modelValue', item.value)"
    >
      <span class="device-color-swatches__dot" :style="{ backgroundColor: item.value }"></span>
      <span>{{ item.label }}</span>
    </button>
  </div>
</template>

<script setup>
defineProps({
  modelValue: {
    type: String,
    required: true,
  },
  items: {
    type: Array,
    required: true,
  },
});

defineEmits(["update:modelValue"]);
</script>

<style scoped>
.device-color-swatches {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 10px;
}

.device-color-swatches__item {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  min-height: 42px;
  padding: 8px 10px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font-weight: 700;
  cursor: pointer;
  text-align: left;
}

.device-color-swatches__item--active {
  background: #ffd23f;
}

.device-color-swatches__dot {
  width: 18px;
  height: 18px;
  border: 2px solid #000000;
  flex-shrink: 0;
}
</style>

<template>
  <div class="device-mode-tabs">
    <button
      v-for="item in items"
      :key="item.value"
      type="button"
      class="device-mode-tabs__item"
      :class="{ 'device-mode-tabs__item--active': modelValue === item.value }"
      @click="$emit('update:modelValue', item.value)"
    >
      {{ item.label }}
    </button>
  </div>
</template>

<script setup>
defineProps({
  modelValue: {
    type: [String, Number],
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
.device-mode-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.device-mode-tabs__item {
  min-width: 92px;
  min-height: 42px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font-weight: 700;
  cursor: pointer;
}

.device-mode-tabs__item--active {
  background: #ffd23f;
}
</style>

<template>
  <div class="ds-tabs">
    <button
      v-for="tab in tabs"
      :key="tab.key"
      type="button"
      class="ds-tab"
      :class="{ 'is-active': modelValue === tab.key }"
      @click="$emit('update:modelValue', tab.key)"
    >
      {{ tab.label }}
    </button>
  </div>
</template>

<script setup>
defineProps({
  tabs: {
    type: Array,
    required: true,
  },
  modelValue: {
    type: String,
    required: true,
  },
});

defineEmits(["update:modelValue"]);
</script>

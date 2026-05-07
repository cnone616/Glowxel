<template>
  <button
    type="button"
    class="glx-switch"
    :class="{ 'is-checked': checked }"
    :disabled="disabled"
    @click="toggle"
  >
    <span class="glx-switch__thumb"></span>
  </button>
</template>

<script setup>
const props = defineProps({
  checked: {
    type: Boolean,
    default: false,
  },
  disabled: {
    type: Boolean,
    default: false,
  },
});

const emit = defineEmits(["change"]);

function toggle() {
  if (props.disabled) {
    return;
  }
  emit("change", !props.checked);
}
</script>

<style scoped>
.glx-switch {
  width: 64px;
  min-width: 64px;
  height: 36px;
  padding: 2px;
  display: inline-flex;
  align-items: center;
  border: 2px solid #111111;
  background: #ffffff;
  box-shadow: 2px 2px 0 #111111;
  cursor: pointer;
}

.glx-switch.is-checked {
  justify-content: flex-end;
  background: #ffcb45;
}

.glx-switch__thumb {
  width: 26px;
  height: 26px;
  border: 2px solid #111111;
  background: #111111;
}

.glx-switch[disabled] {
  cursor: not-allowed;
  opacity: 0.45;
  box-shadow: none;
}
</style>

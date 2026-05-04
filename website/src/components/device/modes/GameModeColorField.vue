<template>
  <div class="game-mode-color-field">
    <div class="game-mode-color-field__head">
      <span class="game-mode-color-field__label">{{ label }}</span>
      <div class="game-mode-color-field__value">
        <input
          class="game-mode-color-field__picker"
          type="color"
          :value="modelValue"
          :disabled="disabled"
          @input="handleInput"
        />
        <span class="game-mode-color-field__hex">{{ modelValue }}</span>
      </div>
    </div>

    <div class="game-mode-color-field__swatches">
      <button
        v-for="item in normalizedPresetColors"
        :key="item.hex"
        type="button"
        class="game-mode-color-field__swatch"
        :class="{ 'is-active': modelValue === item.hex }"
        :title="item.name"
        :disabled="disabled"
        :style="{ backgroundColor: item.hex }"
        @click="$emit('update:modelValue', item.hex)"
      />
    </div>
  </div>
</template>

<script setup>
import { computed } from "vue";
import { normalizeHexColor } from "@/utils/device-mode-core.js";

const props = defineProps({
  label: {
    type: String,
    required: true,
  },
  modelValue: {
    type: String,
    required: true,
  },
  presetColors: {
    type: Array,
    required: true,
  },
  disabled: {
    type: Boolean,
    default: false,
  },
});

const emit = defineEmits(["update:modelValue"]);

const normalizedPresetColors = computed(() => {
  return props.presetColors.map((item) => {
    if (typeof item === "string") {
      return {
        name: item,
        hex: item,
      };
    }
    return {
      name: item.name,
      hex: item.hex,
    };
  });
});

function handleInput(event) {
  emit("update:modelValue", normalizeHexColor(event.target.value));
}
</script>

<style scoped>
.game-mode-color-field {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.game-mode-color-field__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.game-mode-color-field__label {
  font-size: 14px;
  font-weight: 800;
  color: #000000;
}

.game-mode-color-field__value {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.game-mode-color-field__picker {
  width: 50px;
  height: 34px;
  padding: 0;
  border: 2px solid #000000;
  background: #ffffff;
  cursor: pointer;
}

.game-mode-color-field__hex {
  min-width: 84px;
  min-height: 34px;
  padding: 0 10px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 2px solid #000000;
  background: #ffffff;
  font-size: 12px;
  font-weight: 800;
  text-transform: lowercase;
}

.game-mode-color-field__swatches {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.game-mode-color-field__swatch {
  width: 34px;
  height: 34px;
  border: 2px solid #000000;
  cursor: pointer;
}

.game-mode-color-field__swatch.is-active {
  outline: 3px solid #000000;
  outline-offset: 2px;
}

.game-mode-color-field__swatch:disabled,
.game-mode-color-field__picker:disabled {
  cursor: not-allowed;
  opacity: 0.45;
}

@media (max-width: 720px) {
  .game-mode-color-field__head {
    flex-direction: column;
    align-items: stretch;
  }

  .game-mode-color-field__value {
    justify-content: space-between;
  }
}
</style>

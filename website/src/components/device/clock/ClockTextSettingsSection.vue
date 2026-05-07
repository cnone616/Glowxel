<template>
  <section class="clock-section">
    <div class="clock-section__head">
      <div>
        <h3 class="clock-section__title">{{ title }}</h3>
        <p class="clock-section__meta">{{ description }}</p>
      </div>
      <button type="button" class="clock-toggle" :class="{ 'is-on': section.show }" @click="$emit('toggle')">
        {{ section.show ? "显示中" : "已隐藏" }}
      </button>
    </div>

    <div v-if="showSecondsControl" class="clock-row">
      <span class="clock-row__label">显示秒钟</span>
      <button type="button" class="glx-button glx-button--ghost" @click="$emit('toggle-seconds')">
        {{ showSeconds ? "开启" : "关闭" }}
      </button>
    </div>

    <div v-if="showFontSize" class="clock-row">
      <span class="clock-row__label">字体大小</span>
      <div class="clock-stepper">
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'fontSize', -1, minFontSize, maxFontSize)">-</button>
        <strong class="clock-stepper__value">{{ section.fontSize }}</strong>
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'fontSize', 1, minFontSize, maxFontSize)">+</button>
      </div>
    </div>

    <div class="clock-row">
      <span class="clock-row__label">X 位置</span>
      <div class="clock-stepper">
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'x', -1, 0, 64)">-</button>
        <strong class="clock-stepper__value">{{ section.x }}</strong>
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'x', 1, 0, 64)">+</button>
      </div>
    </div>

    <div class="clock-row">
      <span class="clock-row__label">Y 位置</span>
      <div class="clock-stepper">
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'y', -1, 0, 64)">-</button>
        <strong class="clock-stepper__value">{{ section.y }}</strong>
        <button type="button" class="clock-stepper__button" @click="$emit('adjust', 'y', 1, 0, 64)">+</button>
      </div>
    </div>

    <div class="clock-block">
      <span class="clock-row__label">对齐方式</span>
      <div class="glx-tabs">
        <button
          type="button"
          class="glx-tab"
          :class="{ 'is-active': section.align === 'left' }"
          @click="$emit('set-align', 'left')"
        >
          左对齐
        </button>
        <button
          type="button"
          class="glx-tab"
          :class="{ 'is-active': section.align === 'center' }"
          @click="$emit('set-align', 'center')"
        >
          居中
        </button>
        <button
          type="button"
          class="glx-tab"
          :class="{ 'is-active': section.align === 'right' }"
          @click="$emit('set-align', 'right')"
        >
          右对齐
        </button>
      </div>
    </div>

    <div class="clock-block">
      <div class="clock-block__head">
        <span class="clock-row__label">颜色</span>
        <input
          class="clock-color-input"
          type="color"
          :value="section.color"
          @input="$emit('update-color', $event.target.value)"
        />
      </div>
      <div class="clock-color-palette">
        <button
          v-for="color in presetColors"
          :key="color.hex"
          type="button"
          class="clock-color-swatch"
          :title="color.name"
          :style="{ backgroundColor: color.hex }"
          :class="{ 'is-active': section.color === color.hex }"
          @click="$emit('update-color', color.hex)"
        />
      </div>
    </div>
  </section>
</template>

<script setup>
defineProps({
  title: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    required: true,
  },
  section: {
    type: Object,
    required: true,
  },
  presetColors: {
    type: Array,
    required: true,
  },
  showFontSize: {
    type: Boolean,
    default: true,
  },
  minFontSize: {
    type: Number,
    default: 1,
  },
  maxFontSize: {
    type: Number,
    default: 3,
  },
  showSecondsControl: {
    type: Boolean,
    default: false,
  },
  showSeconds: {
    type: Boolean,
    default: false,
  },
});

defineEmits(["toggle", "toggle-seconds", "adjust", "set-align", "update-color"]);
</script>

<style scoped>
.clock-section {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.clock-section__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.clock-section__title {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
}

.clock-section__meta {
  margin: 6px 0 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.5;
}

.clock-toggle {
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font: inherit;
  font-weight: 700;
  padding: 10px 14px;
  cursor: pointer;
  white-space: nowrap;
}

.clock-toggle.is-on {
  background: #facc15;
}

.clock-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.clock-row__label {
  font-size: 14px;
  font-weight: 700;
}

.clock-stepper {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.clock-stepper__button {
  width: 40px;
  height: 40px;
  border: 2px solid #000000;
  background: #ffffff;
  font: inherit;
  font-size: 22px;
  font-weight: 800;
  cursor: pointer;
}

.clock-stepper__value {
  min-width: 42px;
  text-align: center;
  font-size: 20px;
}

.clock-block {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.clock-block__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.clock-color-input {
  width: 50px;
  height: 34px;
  border: 2px solid #000000;
  background: transparent;
  padding: 0;
  cursor: pointer;
}

.clock-color-palette {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.clock-color-swatch {
  width: 34px;
  height: 34px;
  border: 2px solid #000000;
  cursor: pointer;
}

.clock-color-swatch.is-active {
  outline: 3px solid #000000;
  outline-offset: 2px;
}

@media (max-width: 720px) {
  .clock-section__head,
  .clock-row,
  .clock-block__head {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>

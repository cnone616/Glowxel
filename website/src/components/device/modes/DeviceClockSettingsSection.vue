<template>
  <section class="mode-clock-section">
    <div class="mode-clock-section__head">
      <div>
        <h3 class="mode-clock-section__title">{{ title }}</h3>
        <p class="mode-clock-section__meta">{{ description }}</p>
      </div>
      <button
        v-if="showToggle"
        type="button"
        class="mode-clock-toggle"
        :class="{ 'is-on': section.show }"
        @click="$emit('toggle')"
      >
        {{ section.show ? "显示中" : "已隐藏" }}
      </button>
    </div>

    <div v-if="showSecondsControl" class="mode-clock-row">
      <span class="mode-clock-row__label">显示秒钟</span>
      <button type="button" class="glx-button glx-button--ghost" @click="$emit('toggle-seconds')">
        {{ showSeconds ? "开启" : "关闭" }}
      </button>
    </div>

    <div v-if="showFontSize" class="mode-clock-row">
      <span class="mode-clock-row__label">字体大小</span>
      <div class="mode-clock-stepper">
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'fontSize', -1, minFontSize, maxFontSize)"
        >
          -
        </button>
        <strong class="mode-clock-stepper__value">{{ section.fontSize }}</strong>
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'fontSize', 1, minFontSize, maxFontSize)"
        >
          +
        </button>
      </div>
    </div>

    <div class="mode-clock-row">
      <span class="mode-clock-row__label">X 位置</span>
      <div class="mode-clock-stepper">
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'x', -1, 0, 64)"
        >
          -
        </button>
        <strong class="mode-clock-stepper__value">{{ section.x }}</strong>
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'x', 1, 0, 64)"
        >
          +
        </button>
      </div>
    </div>

    <div class="mode-clock-row">
      <span class="mode-clock-row__label">Y 位置</span>
      <div class="mode-clock-stepper">
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'y', -1, 0, 64)"
        >
          -
        </button>
        <strong class="mode-clock-stepper__value">{{ section.y }}</strong>
        <button
          type="button"
          class="mode-clock-stepper__button"
          @click="$emit('adjust', 'y', 1, 0, 64)"
        >
          +
        </button>
      </div>
    </div>

    <div class="mode-clock-block">
      <span class="mode-clock-row__label">对齐方式</span>
      <div class="mode-clock-tabs">
        <button
          type="button"
          class="mode-clock-tab"
          :class="{ 'is-active': section.align === 'left' }"
          @click="$emit('set-align', 'left')"
        >
          左对齐
        </button>
        <button
          type="button"
          class="mode-clock-tab"
          :class="{ 'is-active': section.align === 'center' }"
          @click="$emit('set-align', 'center')"
        >
          居中
        </button>
        <button
          type="button"
          class="mode-clock-tab"
          :class="{ 'is-active': section.align === 'right' }"
          @click="$emit('set-align', 'right')"
        >
          右对齐
        </button>
      </div>
    </div>

    <div class="mode-clock-block">
      <div class="mode-clock-block__head">
        <span class="mode-clock-row__label">颜色</span>
        <input
          class="mode-clock-color-input"
          type="color"
          :value="section.color"
          @input="$emit('update-color', $event.target.value)"
        />
      </div>
      <div class="mode-clock-palette">
        <button
          v-for="color in presetColors"
          :key="color.hex"
          type="button"
          class="mode-clock-swatch"
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
  showToggle: {
    type: Boolean,
    default: true,
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
.mode-clock-section {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.mode-clock-section__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.mode-clock-section__title {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
}

.mode-clock-section__meta {
  margin: 6px 0 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.5;
}

.mode-clock-toggle {
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font: inherit;
  font-weight: 700;
  padding: 10px 14px;
  cursor: pointer;
  white-space: nowrap;
}

.mode-clock-toggle.is-on {
  background: #facc15;
}

.mode-clock-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.mode-clock-row__label {
  font-size: 14px;
  font-weight: 700;
}

.mode-clock-stepper {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.mode-clock-stepper__button {
  width: 40px;
  height: 40px;
  border: 2px solid #000000;
  background: #ffffff;
  font: inherit;
  font-size: 22px;
  font-weight: 800;
  cursor: pointer;
}

.mode-clock-stepper__value {
  min-width: 42px;
  text-align: center;
  font-size: 20px;
}

.mode-clock-block {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.mode-clock-block__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.mode-clock-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.mode-clock-tab {
  min-width: 92px;
  min-height: 42px;
  border: 2px solid #000000;
  background: #ffffff;
  color: #000000;
  font-weight: 700;
  cursor: pointer;
}

.mode-clock-tab.is-active {
  background: #ffd23f;
}

.mode-clock-color-input {
  width: 44px;
  height: 44px;
  padding: 0;
  border: 2px solid #000000;
  background: #ffffff;
  cursor: pointer;
}

.mode-clock-palette {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(44px, 1fr));
  gap: 10px;
}

.mode-clock-swatch {
  width: 100%;
  min-height: 40px;
  border: 2px solid #000000;
  cursor: pointer;
}

.mode-clock-swatch.is-active {
  box-shadow: inset 0 0 0 3px #ffffff;
}

@media (max-width: 640px) {
  .mode-clock-section__head,
  .mode-clock-row,
  .mode-clock-block__head {
    flex-direction: column;
    align-items: stretch;
  }

  .mode-clock-stepper,
  .mode-clock-tabs {
    width: 100%;
  }

  .mode-clock-tab {
    flex: 1 1 0;
  }
}
</style>

<template>
  <section class="game-mode-font-section">
    <div class="game-mode-font-section__head">
      <div>
        <h3 class="game-mode-font-section__title">{{ title }}</h3>
        <p v-if="description.length > 0" class="game-mode-font-section__meta">{{ description }}</p>
      </div>
    </div>

    <div class="game-mode-font-grid">
      <button
        v-for="font in fontOptions"
        :key="font.id"
        type="button"
        class="game-mode-font-card"
        :class="{ 'is-active': selectedFont === font.id }"
        @click="$emit('select-font', font.id)"
      >
        <div class="game-mode-font-card__preview">
          <ClockPixelCanvas :frame="previewFrames[font.id]" />
        </div>
        <strong class="game-mode-font-card__name">{{ font.name }}</strong>
      </button>
    </div>

    <div class="game-mode-font-controls" :class="{ 'game-mode-font-controls--single': showHourFormat === false }">
      <div class="game-mode-font-box">
        <span class="game-mode-font-box__label">秒钟</span>
        <div class="glx-tabs">
          <button
            type="button"
            class="glx-tab"
            :class="{ 'is-active': showSeconds === false }"
            @click="$emit('set-show-seconds', false)"
          >
            关闭
          </button>
          <button
            type="button"
            class="glx-tab"
            :class="{ 'is-active': showSeconds === true }"
            @click="$emit('set-show-seconds', true)"
          >
            显示
          </button>
        </div>
      </div>

      <div v-if="showHourFormat" class="game-mode-font-box">
        <span class="game-mode-font-box__label">小时制式</span>
        <div class="glx-tabs">
          <button
            type="button"
            class="glx-tab"
            :class="{ 'is-active': hourFormat === 24 }"
            @click="$emit('set-hour-format', 24)"
          >
            24 小时
          </button>
          <button
            type="button"
            class="glx-tab"
            :class="{ 'is-active': hourFormat === 12 }"
            @click="$emit('set-hour-format', 12)"
          >
            12 小时
          </button>
        </div>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed } from "vue";
import ClockPixelCanvas from "@/components/device/clock/ClockPixelCanvas.vue";
import { renderFontPreviewFrame } from "@/utils/device-clock-core.js";

const props = defineProps({
  title: {
    type: String,
    default: "字体样式",
  },
  description: {
    type: String,
    default: "",
  },
  fontOptions: {
    type: Array,
    required: true,
  },
  selectedFont: {
    type: String,
    required: true,
  },
  showSeconds: {
    type: Boolean,
    required: true,
  },
  hourFormat: {
    type: Number,
    default: 24,
  },
  showHourFormat: {
    type: Boolean,
    default: true,
  },
});

defineEmits(["select-font", "set-show-seconds", "set-hour-format"]);

const previewFrames = computed(() => {
  return props.fontOptions.reduce((accumulator, font) => {
    accumulator[font.id] = renderFontPreviewFrame(font.id, props.showSeconds, props.hourFormat);
    return accumulator;
  }, {});
});
</script>

<style scoped>
.game-mode-font-section {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.game-mode-font-section__title {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
}

.game-mode-font-section__meta {
  margin: 6px 0 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.5;
}

.game-mode-font-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

.game-mode-font-card {
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px;
  border: 2px solid #000000;
  background: #ffffff;
  cursor: pointer;
  text-align: left;
}

.game-mode-font-card.is-active {
  background: #ffd23f;
}

.game-mode-font-card__preview {
  overflow: hidden;
  border: 2px solid #000000;
  background: #000000;
}

.game-mode-font-card__name {
  font-size: 13px;
  line-height: 1.4;
}

.game-mode-font-controls {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.game-mode-font-controls--single {
  grid-template-columns: minmax(0, 1fr);
}

.game-mode-font-box {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.game-mode-font-box__label {
  font-size: 14px;
  font-weight: 700;
}

@media (max-width: 900px) {
  .game-mode-font-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .game-mode-font-grid,
  .game-mode-font-controls {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>

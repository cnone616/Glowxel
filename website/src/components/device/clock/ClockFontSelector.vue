<template>
  <section class="clock-section">
    <div class="clock-section__head">
      <div>
        <h3 class="clock-section__title">字体样式</h3>
        <p class="clock-section__meta">沿用 uniapp 的时钟字模，网站端预览和设备端落字保持同一套字宽逻辑。</p>
      </div>
    </div>

    <div class="clock-font-grid">
      <button
        v-for="font in fontOptions"
        :key="font.id"
        type="button"
        class="clock-font-card"
        :class="{ 'is-active': selectedFont === font.id }"
        @click="$emit('select-font', font.id)"
      >
        <div class="clock-font-card__preview">
          <ClockPixelCanvas :frame="previewFrames[font.id]" />
        </div>
        <strong class="clock-font-card__name">{{ font.name }}</strong>
      </button>
    </div>

    <div class="clock-setting-pair">
      <div class="clock-setting-box">
        <span class="clock-row__label">秒钟</span>
        <div class="glx-tabs">
          <button type="button" class="glx-tab" :class="{ 'is-active': showSeconds === false }" @click="$emit('set-show-seconds', false)">关闭</button>
          <button type="button" class="glx-tab" :class="{ 'is-active': showSeconds === true }" @click="$emit('set-show-seconds', true)">显示</button>
        </div>
      </div>

      <div class="clock-setting-box">
        <span class="clock-row__label">小时制式</span>
        <div class="glx-tabs">
          <button type="button" class="glx-tab" :class="{ 'is-active': hourFormat === 24 }" @click="$emit('set-hour-format', 24)">24 小时</button>
          <button type="button" class="glx-tab" :class="{ 'is-active': hourFormat === 12 }" @click="$emit('set-hour-format', 12)">12 小时</button>
        </div>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed } from "vue";
import ClockPixelCanvas from "./ClockPixelCanvas.vue";
import { renderFontPreviewFrame } from "@/utils/device-clock-core.js";

const props = defineProps({
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
    required: true,
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
.clock-section {
  display: flex;
  flex-direction: column;
  gap: 18px;
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

.clock-font-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

.clock-font-card {
  display: flex;
  flex-direction: column;
  gap: 12px;
  border: 2px solid #000000;
  background: #ffffff;
  padding: 12px;
  cursor: pointer;
  text-align: left;
}

.clock-font-card.is-active {
  background: #facc15;
}

.clock-font-card__preview {
  border: 2px solid #000000;
  background: #000000;
  overflow: hidden;
}

.clock-font-card__name {
  font-size: 13px;
  line-height: 1.4;
}

.clock-setting-pair {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.clock-setting-box {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.clock-row__label {
  font-size: 14px;
  font-weight: 700;
}

@media (max-width: 900px) {
  .clock-font-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 640px) {
  .clock-font-grid,
  .clock-setting-pair {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>

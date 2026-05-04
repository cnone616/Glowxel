<template>
  <section class="clock-theme-grid">
    <button
      v-for="preset in presets"
      :key="preset.id"
      type="button"
      class="clock-theme-card"
      :class="{
        'is-active': selectedThemeId === preset.id,
        'is-current': currentThemeId === preset.id,
      }"
      @click="$emit('select-theme', preset.id)"
    >
      <div class="clock-theme-card__preview">
        <img
          v-if="typeof preset.previewImage === 'string' && preset.previewImage.length > 0"
          :src="preset.previewImage"
          :alt="preset.name"
          class="clock-theme-card__preview-image"
        />
        <ClockPixelCanvas v-else :frame="previewFrames[preset.id]" />
      </div>
      <div class="clock-theme-card__copy">
        <div class="clock-theme-card__topline">
          <strong>{{ preset.name }}</strong>
          <div class="clock-theme-card__tag-row">
            <span v-if="currentThemeId === preset.id" class="clock-theme-card__badge">当前</span>
            <span class="clock-theme-card__tag">{{ preset.styleTag }}</span>
          </div>
        </div>
        <p>{{ preset.description }}</p>
      </div>
    </button>
  </section>
</template>

<script setup>
import { computed } from "vue";
import ClockPixelCanvas from "./ClockPixelCanvas.vue";
import { renderDeviceClockFrame } from "@/utils/device-clock-core.js";

const props = defineProps({
  presets: {
    type: Array,
    required: true,
  },
  selectedThemeId: {
    type: String,
    required: true,
  },
  currentThemeId: {
    type: String,
    default: "",
  },
});

defineEmits(["select-theme"]);

const previewFrames = computed(() => {
  return props.presets.reduce((accumulator, preset) => {
    accumulator[preset.id] = renderDeviceClockFrame({
      config: preset.config,
      themeId: preset.id,
      now: new Date("2024-01-01T12:34:56"),
    });
    return accumulator;
  }, {});
});
</script>

<style scoped>
.clock-theme-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.clock-theme-card {
  display: grid;
  grid-template-columns: 180px minmax(0, 1fr);
  gap: 16px;
  padding: 14px;
  border: 2px solid #000000;
  background: #ffffff;
  text-align: left;
  cursor: pointer;
}

.clock-theme-card.is-active {
  background: #facc15;
}

.clock-theme-card.is-current {
  box-shadow: 4px 4px 0 #000000;
}

.clock-theme-card__preview {
  border: 2px solid #000000;
  background: #000000;
  overflow: hidden;
}

.clock-theme-card__preview-image {
  width: 100%;
  aspect-ratio: 1;
  display: block;
  object-fit: contain;
  background: #000000;
  image-rendering: pixelated;
}

.clock-theme-card__copy {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.clock-theme-card__copy p {
  margin: 0;
  color: var(--glx-text-muted);
  font-size: 13px;
  line-height: 1.5;
}

.clock-theme-card__topline {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  align-items: center;
}

.clock-theme-card__tag-row {
  display: inline-flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.clock-theme-card__badge {
  display: inline-flex;
  padding: 4px 8px;
  border: 2px solid #000000;
  background: #facc15;
  font-size: 12px;
  font-weight: 900;
}

.clock-theme-card__tag {
  display: inline-flex;
  padding: 4px 8px;
  border: 2px solid #000000;
  background: #ffffff;
  font-size: 12px;
  font-weight: 700;
}

@media (max-width: 1080px) {
  .clock-theme-grid {
    grid-template-columns: minmax(0, 1fr);
  }
}

@media (max-width: 640px) {
  .clock-theme-card {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>

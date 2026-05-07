<template>
  <section class="clock-section">
    <div class="clock-section__head">
      <div>
        <h3 class="clock-section__title">{{ title }}</h3>
        <p class="clock-section__meta">{{ description }}</p>
      </div>
      <button type="button" class="clock-toggle" :class="{ 'is-on': imageConfig.show }" @click="$emit('toggle-show')">
        {{ imageConfig.show ? "图层开启" : "图层关闭" }}
      </button>
    </div>

    <div class="clock-image-drop">
      <div v-if="imageSource" class="clock-image-preview">
        <img :src="imageSource.sourceUrl" :alt="imageSource.name" class="clock-image-preview__img" />
        <div class="clock-image-preview__meta">
          <strong>{{ imageSource.name }}</strong>
          <span>{{ imageSource.width }} x {{ imageSource.height }}</span>
          <span v-if="imageSource.isGif" class="clock-image-preview__badge">
            GIF {{ gifFrameCount > 0 ? `${gifFrameCount} 帧` : "素材" }}
          </span>
        </div>
      </div>
      <div v-else class="clock-image-empty">
        <strong>还没有图片素材</strong>
        <span>{{ emptyDescription }}</span>
      </div>

      <div class="clock-image-actions">
        <button type="button" class="glx-button glx-button--primary" @click="openFilePicker">选择图片</button>
        <button type="button" class="glx-button glx-button--ghost" :disabled="!imageSource" @click="$emit('clear-image')">清除</button>
        <button type="button" class="glx-button glx-button--ghost" @click="$emit('set-square')">长宽一致</button>
      </div>

      <input
        ref="inputRef"
        class="clock-file-input"
        type="file"
        :accept="acceptValue"
        @change="handleFileChange"
      />
    </div>

    <div v-if="imageSource && imageSource.isGif && allowGif" class="clock-gif-controls">
      <button type="button" class="glx-button glx-button--ghost" @click="$emit('toggle-gif-play')">
        {{ gifIsPlaying ? "暂停预览" : "播放预览" }}
      </button>
      <div class="clock-row clock-row--compact">
        <span class="clock-row__label">播放速度</span>
        <div class="clock-stepper">
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-gif-speed', -0.5)">-</button>
          <strong class="clock-stepper__value">{{ gifPlaySpeed.toFixed(1) }}x</strong>
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-gif-speed', 0.5)">+</button>
        </div>
      </div>
    </div>

    <div class="clock-setting-grid">
      <div class="clock-row">
        <span class="clock-row__label">宽度</span>
        <div class="clock-stepper">
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'width', -1)">-</button>
          <strong class="clock-stepper__value">{{ imageConfig.width }}</strong>
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'width', 1)">+</button>
        </div>
      </div>

      <div class="clock-row">
        <span class="clock-row__label">高度</span>
        <div class="clock-stepper">
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'height', -1)">-</button>
          <strong class="clock-stepper__value">{{ imageConfig.height }}</strong>
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'height', 1)">+</button>
        </div>
      </div>

      <div class="clock-row">
        <span class="clock-row__label">X 位置</span>
        <div class="clock-stepper">
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'x', -1)">-</button>
          <strong class="clock-stepper__value">{{ imageConfig.x }}</strong>
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'x', 1)">+</button>
        </div>
      </div>

      <div class="clock-row">
        <span class="clock-row__label">Y 位置</span>
        <div class="clock-stepper">
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'y', -1)">-</button>
          <strong class="clock-stepper__value">{{ imageConfig.y }}</strong>
          <button type="button" class="clock-stepper__button" @click="$emit('adjust-image', 'y', 1)">+</button>
        </div>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, ref } from "vue";

const props = defineProps({
  title: {
    type: String,
    required: true,
  },
  description: {
    type: String,
    required: true,
  },
  imageConfig: {
    type: Object,
    required: true,
  },
  imageSource: {
    type: Object,
    default: null,
  },
  allowGif: {
    type: Boolean,
    default: false,
  },
  gifFrameCount: {
    type: Number,
    default: 0,
  },
  gifIsPlaying: {
    type: Boolean,
    default: false,
  },
  gifPlaySpeed: {
    type: Number,
    default: 1,
  },
  emptyDescription: {
    type: String,
    default: "网站端支持本地预览图片。",
  },
});

const emit = defineEmits([
  "select-file",
  "clear-image",
  "toggle-show",
  "adjust-image",
  "set-square",
  "toggle-gif-play",
  "adjust-gif-speed",
]);

const inputRef = ref(null);

const acceptValue = computed(() => {
  if (props.allowGif) {
    return "image/*,.gif";
  }
  return "image/png,image/jpeg,image/webp,image/bmp,.png,.jpg,.jpeg,.webp,.bmp";
});

function openFilePicker() {
  if (inputRef.value) {
    inputRef.value.click();
  }
}

function handleFileChange(event) {
  const files = event.target.files || [];
  const file = files[0];
  if (file) {
    emit("select-file", file);
  }
  event.target.value = "";
}
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

.clock-image-drop {
  display: flex;
  flex-direction: column;
  gap: 14px;
  padding: 16px;
  border: 2px solid #000000;
  background: #ffffff;
}

.clock-image-preview {
  display: grid;
  grid-template-columns: 140px minmax(0, 1fr);
  gap: 16px;
  align-items: center;
}

.clock-image-preview__img {
  width: 140px;
  aspect-ratio: 1;
  object-fit: contain;
  border: 2px solid #000000;
  background: #000000;
  image-rendering: pixelated;
}

.clock-image-preview__meta {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.clock-image-preview__badge {
  display: inline-flex;
  width: fit-content;
  padding: 4px 8px;
  border: 2px solid #000000;
  background: #facc15;
  font-size: 12px;
  font-weight: 700;
}

.clock-image-empty {
  display: flex;
  flex-direction: column;
  gap: 8px;
  color: var(--glx-text-muted);
}

.clock-image-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.clock-file-input {
  display: none;
}

.clock-gif-controls {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  border: 2px solid #000000;
  background: #ffffff;
}

.clock-setting-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.clock-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.clock-row--compact {
  width: fit-content;
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

@media (max-width: 900px) {
  .clock-image-preview,
  .clock-setting-grid {
    grid-template-columns: minmax(0, 1fr);
  }
}

@media (max-width: 720px) {
  .clock-section__head,
  .clock-row {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>

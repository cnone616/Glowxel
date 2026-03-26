<template>
  <view class="settings-card">
    <view class="card-title-section">
      <Icon name="text" :size="32" />
      <text class="card-title">字体样式</text>
    </view>

    <view class="setting-group">
      <view class="setting-item">
        <text class="setting-label">统一字体</text>
        <scroll-view scroll-x class="font-scroll" show-scrollbar="false">
          <view class="font-list">
            <view
              v-for="font in fontOptions"
              :key="font.id"
              class="font-card"
              :class="{ active: selectedFont === font.id }"
              @click="$emit('select-font', font.id)"
            >
              <view
                class="font-preview-board"
                :style="{ backgroundColor: font.previewBg }"
              >
                <view class="font-preview-viewport">
                  <view
                    class="font-preview-grid-wrap"
                    :style="getFontPreviewWrapStyle(font.id)"
                  >
                    <view
                      class="font-preview-grid"
                      :style="getFontPreviewGridStyle(font.id)"
                    >
                      <view
                        v-for="cell in getFontPreviewCells(font.id)"
                        :key="cell.key"
                        class="font-preview-pixel"
                        :style="{
                          backgroundColor: cell.active
                            ? font.previewColor
                            : 'transparent',
                        }"
                      ></view>
                    </view>
                  </view>
                </view>
              </view>
              <text class="font-card-name">{{ font.name }}</text>
            </view>
          </view>
        </scroll-view>
      </view>

      <view class="setting-item">
        <view class="setting-header-row">
          <text class="setting-label">显示秒钟</text>
          <view class="toggle-switch" @click="$emit('toggle-seconds')">
            <view class="switch-track" :class="{ active: showSeconds }">
              <view class="switch-thumb"></view>
            </view>
          </view>
        </view>
      </view>

      <view class="setting-item">
        <text class="setting-label">小时制式</text>
        <view class="align-buttons">
          <view
            class="align-btn"
            :class="{ active: hourFormat === 24 }"
            @click="$emit('set-hour-format', 24)"
          >
            <text class="align-text">24 小时</text>
          </view>
          <view
            class="align-btn"
            :class="{ active: hourFormat === 12 }"
            @click="$emit('set-hour-format', 12)"
          >
            <text class="align-text">12 小时</text>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "../Icon.vue";
import {
  drawClockTextToPixels,
  getClockTextHeight,
  getClockTextWidth,
} from "../../utils/clockCanvas.js";

const PREVIEW_PIXEL_SIZE_RPX = 10;
const PREVIEW_PIXEL_GAP_RPX = 2;
const PREVIEW_VIEWPORT_WIDTH_RPX = 164;
const PREVIEW_VIEWPORT_HEIGHT_RPX = 72;

export default {
  components: {
    Icon,
  },
  props: {
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
  },
  emits: ["select-font", "toggle-seconds", "set-hour-format"],
  methods: {
    getFontPreviewText() {
      return this.showSeconds ? "12:34:56" : "12:34";
    },

    getFontPreviewMetrics(fontId) {
      const text = this.getFontPreviewText();
      const width = getClockTextWidth(text, fontId, 1);
      const height = getClockTextHeight(fontId, 1);

      const gridWidth =
        width * PREVIEW_PIXEL_SIZE_RPX + (width - 1) * PREVIEW_PIXEL_GAP_RPX;
      const gridHeight =
        height * PREVIEW_PIXEL_SIZE_RPX +
        (height - 1) * PREVIEW_PIXEL_GAP_RPX;

      const widthScale = PREVIEW_VIEWPORT_WIDTH_RPX / gridWidth;
      const heightScale = PREVIEW_VIEWPORT_HEIGHT_RPX / gridHeight;

      return {
        width,
        height,
        gridWidth,
        gridHeight,
        scale: Math.min(1, widthScale, heightScale),
      };
    },

    getFontPreviewWrapStyle(fontId) {
      const metrics = this.getFontPreviewMetrics(fontId);
      return {
        width: `${metrics.gridWidth}rpx`,
        height: `${metrics.gridHeight}rpx`,
        transform: `scale(${metrics.scale})`,
      };
    },

    getFontPreviewGridStyle(fontId) {
      const metrics = this.getFontPreviewMetrics(fontId);
      return {
        gridTemplateColumns: `repeat(${metrics.width}, ${PREVIEW_PIXEL_SIZE_RPX}rpx)`,
        gridTemplateRows: `repeat(${metrics.height}, ${PREVIEW_PIXEL_SIZE_RPX}rpx)`,
        gap: `${PREVIEW_PIXEL_GAP_RPX}rpx`,
        width: `${metrics.gridWidth}rpx`,
        height: `${metrics.gridHeight}rpx`,
      };
    },

    getFontPreviewCells(fontId) {
      const metrics = this.getFontPreviewMetrics(fontId);
      const pixels = new Map();
      drawClockTextToPixels(
        this.getFontPreviewText(),
        0,
        0,
        "#ffffff",
        pixels,
        fontId,
        1,
        "left",
      );

      const cells = [];
      for (let y = 0; y < metrics.height; y++) {
        for (let x = 0; x < metrics.width; x++) {
          cells.push({
            key: `${fontId}-${x}-${y}`,
            active: pixels.has(`${x},${y}`),
          });
        }
      }
      return cells;
    },
  },
};
</script>

<style scoped>
.settings-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  padding: 20rpx;
  margin-bottom: 16rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
  flex: 1;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.setting-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.font-scroll {
  width: 100%;
}

.font-list {
  display: flex;
  gap: 16rpx;
  padding: 4rpx 0;
}

.font-card {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  padding: 16rpx;
  min-width: 220rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.font-card:active {
  transform: scale(0.98);
}

.font-card.active {
  border-color: var(--accent-primary);
  box-shadow: 0 0 16rpx rgba(79, 127, 255, 0.2);
}

.font-preview-board {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 104rpx;
  padding: 16rpx 12rpx;
  border-radius: 12rpx;
}

.font-preview-viewport {
  width: 164rpx;
  height: 72rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}

.font-preview-grid-wrap {
  transform-origin: center center;
}

.font-preview-grid {
  display: grid;
}

.font-preview-pixel {
  width: 10rpx;
  height: 10rpx;
  border-radius: 2rpx;
}

.font-card-name {
  font-size: 22rpx;
  color: var(--text-primary);
  text-align: center;
  white-space: nowrap;
}

.align-buttons {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.align-btn {
  flex: 1;
  padding: 20rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  text-align: center;
  transition: var(--transition-base);
}

.align-btn:active {
  transform: scale(0.98);
}

.align-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.align-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.align-btn.active .align-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 22rpx;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 18rpx;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #ffffff;
}
</style>

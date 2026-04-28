<template>
  <view class="settings-card glx-panel-card">
    <view class="card-title-section glx-panel-head">
      <view class="font-title-main">
        <Icon name="text" :size="32" />
        <text class="card-title glx-panel-title">字体样式</text>
      </view>
    </view>

    <view class="setting-group">
      <view class="settings-block">
        <text class="setting-label">统一字体</text>
        <view class="font-list">
          <view
            v-for="font in fontOptions"
            :key="font.id"
            class="font-card glx-feature-card-option"
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
            <text
              class="glx-feature-card-option__label glx-feature-card-option__label--single-line"
              >{{ font.name }}</text
            >
          </view>
        </view>
      </view>

      <view v-if="showSecondsControl" class="settings-block">
        <text class="setting-label">秒钟</text>
        <view v-if="showSecondsStyle === 'tabs'" class="seconds-tabs">
          <view
            class="seconds-tab glx-feature-option"
            :class="{ active: showSeconds === false }"
            @click="$emit('set-show-seconds', false)"
          >
            <text class="glx-feature-option__label">关闭</text>
          </view>
          <view
            class="seconds-tab glx-feature-option"
            :class="{ active: showSeconds === true }"
            @click="$emit('set-show-seconds', true)"
          >
            <text class="glx-feature-option__label">显示</text>
          </view>
        </view>
        <view v-else class="setting-header-row">
          <text class="setting-label">显示秒钟</text>
          <GlxSwitch
            class="glx-row-switch"
            :checked="showSeconds"
            @change="$emit('toggle-seconds')"
          />
        </view>
      </view>

      <view v-if="showHourFormat" class="settings-block">
        <text class="setting-label">小时制式</text>
        <view class="align-buttons">
          <view
            class="align-btn glx-feature-option"
            :class="{ active: hourFormat === 24 }"
            @click="$emit('set-hour-format', 24)"
          >
            <text class="glx-feature-option__label">24 小时</text>
          </view>
          <view
            class="align-btn glx-feature-option"
            :class="{ active: hourFormat === 12 }"
            @click="$emit('set-hour-format', 12)"
          >
            <text class="glx-feature-option__label">12 小时</text>
          </view>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "../Icon.vue";
import GlxSwitch from "../GlxSwitch.vue";
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
    GlxSwitch,
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
    showSecondsStyle: {
      type: String,
      default: "switch",
    },
    showSecondsControl: {
      type: Boolean,
      default: false,
    },
    hourFormat: {
      type: Number,
      default: 24,
    },
    showHourFormat: {
      type: Boolean,
      default: true,
    },
  },
  emits: [
    "select-font",
    "toggle-seconds",
    "set-hour-format",
    "set-show-seconds",
  ],
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
        height * PREVIEW_PIXEL_SIZE_RPX + (height - 1) * PREVIEW_PIXEL_GAP_RPX;

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
  background-color: transparent;
  border: 0;
  padding: 8rpx 12rpx 14rpx;
  margin-bottom: 16rpx;
  box-shadow: none;
}

.font-title-main {
  display: flex;
  align-items: center;
  gap: 10rpx;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.settings-block {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  padding: 0 2rpx;
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

.seconds-tabs {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.seconds-tab {
  flex: 1;
  padding: 20rpx 16rpx;
  text-align: center;
  transition: var(--transition-base);
}

.font-list {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16rpx;
  padding: 4rpx 2rpx;
  box-sizing: border-box;
}

.font-card {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  padding: 16rpx;
  width: 100%;
  min-width: 0;
  box-sizing: border-box;
  transition: var(--transition-base);
}

.font-card.active {
  box-shadow: none;
}

.font-preview-board {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  min-height: 104rpx;
  padding: 16rpx 12rpx;
  border-radius: 0;
  box-sizing: border-box;
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

.align-buttons {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.align-btn {
  flex: 1;
  padding: 20rpx 16rpx;
  text-align: center;
  transition: var(--transition-base);
}
</style>

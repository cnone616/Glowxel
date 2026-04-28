<template>
  <view class="clock-editor-page glx-page-shell">
    <!-- 状态栏占位 -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>

    <!-- 头部 -->
    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">{{ pageHeaderTitle }}</text>
      <view class="nav-right"></view>
    </view>

    <!-- 主题图片展示区 -->
    <view class="canvas-section">
      <view class="canvas-container theme-image-stage" ref="canvasContainer">
        <image
          v-if="displayClockThemePreset && displayClockThemePreset.previewImage"
          :src="displayClockThemePreset.previewImage"
          class="theme-image-stage__image"
          mode="aspectFit"
        />
        <view v-else class="theme-image-stage__empty">
          <text class="theme-image-stage__empty-text">请选择主题</text>
        </view>
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-caption-title">{{ previewPanelTitle }}</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
            :class="{ disabled: isSending }"
            @click="sendToDevice"
          >
            <Icon name="link" :size="36" color="#000000" />
            <text>发送</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 主内容：当前 Tab 的表单 -->
    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
      :style="{ height: contentHeight }"
    >
      <view class="content-wrapper glx-scroll-stack">
        <ClockThemePanel
          :presets="clockThemePresets"
          :selected-theme-id="displayClockThemeId"
          :current-theme-id="currentDeviceThemeId"
          :active-preset="displayClockThemePreset"
          :has-image="!!config.image.data || !!imagePixels"
          :is-modified="isClockThemeModified"
          @apply-theme="applyClockTheme"
        />
      </view>
    </scroll-view>

    <view
      v-if="isSending"
      class="glx-device-sending-overlay"
      @touchmove.stop.prevent
    >
      <view class="glx-device-sending-card">
        <GlxInlineLoader
          class="glx-device-sending-spinner"
          variant="chase"
          size="lg"
        />
        <text class="glx-device-sending-title">{{ sendOverlayTitle }}</text>
        <text class="glx-device-sending-tip">{{ sendOverlayTip }}</text>
      </view>
    </view>

    <Toast
      ref="toastRef"
      @show="handleToastShow"
      @hide="handleToastHide"
    />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import clockPreviewMixin from "./mixins/clockPreviewMixin.js";
import imageGifMixin from "./mixins/imageGifMixin.js";
import deviceSyncMixin from "./mixins/deviceSyncMixin.js";
import deviceSendUxMixin from "../../mixins/deviceSendUxMixin.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";
import ClockThemePanel from "../../components/clock-editor/ClockThemePanel.vue";
import {
  applyClockThemePreset,
  findClockThemePreset,
  getClockThemePresets,
} from "../../utils/clockThemePresets.js";

const CLOCK_DEVICE_THEME_ID_KEY = "clock_device_theme_id";

export default {
  mixins: [
    statusBarMixin,
    clockPreviewMixin,
    imageGifMixin,
    deviceSyncMixin,
    deviceSendUxMixin,
  ],
  components: {
    Icon,
    Toast,
    GlxInlineLoader,
    ClockThemePanel,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isReady: false,
      clockMode: "theme", // theme=主题模式
      gifAnimationData: null, // GIF 动画数据 { frameCount, frames }
      gifRenderedFrameMaps: null, // GIF 所有帧的像素 Map 数组
      gifFrameIndex: 0, // 当前播放帧索引
      gifTimer: null, // GIF 动画定时器
      gifIsPlaying: false, // 是否正在播放
      gifPlaySpeed: 1.0, // 播放速度倍率
      lastAppliedClockThemeId: "",
      deviceThemeId: "",
      _gifParser: null, // GIF 解析器实例，改宽高时重新生成数据
      _imageConvertToken: 0,

      contentHeight: "calc(100vh - 112rpx - 120rpx - 80rpx)",

      imagePixels: null,

      // PixelCanvas 视图控制
      zoom: 4,
      pan: { x: 0, y: 0 },
      containerSize: { width: 320, height: 320 },
      canvasReady: false,
      previewTick: 0,
      previewClockTimer: null,

      // loading 动画定时器（实例变量，方便清理）
      loadingTimer: null,
      loadingActive: false,

      clockThemePresets: getClockThemePresets(),

      config: {
        font: "classic_5x7",
        showSeconds: false,
        hourFormat: 24,
        time: {
          show: true,
          fontSize: 1,
          x: 32,
          y: 5,
          color: "#64c8ff",
          align: "center",
        },
        date: {
          show: true,
          fontSize: 1,
          x: 32,
          y: 29,
          color: "#787878",
          align: "center",
        },
        week: {
          show: true,
          x: 32,
          y: 38,
          color: "#646464",
          align: "center",
        },
        image: {
          show: false,
          x: 6,
          y: 6,
          width: 64,
          height: 64,
          data: null,
        },
      },

    };
  },

  watch: {
    canvasReady(newVal) {
      if (newVal) {
        this.$nextTick(() => {
          this.drawCanvas();
        });
      }
    },
  },

  computed: {
    accentColor() {
      return "#4F7FFF";
    },
    pageHeaderTitle() {
      return "主题模式";
    },
    displayClockThemeId() {
      return this.lastAppliedClockThemeId;
    },
    displayClockThemePreset() {
      if (!this.displayClockThemeId) {
        return null;
      }
      return findClockThemePreset(this.displayClockThemeId);
    },
    currentDeviceThemeId() {
      if (!this.deviceStore || !this.deviceStore.connected) {
        return "";
      }
      if (this.deviceStore.deviceMode !== "theme") {
        return "";
      }
      return this.deviceThemeId;
    },
    isClockThemeModified() {
      return false;
    },
    previewPanelTitle() {
      return "主题展示";
    },
  },

  onUnload() {
    this.cleanupTransientState();
  },

  onHide() {
    this.cleanupTransientState();
  },

  onLoad() {
    this.clockMode = "theme";

    this.loadConfig();
    const savedDeviceThemeId = uni.getStorageSync(CLOCK_DEVICE_THEME_ID_KEY);
    if (typeof savedDeviceThemeId === "string") {
      this.deviceThemeId = savedDeviceThemeId;
    }
    this.ensureValidThemeSelection();

    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();

    const systemInfo = uni.getSystemInfoSync();
    const statusBarHeight = systemInfo.statusBarHeight || 0;
    const headerHeight = 56;
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight - 360}px`;
  },

  onReady() {
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef);
      }
      this.initPreviewCanvas();
    });
  },

  onShow() {
  },

  methods: {
    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query.select(".canvas-section").boundingClientRect((sectionRect) => {
            if (!sectionRect || !sectionRect.height) {
              return;
            }

            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - sectionRect.height;
            this.contentHeight = `${Math.max(120, nextHeight)}px`;
          });
          query
            .select(".canvas-container")
            .boundingClientRect((data) => {
              if (data && data.width > 0) {
                this.containerSize = { width: data.width, height: data.width };
                const fitZoom = Math.max(2, Math.floor((data.width * 0.96) / 64));
                this.zoom = fitZoom;
                this.pan = {
                  x: (data.width - 64 * fitZoom) / 2,
                  y: (data.width - 64 * fitZoom) / 2,
                };
              } else {
                this.zoom = 4;
                this.pan = { x: 16, y: 16 };
              }
              this.canvasReady = true;
              this.isReady = true;
            })
            .exec();
        }, 80);
      });
    },

    cleanupTransientState() {
      this.stopGifAnimation();
      this.stopLoading();
      if (this._resizeDebounceTimer) {
        clearTimeout(this._resizeDebounceTimer);
        this._resizeDebounceTimer = null;
      }
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? {
            r: parseInt(result[1], 16),
            g: parseInt(result[2], 16),
            b: parseInt(result[3], 16),
          }
        : { r: 255, g: 255, b: 255 };
    },
    handleBack() {
      uni.navigateBack();
    },
    ensureValidThemeSelection() {
      if (this.clockThemePresets.length === 0) {
        this.lastAppliedClockThemeId = "";
        return;
      }

      const hasMatchedPreset = this.clockThemePresets.some(
        (preset) => preset.id === this.lastAppliedClockThemeId,
      );
      if (hasMatchedPreset) {
        return;
      }

      this.applyClockTheme(this.clockThemePresets[0].id);
    },
    applyClockTheme(themeId) {
      this._imageConvertToken += 1;
      this.config = applyClockThemePreset(this.config, themeId);
      this.lastAppliedClockThemeId = themeId;
      this.stopGifAnimation();
      this.gifAnimationData = null;
      this.gifRenderedFrameMaps = null;
      this.gifFrameIndex = 0;
      this._gifParser = null;
      this.imagePixels = null;
      this.config.image.data = null;
      this.config.image.show = false;
      this.drawCanvas();
    },
  },
};
</script>

<style scoped>
.clock-editor-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.icon {
  font-size: 56rpx;
  color: var(--text-primary);
}

.action-btn-sm {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn-sm.primary {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.preview-hint {
  padding: 20rpx 32rpx;
  background-color: var(--bg-tertiary);
  border-bottom: 2rpx solid var(--nb-ink);
}

.preview-hint-text {
  font-size: 22rpx;
  color: var(--text-secondary);
  line-height: 1.4;
}

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000;
}

.canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  background: #000000;
}

.theme-image-stage {
  padding: 28rpx;
  box-sizing: border-box;
}

.theme-image-stage__image {
  width: 100%;
  height: 100%;
  display: block;
  image-rendering: pixelated;
}

.theme-image-stage__empty {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.theme-image-stage__empty-text {
  font-size: 24rpx;
  font-weight: 600;
  color: #ffffff;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
  padding: 10rpx 16rpx 12rpx;
  background: var(--bg-tertiary);
}

.preview-caption-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 0;
}

.preview-caption-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.preview-actions .action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 0;
}

.preview-actions .action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.preview-actions .action-btn-sm.primary text {
  color: #000000;
}

.canvas-wrapper {
  width: 100%;
  display: flex;
  justify-content: center;
}

.clock-canvas {
  width: 320px;
  height: 320px;
  background-color: #000;
  border: 2rpx solid var(--nb-yellow);
  border-radius: 0;
  box-shadow: none;
}

.tool-selection {
  display: flex;
  gap: 16rpx;
  margin-bottom: 24rpx;
}

.tool-option {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 32rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
}

.tool-option.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
  box-shadow: none;
}

.tool-name {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.tool-option.active .tool-name {
  color: #000000;
  font-weight: 700;
}

.action-buttons {
  display: flex;
  gap: 16rpx;
  margin-top: 24rpx;
}

.action-button {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  font-size: 22rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.action-button.danger {
  background-color: #d92d20;
  border-color: #000000;
  color: #ffffff;
}

.action-button.danger:active {
  background-color: #d92d20;
}

.current-color-section {
  margin-bottom: 24rpx;
}

.current-color-display {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 0;
  margin-top: 12rpx;
  border: 2rpx solid var(--nb-ink);
}

.current-color-box {
  width: 80rpx;
  height: 80rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  box-shadow: none;
}

.current-color-hex {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
}

.palette-toggle {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  margin: 24rpx 0;
  transition: var(--transition-base);
}

.palette-toggle:active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.palette-toggle-text {
  font-size: 24rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.palette-toggle-icon {
  font-size: 24rpx;
  color: var(--nb-ink);
}

.full-color-palette {
  display: grid;
  grid-template-columns: repeat(12, 1fr);
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 0;
  margin-bottom: 24rpx;
  max-height: 600rpx;
  overflow-y: auto;
  border: 2rpx solid var(--nb-ink);
}

.palette-color-item {
  aspect-ratio: 1;
  border-radius: 0;
  border: 2rpx solid #000000;
  transition: var(--transition-base);
}

.palette-color-item.active {
  border-color: var(--nb-yellow);
  border-width: 4rpx;
  box-shadow: none;
  transform: none;
  z-index: auto;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12rpx;
}

.quick-size-btn {
  min-height: 60rpx;
  padding: 12rpx 24rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  background-color: #ffffff;
  border: 2rpx solid #000000;
  border-radius: 0;
  box-shadow: none;
  transition: var(--transition-base);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: #000000;
  font-weight: 700;
}

.warning-color {
  color: #ff6464 !important;
}

.warning-text {
  font-size: 20rpx;
  color: #ff6464;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
  gap: 4rpx;
}

.info-color {
  color: #4f7fff !important;
}

.info-text {
  font-size: 20rpx;
  color: #4f7fff;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.content-wrapper {
  padding: 0 0 56rpx;
}

/* 底部 Tab 栏：与拼豆辅助页底部控制区风格一致 */
.bottom-tabs {
  display: flex;
  flex-shrink: 0;
  padding: 2rpx 10rpx 0;
  padding-bottom: var(--layout-bottom-offset);
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--nb-ink);
  gap: 2rpx;
}

.bottom-tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 2rpx;
  min-height: 68rpx;
  padding: 2rpx 0;
  background-color: transparent;
}

.bottom-tab-item:active {
  background-color: transparent;
}

.bottom-tab-item.active {
  background-color: transparent;
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: #000000;
  font-weight: 900;
  font-size: 22rpx;
}

.settings-card {
  background-color: transparent;
  border: 0;
  padding: 0;
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

.setting-item-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.quick-size-btn {
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-yellow);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: var(--nb-yellow);
  font-weight: 500;
}

.size-input {
  width: 120rpx;
  height: 56rpx;
  padding: 0 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  text-align: center;
  font-family: monospace;
  transition: var(--transition-base);
}

.size-input:focus {
  border-color: var(--nb-yellow);
}

.size-input.warning {
  border-color: #ff6464;
  background-color: rgba(255, 100, 100, 0.1);
}

.warning-text {
  font-size: 18rpx;
  color: #ff6464;
  margin-top: 4rpx;
}

.setting-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.setting-control {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.setting-control-buttons {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.control-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
}

.control-icon {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.setting-value-large {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--nb-yellow);
  min-width: 64rpx;
  text-align: center;
}

.setting-value {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--nb-yellow);
  min-width: 40rpx;
  text-align: right;
}

.setting-slider {
  flex: 1;
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--nb-ink);
  border-radius: 0;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 0;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #000000;
}

.image-upload {
  margin-bottom: 24rpx;
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 64rpx 32rpx;
  background-color: var(--bg-secondary);
  border: 4rpx dashed var(--nb-ink);
  border-radius: 0;
  transition: var(--transition-base);
}

.upload-placeholder:active {
  border-color: var(--nb-yellow);
  background-color: var(--bg-tertiary);
}

.upload-text {
  font-size: 26rpx;
  color: var(--text-primary);
  margin-top: 16rpx;
}

.upload-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 8rpx;
}

.image-preview {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16rpx;
  position: relative;
}

.gif-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  background: rgba(139, 92, 246, 0.9);
  border-radius: 0;
  padding: 4rpx 12rpx;
}

.gif-badge-text {
  color: #fff;
  font-size: 22rpx;
  font-weight: 600;
}

.preview-image {
  width: 100%;
  height: 400rpx;
  background-color: var(--bg-secondary);
  border-radius: 0;
  /* border: 2rpx solid var(--nb-ink); */
}

.image-actions {
  display: flex;
  gap: 16rpx;
}

.image-action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 20rpx;
  background-color: #2563eb;
  border: 2rpx solid #000000;
  border-radius: 12rpx;
  font-size: 24rpx;
  color: #ffffff;
  transition: var(--transition-base);
}

.image-action-btn.danger {
  background-color: #d92d20;
  border-color: #000000;
  color: #ffffff;
}

.image-action-btn.danger:active {
  background-color: #d92d20;
}

/* 传输遮罩弹窗 */
.sending-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.sending-modal {
  background: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
  padding: 60rpx 50rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  min-width: 400rpx;
}

.sending-spinner {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  line-height: 1;
}

.sending-title {
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.sending-tip {
  font-size: 24rpx;
  color: #4a4a4a;
}

.sending-text {
  font-size: 24rpx;
  color: #fff;
}

.action-btn.disabled {
  opacity: 0.5;
  pointer-events: none;
}
</style>

<template>
  <view class="snake-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">贪吃蛇</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container" :style="previewCanvasBoxStyle">
        <PixelCanvas
          v-if="previewCanvasReady && !shouldShowSendingSnapshot"
          :width="64"
          :height="64"
          :pixels="currentPreviewPixels"
          :zoom="previewZoom"
          :offset-x="previewOffset.x"
          :offset-y="previewOffset.y"
          :canvas-width="previewContainerSize.width"
          :canvas-height="previewContainerSize.height"
          :grid-visible="previewGridVisible"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="snakePreviewCanvas"
        />
        <PixelPreviewBoard
          v-else-if="previewCanvasReady && shouldShowSendingSnapshot"
          :width="64"
          :height="64"
          :pixels="sendingPreviewPixels"
          :refresh-token="sendingPreviewTick"
          :zoom="previewZoom"
          :offset-x="previewOffset.x"
          :offset-y="previewOffset.y"
          :grid-visible="previewGridVisible"
          :is-dark-mode="true"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">预览效果</text>
          <text class="preview-note">发送会保存当前外观和参数</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="var(--nb-ink)" />
            <text>发送</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view
      scroll-y
      class="content glx-scroll-region glx-page-shell__content"
      :style="{ height: contentHeight }"
    >
      <view class="content-wrapper glx-scroll-stack">
        <view v-if="currentTab === 0" class="tab-panel glx-tab-panel">
          <view class="tab-section-head glx-section-head">
            <text class="tab-section-title glx-section-title">外观</text>
            <text class="tab-section-meta">皮肤、字体、果子颜色</text>
          </view>

          <view class="skin-card glx-panel-card snake-top-card">
            <view class="snake-top-head">
              <text class="card-title glx-panel-title">蛇皮肤</text>
            </view>
            <view class="skin-toolbar">
              <view
                class="skin-toolbar-btn glx-feature-option"
                @click="randomizeSkinColor"
              >
                <text class="glx-feature-option__label">换个随机颜色</text>
              </view>
            </view>
            <view class="skin-grid">
              <view
                v-for="item in snakeSkinOptions"
                :key="item.value"
                class="skin-option glx-feature-option"
                :class="{ active: snakeSkin === item.value }"
                @click="selectSnakeSkin(item.value)"
              >
                <text class="glx-feature-option__label">{{ item.label }}</text>
              </view>
            </view>
          </view>

          <ClockFontPanel
            :font-options="fontOptions"
            :selected-font="font"
            :show-seconds="showSeconds"
            :show-hour-format="false"
            @select-font="selectFont"
          />

          <view class="card glx-panel-card glx-editor-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">颜色</text>
            </view>
            <view class="form-row color-picker-row">
              <ColorPanelPicker
                :value="foodColor"
                label="果子颜色"
                :preset-colors="foodPresetColors"
                @change="handleFoodColorChange"
              />
            </view>
          </view>
        </view>

        <view v-else class="tab-panel glx-tab-panel">
          <view class="tab-section-head glx-section-head">
            <text class="tab-section-title glx-section-title">参数</text>
            <text class="tab-section-meta">速度、蛇宽、秒钟</text>
          </view>

          <view class="card glx-panel-card glx-editor-card">
            <view class="form-row">
              <text class="form-label">速度 {{ speed }}</text>
              <GlxStepper
                :value="speed"
                :min="1"
                :max="10"
                :step="1"
                @change="handleSpeedChange"
              />
            </view>
            <view class="form-row">
              <text class="form-label">蛇宽 {{ snakeWidth }}</text>
              <GlxStepper
                :value="snakeWidth"
                :min="2"
                :max="4"
                :step="1"
                @change="handleSnakeWidthChange"
              />
            </view>
            <view class="form-row">
              <text class="form-label">显示秒钟</text>
              <GlxSwitch
                class="glx-row-switch"
                :checked="showSeconds"
                @change="setShowSeconds($event.detail.value)"
              />
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <view class="bottom-tabs">
      <view
        v-for="tab in tabDefinitions"
        :key="tab.index"
        class="bottom-tab-item"
        :class="{ active: currentTab === tab.index }"
        @click="switchTab(tab.index)"
      >
        <Icon
          :name="tab.icon"
          :size="36"
          :color="currentTab === tab.index ? '#000000' : '#666666'"
        />
        <text class="bottom-tab-text">{{ tab.label }}</text>
      </view>
    </view>

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
import statusBarMixin from "../../mixins/statusBar.js";
import deviceSendUxMixin from "../../mixins/deviceSendUxMixin.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import PixelPreviewBoard from "../../components/PixelPreviewBoard.vue";
import GlxStepper from "../../components/GlxStepper.vue";
import GlxSwitch from "../../components/GlxSwitch.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import ClockFontPanel from "../../components/clock-editor/ClockFontPanel.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { buildLedMatrixPreviewSequence } from "../../utils/ledMatrixShowcase.js";
import { getClockFontOptions } from "../../utils/clockCanvas.js";

const SNAKE_MODE_CONFIG_KEY = "snake_mode_config";
const SNAKE_FONT_OPTIONS = getClockFontOptions();
const SNAKE_FONT_IDS = Object.freeze(
  SNAKE_FONT_OPTIONS.map((item) => item.id),
);
const SNAKE_SKIN_OPTIONS = Object.freeze([
  { value: "solid", label: "纯色" },
  { value: "gradient", label: "渐变" },
  { value: "spotted", label: "斑点" },
]);
const SNAKE_TAB_DEFINITIONS = Object.freeze([
  { index: 0, label: "外观", icon: "palette" },
  { index: 1, label: "参数", icon: "setting" },
]);
const SNAKE_RANDOM_COLOR_MAP = Object.freeze({
  solid: [
    "#56d678",
    "#39c46a",
    "#8bff8a",
    "#4fd1c5",
    "#7dd3fc",
    "#facc15",
  ],
  gradient: [
    "#ff8a5b",
    "#ffd166",
    "#7c9cff",
    "#fb7185",
    "#67e8f9",
    "#86efac",
  ],
  spotted: [
    "#a3d977",
    "#d4a373",
    "#34d399",
    "#22c55e",
    "#f59e0b",
    "#c084fc",
  ],
});

function createDefaultSnakeModeConfig() {
  return {
    speed: 6,
    snakeWidth: 2,
    snakeColor: "#56d678",
    foodColor: "#ffa854",
    font: "minimal_3x5",
    showSeconds: false,
    snakeSkin: "gradient",
  };
}

function normalizeSavedSnakeModeConfig(saved) {
  const normalized = createDefaultSnakeModeConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (Number.isFinite(Number(saved.speed))) {
    normalized.speed = Math.max(1, Math.min(10, Number(saved.speed)));
  }
  if (Number.isFinite(Number(saved.snakeWidth))) {
    normalized.snakeWidth = Math.max(2, Math.min(4, Number(saved.snakeWidth)));
  }
  if (typeof saved.snakeColor === "string") {
    normalized.snakeColor = saved.snakeColor;
  }
  if (typeof saved.foodColor === "string") {
    normalized.foodColor = saved.foodColor;
  }
  if (typeof saved.font === "string" && SNAKE_FONT_IDS.includes(saved.font)) {
    normalized.font = saved.font;
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    normalized.showSeconds = saved.showSeconds;
  }
  if (
    saved.snakeSkin === "solid" ||
    saved.snakeSkin === "gradient" ||
    saved.snakeSkin === "spotted"
  ) {
    normalized.snakeSkin = saved.snakeSkin;
  }
  return normalized;
}

function readSavedSnakeModeConfig() {
  const saved = uni.getStorageSync(SNAKE_MODE_CONFIG_KEY);
  if (!saved || typeof saved !== "object") {
    return null;
  }
  return normalizeSavedSnakeModeConfig(saved);
}

function cloneSnakeModeConfig(config) {
  return {
    speed: config.speed,
    snakeWidth: config.snakeWidth,
    snakeColor: config.snakeColor,
    foodColor: config.foodColor,
    font: config.font,
    showSeconds: config.showSeconds,
    snakeSkin: config.snakeSkin,
  };
}

function normalizeHexText(value) {
  return typeof value === "string" ? value.trim().toLowerCase() : "";
}

function pickRandomSnakeColorBySkin(snakeSkin, currentColor) {
  const colorPool = SNAKE_RANDOM_COLOR_MAP[snakeSkin];
  if (!Array.isArray(colorPool) || colorPool.length === 0) {
    return currentColor;
  }
  const current = normalizeHexText(currentColor);
  const candidates = colorPool.filter((item) => item !== current);
  const pool = candidates.length > 0 ? candidates : colorPool;
  return pool[Math.floor(Math.random() * pool.length)];
}

function shouldAvoidSnakeColor(color) {
  return normalizeHexText(color) === "#000000";
}

function resolveVisibleSnakeColor(snakeSkin, snakeColor) {
  if (!shouldAvoidSnakeColor(snakeColor)) {
    return snakeColor;
  }
  return pickRandomSnakeColorBySkin(snakeSkin, snakeColor);
}

export default {
  mixins: [statusBarMixin, deviceSendUxMixin],
  components: {
    Icon,
    Toast,
    GlxInlineLoader,
    PixelCanvas,
    PixelPreviewBoard,
    GlxStepper,
    GlxSwitch,
    ColorPanelPicker,
    ClockFontPanel,
  },
  data() {
    const defaultConfig = createDefaultSnakeModeConfig();
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx - 520rpx - 112rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      sendingPreviewPixels: new Map(),
      sendingPreviewTick: 0,
      previewFrameDelays: [],
      previewFrameIndex: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      currentTab: 0,
      tabDefinitions: SNAKE_TAB_DEFINITIONS,
      speed: defaultConfig.speed,
      snakeWidth: defaultConfig.snakeWidth,
      snakeColor: defaultConfig.snakeColor,
      foodColor: defaultConfig.foodColor,
      font: defaultConfig.font,
      showSeconds: defaultConfig.showSeconds,
      snakeSkin: defaultConfig.snakeSkin,
      fontOptions: SNAKE_FONT_OPTIONS,
      snakeSkinOptions: SNAKE_SKIN_OPTIONS,
      foodPresetColors: [
        { hex: "#ffa854" },
        { hex: "#ff7f50" },
        { hex: "#ff5c5c" },
        { hex: "#ffd166" },
        { hex: "#c7f464" },
        { hex: "#ffffff" },
      ],
    };
  },
  computed: {
    currentPreviewPixels() {
      if (this.previewFrameMaps.length === 0) {
        return new Map();
      }
      if (this.previewFrameIndex >= this.previewFrameMaps.length) {
        return this.previewFrameMaps[0];
      }
      return this.previewFrameMaps[this.previewFrameIndex];
    },
    previewCanvasBoxStyle() {
      const size =
        this.previewContainerSize && this.previewContainerSize.height
          ? this.previewContainerSize.height
          : 320;
      return {
        height: `${size}px`,
      };
    },
    previewGridVisible() {
      return true;
    },
  },
  watch: {
    speed() {
      this.schedulePreviewRefresh();
    },
    snakeWidth() {
      this.resetPreviewFrames();
      this.schedulePreviewRefresh();
    },
    snakeColor() {
      this.schedulePreviewRefresh();
    },
    foodColor() {
      this.schedulePreviewRefresh();
    },
    font() {
      this.resetPreviewFrames();
      this.schedulePreviewRefresh();
    },
    showSeconds() {
      this.resetPreviewFrames();
      this.schedulePreviewRefresh();
    },
    snakeSkin() {
      this.schedulePreviewRefresh();
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    this.loadSavedConfig();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
  },
  onHide() {
    this.cleanupPreviewTimers();
  },
  onUnload() {
    this.cleanupPreviewTimers();
  },
  methods: {
    captureSendingPreview() {
      this.sendingPreviewPixels = new Map(this.currentPreviewPixels);
      this.sendingPreviewTick += 1;
    },
    clearSendingPreview() {
      this.sendingPreviewPixels = new Map();
      this.sendingPreviewTick += 1;
    },
    beginSendUi() {
      this.captureSendingPreview();
      deviceSendUxMixin.methods.beginSendUi.call(this);
    },
    endSendUi() {
      deviceSendUxMixin.methods.endSendUi.call(this);
    },
    applyConfig(config) {
      const visibleSnakeColor = resolveVisibleSnakeColor(
        config.snakeSkin,
        config.snakeColor,
      );
      this.speed = config.speed;
      this.snakeWidth = config.snakeWidth;
      this.snakeColor = visibleSnakeColor;
      this.foodColor = config.foodColor;
      this.font = config.font;
      this.showSeconds = config.showSeconds;
      this.snakeSkin = config.snakeSkin;
    },
    buildCurrentConfig() {
      return {
        speed: this.speed,
        snakeWidth: this.snakeWidth,
        snakeColor: this.snakeColor,
        foodColor: this.foodColor,
        font: this.font,
        showSeconds: this.showSeconds,
        snakeSkin: this.snakeSkin,
      };
    },
    handleBack() {
      uni.navigateBack();
    },
    switchTab(tabIndex) {
      this.currentTab = tabIndex;
    },
    initPreviewCanvas() {
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;
      const fallbackBottomTabsHeight = 76;

      this.$nextTick(() => {
        setTimeout(() => {
          const query = uni.createSelectorQuery().in(this);
          query.select(".canvas-section").boundingClientRect();
          query.select(".bottom-tabs").boundingClientRect();
          query.select(".preview-canvas-container").boundingClientRect();
          query.exec((results) => {
            const [sectionRect, tabsRect, previewRect] = Array.isArray(results)
              ? results
              : [];

            if (sectionRect && sectionRect.height) {
              const nextHeight =
                systemInfo.windowHeight -
                statusBarHeight -
                88 -
                sectionRect.height;
              const bottomTabsHeight =
                tabsRect && tabsRect.height
                  ? tabsRect.height
                  : fallbackBottomTabsHeight;
              this.contentHeight = `${Math.max(120, nextHeight - bottomTabsHeight)}px`;
            }

            if (!previewRect || !previewRect.width) {
              this.previewCanvasReady = true;
              this.schedulePreviewRefresh();
              return;
            }

            const fitZoom = Math.max(
              2,
              Math.floor((previewRect.width * 0.96) / 64),
            );
            this.previewContainerSize = {
              width: previewRect.width,
              height: previewRect.width,
            };
            this.previewZoom = fitZoom;
            this.previewOffset = {
              x: (previewRect.width - 64 * fitZoom) / 2,
              y: (previewRect.width - 64 * fitZoom) / 2,
            };
            this.previewCanvasReady = true;
            this.schedulePreviewRefresh();
          });
        }, 80);
      });
    },
    resetPreviewFrames() {
      this.stopPreviewPlayback();
      this.previewFrameMaps = [];
      this.previewFrameDelays = [];
      this.previewFrameIndex = 0;
    },
    schedulePreviewRefresh() {
      if (!this.previewCanvasReady) {
        return;
      }
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
      this.previewRefreshTimer = setTimeout(() => {
        this.refreshPreviewFrames();
      }, 100);
    },
    refreshPreviewFrames() {
      const previewSequence = buildLedMatrixPreviewSequence({
        demoId: "snake",
        speed: this.speed,
        snakeWidth: this.snakeWidth,
        snakeColor: this.snakeColor,
        foodColor: this.foodColor,
        font: this.font,
        showSeconds: this.showSeconds,
        snakeSkin: this.snakeSkin,
      });
      this.previewFrameMaps = Array.isArray(previewSequence.maps)
        ? previewSequence.maps
        : [];
      this.previewFrameDelays = Array.isArray(previewSequence.delays)
        ? previewSequence.delays
        : [];
      this.previewFrameIndex = 0;
      this.startPreviewPlayback();
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (this.previewFrameMaps.length <= 1) {
        return;
      }
      const playNext = () => {
        const currentDelay = this.previewFrameDelays[this.previewFrameIndex];
        const delay = typeof currentDelay === "number" ? currentDelay : 120;
        this.previewTimer = setTimeout(() => {
          if (this.previewFrameMaps.length === 0) {
            return;
          }
          this.previewFrameIndex =
            this.previewFrameIndex + 1 >= this.previewFrameMaps.length
              ? 0
              : this.previewFrameIndex + 1;
          playNext();
        }, delay);
      };
      playNext();
    },
    stopPreviewPlayback() {
      if (this.previewTimer) {
        clearTimeout(this.previewTimer);
        this.previewTimer = null;
      }
    },
    cleanupPreviewTimers() {
      this.stopPreviewPlayback();
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
    },
    handleSpeedChange(event) {
      this.speed = Number(event.detail.value);
    },
    handleSnakeWidthChange(event) {
      this.snakeWidth = Number(event.detail.value);
    },
    handleFoodColorChange(value) {
      this.foodColor = value;
    },
    selectSnakeSkin(value) {
      if (this.snakeSkin === value) {
        return;
      }
      this.snakeColor = pickRandomSnakeColorBySkin(value, this.snakeColor);
      this.snakeSkin = value;
    },
    randomizeSkinColor() {
      this.snakeColor = pickRandomSnakeColorBySkin(
        this.snakeSkin,
        this.snakeColor,
      );
    },
    selectFont(fontId) {
      this.font = fontId;
    },
    setShowSeconds(value) {
      this.showSeconds = value === true;
    },
    loadSavedConfig() {
      const savedConfig = readSavedSnakeModeConfig();
      if (savedConfig) {
        const visibleConfig = cloneSnakeModeConfig(savedConfig);
        visibleConfig.snakeColor = resolveVisibleSnakeColor(
          visibleConfig.snakeSkin,
          visibleConfig.snakeColor,
        );
        this.applyConfig(visibleConfig);
        return;
      }
      this.applyConfig(createDefaultSnakeModeConfig());
    },
    saveConfig() {
      const currentConfig = this.buildCurrentConfig();
      uni.setStorageSync(SNAKE_MODE_CONFIG_KEY, currentConfig);
    },
    async saveAndApply() {
      if (!this.guardBeforeSend(this.deviceStore.connected)) {
        return;
      }

      this.beginSendUi();
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.startSnake({
          speed: this.speed,
          snakeWidth: this.snakeWidth,
          snakeColor: this.snakeColor,
          foodColor: this.foodColor,
          font: this.font,
          showSeconds: this.showSeconds,
          snakeSkin: this.snakeSkin,
        });
        this.saveConfig();
        this.showSendSuccess();
      } catch (error) {
        console.error("发送贪吃蛇失败:", error);
        this.showSendFailure(error);
      } finally {
        this.endSendUi();
      }
    },
  },
};
</script>

<style scoped>
.snake-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-note {
  font-size: 20rpx;
  line-height: 1.2;
  color: var(--text-secondary);
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.tab-panel {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.tab-section-head {
  align-items: center;
}

.tab-section-title {
  font-size: 28rpx;
}

.tab-section-meta {
  font-size: 22rpx;
  line-height: 1.2;
  color: var(--text-secondary);
}

.card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.color-picker-row {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.snake-top-card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
  padding: 0 8rpx 8rpx;
}

.snake-top-head {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-bottom: 14rpx;
}

.skin-toolbar {
  display: grid;
  grid-template-columns: minmax(0, 1fr);
  gap: 12rpx;
  margin-bottom: 14rpx;
}

.skin-toolbar-btn {
  min-height: 78rpx;
  padding: 14rpx 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  text-align: center;
}

.skin-toolbar-btn.disabled {
  opacity: 0.42;
}

.skin-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12rpx;
}

.skin-option {
  min-height: 0;
  padding: 20rpx 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  text-align: center;
}

.skin-option .glx-feature-option__label,
.skin-toolbar-btn .glx-feature-option__label {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100%;
  white-space: nowrap;
}
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

.bottom-tab-item.active .bottom-tab-text {
  color: #000000;
  font-weight: 900;
  font-size: 22rpx;
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}
</style>

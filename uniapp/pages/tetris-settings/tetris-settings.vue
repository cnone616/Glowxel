<template>
  <view class="tetris-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">俄罗斯方块屏保</text>
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
          :grid-visible="true"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="tetrisScreensaverPreviewCanvas"
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
          :grid-visible="true"
          :is-dark-mode="true"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">预览效果</text>
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
        <view v-show="currentTab === 1">
          <view class="card glx-panel-card glx-editor-card tetris-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">模式</text>
            </view>
            <view class="option-row option-row-double">
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.clearMode }"
                @click="config.clearMode = true"
              >
                <text>消除模式</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: !config.clearMode }"
                @click="config.clearMode = false"
              >
                <text>满屏模式</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card glx-editor-card tetris-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">方块大小</text>
            </view>
            <view class="option-row option-row-triple">
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.cellSize === 1 }"
                @click="config.cellSize = 1"
              >
                <text>小 (1px)</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.cellSize === 2 }"
                @click="config.cellSize = 2"
              >
                <text>中 (2px)</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.cellSize === 3 }"
                @click="config.cellSize = 3"
              >
                <text>大 (3px)</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card glx-editor-card tetris-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">下落速度</text>
            </view>
            <view class="option-row option-row-triple">
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.speed === 'slow' }"
                @click="config.speed = 'slow'"
              >
                <text>慢</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.speed === 'normal' }"
                @click="config.speed = 'normal'"
              >
                <text>中</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.speed === 'fast' }"
                @click="config.speed = 'fast'"
              >
                <text>快</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card glx-editor-card tetris-section-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">时间显示</text>
            </view>
            <view class="option-row option-row-double">
              <view
                class="option-btn glx-feature-option"
                :class="{ active: config.showClock }"
                @click="config.showClock = true"
              >
                <text>显示时间</text>
              </view>
              <view
                class="option-btn glx-feature-option"
                :class="{ active: !config.showClock }"
                @click="config.showClock = false"
              >
                <text>隐藏时间</text>
              </view>
            </view>
          </view>

        </view>

        <ClockTextSettingsCard
          v-show="currentTab === 2"
          icon-name="time"
          title="时间显示"
          :section="effectiveTimeSection"
          :preset-colors="presetColors"
          :show-font-size="true"
          :show-seconds-control="true"
          :show-seconds="clockConfig.showSeconds"
          :min-font-size="1"
          :max-font-size="3"
          :show-toggle="false"
          @toggle-seconds="toggleShowSeconds"
          @adjust="handleTimeAdjust"
          @update-color="handleTimeColor"
          @set-align="handleTimeAlign"
        />

        <ClockFontPanel
          v-show="currentTab === 3"
          :font-options="fontOptions"
          :selected-font="clockConfig.font"
          :show-seconds="clockConfig.showSeconds"
          :hour-format="clockConfig.hourFormat"
          @select-font="selectFont"
          @set-hour-format="setHourFormat"
        />
      </view>
    </scroll-view>

    <view class="bottom-tabs">
      <view
        v-for="tab in tabDefinitions"
        :key="tab.index"
        class="bottom-tab-item"
        :class="{ active: currentTab === tab.index }"
        @click="currentTab = tab.index"
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
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import deviceSendUxMixin from "../../mixins/deviceSendUxMixin.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import PixelPreviewBoard from "../../components/PixelPreviewBoard.vue";
import ClockFontPanel from "../../components/clock-editor/ClockFontPanel.vue";
import ClockTextSettingsCard from "../../components/clock-editor/ClockTextSettingsCard.vue";
import {
  createTetrisScreensaverPreviewState,
  renderTetrisScreensaverPreviewState,
  stepTetrisScreensaverPreviewState,
} from "../../utils/tetrisScreensaverPreview.js";
import {
  getClockFontOptions,
  getClockTextWidth,
  getCurrentTimeText,
} from "../../utils/clockCanvas.js";

const TETRIS_SPEED_OPTIONS = {
  slow: 300,
  normal: 150,
  fast: 80,
};
const TETRIS_ALL_PIECES = Object.freeze([0, 1, 2, 3, 4, 5, 6]);
const TETRIS_CONFIG_STORAGE_KEY = "tetris_config";
const TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY = "tetris_overlay_clock_config";

function cloneAllTetrisPieces() {
  return TETRIS_ALL_PIECES.slice();
}

function createDefaultConfig() {
  return {
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: cloneAllTetrisPieces(),
  };
}

function createDefaultClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 1,
      x: 32,
      y: 2,
      color: "#ffffff",
      align: "center",
    },
    date: {
      show: false,
      fontSize: 1,
      x: 0,
      y: 0,
      color: "#787878",
      align: "left",
    },
    week: {
      show: false,
      x: 0,
      y: 0,
      color: "#646464",
      align: "left",
    },
    image: {
      show: false,
      x: 0,
      y: 0,
      width: 64,
      height: 64,
    },
  };
}

function normalizeSavedConfig(saved) {
  const base = createDefaultConfig();
  if (!saved || typeof saved !== "object") {
    return base;
  }

  const normalized = createDefaultConfig();
  if (typeof saved.clearMode === "boolean") {
    normalized.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    normalized.cellSize = saved.cellSize;
  }
  if (saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast") {
    normalized.speed = saved.speed;
  }
  if (typeof saved.showClock === "boolean") {
    normalized.showClock = saved.showClock;
  }
  return normalized;
}

export default {
  mixins: [statusBarMixin, deviceSendUxMixin],
  components: {
    Icon,
    Toast,
    GlxInlineLoader,
    PixelCanvas,
    PixelPreviewBoard,
    ClockFontPanel,
    ClockTextSettingsCard,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewState: null,
      currentPreviewMap: new Map(),
      sendingPreviewPixels: new Map(),
      sendingPreviewTick: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      config: createDefaultConfig(),
      clockConfig: createDefaultClockConfig(),
      fontOptions: getClockFontOptions(),
      currentTab: 1,
      tabDefinitions: [
        { index: 1, label: "屏保", icon: "picture" },
        { index: 2, label: "时间", icon: "time" },
        { index: 3, label: "字体", icon: "text" },
      ],
      presetColors: [
        { name: "青色", hex: "#64c8ff" },
        { name: "绿色", hex: "#00ff9d" },
        { name: "黄色", hex: "#ffdc00" },
        { name: "橙色", hex: "#ffa500" },
        { name: "红色", hex: "#ff6464" },
        { name: "紫色", hex: "#c864ff" },
        { name: "白色", hex: "#ffffff" },
      ],
    };
  },
  computed: {
    currentPreviewPixels() {
      return this.currentPreviewMap;
    },
    effectiveTimeSection() {
      return {
        ...this.clockConfig.time,
        show: this.config.showClock,
      };
    },
    previewCanvasBoxStyle() {
      return {
        height: `${this.previewContainerSize.height}px`,
      };
    },
  },
  watch: {
    config: {
      handler() {
        this.schedulePreviewRefresh();
      },
      deep: true,
    },
    clockConfig: {
      handler() {
        this.schedulePreviewRefresh();
      },
      deep: true,
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    const saved = uni.getStorageSync(TETRIS_CONFIG_STORAGE_KEY);
    this.config = normalizeSavedConfig(saved);
    const savedClockConfig = uni.getStorageSync(
      TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY,
    );
    if (savedClockConfig && typeof savedClockConfig === "object") {
      this.clockConfig = savedClockConfig;
    }
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
    handleBack() {
      uni.navigateBack();
    },
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
              systemInfo.windowHeight -
              statusBarHeight -
              88 -
              sectionRect.height;
            this.contentHeight = `${Math.max(120, nextHeight)}px`;
          });
          query
            .select(".preview-canvas-container")
            .boundingClientRect((rect) => {
              if (!rect || !rect.width) {
                this.previewCanvasReady = true;
                this.schedulePreviewRefresh();
                return;
              }
              const fitZoom = Math.max(2, Math.floor((rect.width * 0.96) / 64));
              this.previewContainerSize = {
                width: rect.width,
                height: rect.width,
              };
              this.previewZoom = fitZoom;
              this.previewOffset = {
                x: (rect.width - 64 * fitZoom) / 2,
                y: (rect.width - 64 * fitZoom) / 2,
              };
              this.previewCanvasReady = true;
              this.schedulePreviewRefresh();
            })
            .exec();
        }, 80);
      });
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
        const previewConfig = {
          ...this.config,
          pieces: cloneAllTetrisPieces(),
        };
        this.previewState = createTetrisScreensaverPreviewState(
          previewConfig,
          this.buildEffectiveClockConfig(),
        );
        this.currentPreviewMap = renderTetrisScreensaverPreviewState(this.previewState);
        this.startPreviewPlayback();
      }, 80);
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (!this.previewState) {
        return;
      }
      const playNext = () => {
        const delay = Number.isFinite(this.previewState.frameDelay)
          ? this.previewState.frameDelay
          : 110;
        this.previewTimer = setTimeout(() => {
          if (!this.previewState) {
            return;
          }
          stepTetrisScreensaverPreviewState(this.previewState);
          this.currentPreviewMap = renderTetrisScreensaverPreviewState(this.previewState);
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
      this.previewState = null;
    },
    buildEffectiveClockConfig() {
      return {
        ...this.clockConfig,
        time: {
          ...this.clockConfig.time,
          show: this.config.showClock,
        },
      };
    },
    getTimeText(clockConfig = this.clockConfig) {
      return getCurrentTimeText(
        clockConfig.showSeconds,
        clockConfig.hourFormat,
      );
    },
    selectFont(fontId) {
      this.clockConfig.font = fontId;
    },
    toggleShowSeconds() {
      this.clockConfig.showSeconds = !this.clockConfig.showSeconds;
    },
    setHourFormat(hourFormat) {
      this.clockConfig.hourFormat = hourFormat;
    },
    handleTimeAdjust(key, delta, min, max) {
      const currentValue = this.clockConfig.time[key];
      const nextValue = Math.max(min, Math.min(max, currentValue + delta));
      if (nextValue !== currentValue) {
        this.clockConfig.time[key] = nextValue;
      }
    },
    handleTimeColor(color) {
      this.clockConfig.time.color = color;
    },
    handleTimeAlign(align) {
      this.clockConfig.time.align = align;
      if (align === "left") {
        this.clockConfig.time.x = 0;
      } else if (align === "center") {
        this.clockConfig.time.x = 32;
      } else if (align === "right") {
        this.clockConfig.time.x = 63;
      }
    },
    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16),
      };
    },
    buildTetrisClockPayload(clockConfig = this.buildEffectiveClockConfig()) {
      const timeText = this.getTimeText(clockConfig);
      let timeX = clockConfig.time.x;

      if (clockConfig.time.align === "center") {
        timeX =
          timeX -
          Math.floor(
            getClockTextWidth(
              timeText,
              clockConfig.font,
              clockConfig.time.fontSize,
            ) / 2,
          );
      } else if (clockConfig.time.align === "right") {
        timeX =
          timeX -
          getClockTextWidth(
            timeText,
            clockConfig.font,
            clockConfig.time.fontSize,
          );
      }

      return {
        font: clockConfig.font,
        showSeconds: clockConfig.showSeconds,
        hourFormat: clockConfig.hourFormat,
        time: {
          show: clockConfig.time.show,
          fontSize: clockConfig.time.fontSize,
          x: timeX,
          y: clockConfig.time.y,
          color: this.hexToRgb(clockConfig.time.color),
        },
        date: {
          show: clockConfig.date.show,
          fontSize: clockConfig.date.fontSize,
          x: clockConfig.date.x,
          y: clockConfig.date.y,
          color: this.hexToRgb(clockConfig.date.color),
        },
        week: {
          show: clockConfig.week.show,
          x: clockConfig.week.x,
          y: clockConfig.week.y,
          color: this.hexToRgb(clockConfig.week.color),
        },
        image: {
          show: clockConfig.image.show,
          x: clockConfig.image.x,
          y: clockConfig.image.y,
          width: clockConfig.image.width,
          height: clockConfig.image.height,
        },
      };
    },
    async saveAndApply() {
      if (!this.guardBeforeSend(this.deviceStore.connected)) {
        return;
      }

      const nextConfig = {
        clearMode: this.config.clearMode,
        cellSize: this.config.cellSize,
        speed: this.config.speed,
        showClock: this.config.showClock,
        pieces: cloneAllTetrisPieces(),
      };
      const nextClockConfig = this.buildEffectiveClockConfig();
      this.config = nextConfig;
      this.clockConfig = nextClockConfig;
      uni.setStorageSync(TETRIS_CONFIG_STORAGE_KEY, nextConfig);
      uni.setStorageSync(
        TETRIS_OVERLAY_CLOCK_CONFIG_STORAGE_KEY,
        nextClockConfig,
      );

      const previousMode = this.deviceStore.deviceMode;
      this.beginSendUi();
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.startTetris({
          config: this.buildTetrisClockPayload(nextClockConfig),
          clearMode: nextConfig.clearMode,
          cellSize: nextConfig.cellSize,
          speed: TETRIS_SPEED_OPTIONS[nextConfig.speed],
          showClock: nextConfig.showClock,
          pieces: nextConfig.pieces,
        });
        this.showSendSuccess("已应用");
      } catch (err) {
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode: "tetris",
        });
        console.error("发送失败:", err);
        this.showSendFailure(err);
      } finally {
        this.endSendUi();
      }
    },
  },
};
</script>

<style scoped>
.tetris-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
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

.tetris-section-card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.option-row-double {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.option-row-triple {
  grid-template-columns: repeat(3, minmax(0, 1fr));
  box-sizing: border-box;
}

.option-btn.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  color: var(--nb-ink) !important;
}

.option-btn.glx-feature-option.active text {
  color: var(--nb-ink) !important;
  font-weight: 900 !important;
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

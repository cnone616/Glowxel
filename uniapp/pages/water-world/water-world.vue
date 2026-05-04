<template>
  <view class="water-world-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">水世界</text>
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
          :grid-visible="false"
          :is-dark-mode="true"
          :touch-enabled="false"
          canvas-id="waterWorldPreviewCanvas"
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
          :grid-visible="false"
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
            @click="handleSend"
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
        <view v-show="currentTab === 1" class="water-tab-panel">
          <view class="settings-card glx-panel-card water-section-card">
            <view class="route-grid">
              <view
                v-for="option in waterOptions"
                :key="option.preset"
                class="water-option glx-feature-option glx-feature-option--scene"
                :class="{ active: config.preset === option.preset }"
                @click="applyPreset(option.preset)"
              >
                <text class="glx-feature-option__label">{{ option.label }}</text>
              </view>
            </view>
            <view class="water-toolbar">
              <text class="water-toolbar-label">海水颜色</text>
              <view
                class="action-btn-sm glx-secondary-action water-random-btn"
                :class="{ disabled: isSending }"
                @click="handleRandomColorTheme"
              >
                <Icon name="palette" :size="32" color="var(--nb-ink)" />
                <text>随机海色</text>
              </view>
            </view>
            <view class="color-theme-grid">
              <view
                v-for="theme in colorThemeOptions"
                :key="theme.id"
                class="water-color-option glx-feature-option"
                :class="{ active: colorThemeId === theme.id }"
                @click="applyColorTheme(theme.id)"
              >
                <view class="water-color-swatches">
                  <view
                    v-for="swatch in theme.swatches"
                    :key="`${theme.id}-${swatch}`"
                    class="water-color-dot"
                    :style="{ backgroundColor: swatch }"
                  ></view>
                </view>
                <text class="glx-feature-option__label">{{ theme.label }}</text>
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

    <Toast
      ref="toastRef"
      @show="handleToastShow"
      @hide="handleToastHide"
    />
  </view>
</template>

<script>
import deviceSendUxMixin from "../../mixins/deviceSendUxMixin.js";
import { useDeviceStore } from "../../store/device.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import PixelPreviewBoard from "../../components/PixelPreviewBoard.vue";
import ClockFontPanel from "../../components/clock-editor/ClockFontPanel.vue";
import ClockTextSettingsCard from "../../components/clock-editor/ClockTextSettingsCard.vue";
import { useToast } from "../../composables/useToast.js";
import {
  drawClockTextToPixels,
  getClockFontOptions,
  getCurrentTimeText,
  getClockTextHeight,
  getClockTextWidth,
} from "../../utils/clockCanvas.js";
import {
  DEFAULT_WATER_WORLD_COLOR_THEME_ID,
  WATER_WORLD_COLOR_THEME_OPTIONS,
  createWaterWorldPreviewState,
  renderWaterWorldPreviewState,
  stepWaterWorldPreviewState,
} from "../../utils/waterWorldPreview.js";

const WATER_WORLD_CONFIG_KEY = "water_world_preview_config";
const WATER_WORLD_CLOCK_CONFIG_KEY = "water_world_preview_clock_config";
const WATER_WORLD_OPTIONS = Object.freeze([
  { preset: "surface", label: "海面波浪" },
  { preset: "current", label: "深海海流" },
  { preset: "caustics", label: "海底焦散" },
]);
const WATER_WORLD_FONT_OPTIONS = getClockFontOptions();
const WATER_WORLD_FONT_IDS = Object.freeze(
  WATER_WORLD_FONT_OPTIONS.map((item) => item.id),
);
const PRESET_COLORS = Object.freeze([
  { name: "青色", hex: "#64c8ff" },
  { name: "绿色", hex: "#00ff9d" },
  { name: "黄色", hex: "#ffdc00" },
  { name: "橙色", hex: "#ffa500" },
  { name: "红色", hex: "#ff6464" },
  { name: "紫色", hex: "#c864ff" },
  { name: "白色", hex: "#ffffff" },
]);
const WATER_WORLD_SEND_MODE = "led_matrix_showcase";
const WATER_WORLD_BOARD_COMMANDS = Object.freeze({
  surface: Object.freeze({
    preset: "surface",
    speed: 4,
    loop: true,
  }),
  current: Object.freeze({
    preset: "current",
    speed: 5,
    loop: true,
  }),
  caustics: Object.freeze({
    preset: "caustics",
    speed: 4,
    loop: true,
  }),
});

function isBoardReadyPreset(preset) {
  return Object.prototype.hasOwnProperty.call(
    WATER_WORLD_BOARD_COMMANDS,
    preset,
  );
}

function buildWaterWorldSendPlan(preset) {
  if (!isBoardReadyPreset(preset)) {
    throw new Error("当前路线暂未接入设备");
  }
  const command = WATER_WORLD_BOARD_COMMANDS[preset];
  return {
    type: "command",
    deviceMode: WATER_WORLD_SEND_MODE,
    command: {
      cmd: "set_ambient_effect",
      preset: command.preset,
      speed: command.speed,
      loop: command.loop,
    },
  };
}

function createDefaultWaterWorldConfig() {
  return {
    preset: "surface",
  };
}

function createDefaultClockConfig() {
  return {
    font: "classic_5x7",
    showSeconds: false,
    hourFormat: 24,
    time: {
      show: true,
      fontSize: 2,
      x: 32,
      y: 7,
      color: "#ffffff",
      align: "center",
    },
    date: {
      show: false,
      fontSize: 1,
      x: 32,
      y: 22,
      color: "#787878",
      align: "center",
    },
    week: {
      show: false,
      x: 32,
      y: 38,
      color: "#646464",
      align: "center",
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

function isSupportedPreset(value) {
  return WATER_WORLD_OPTIONS.some((item) => item.preset === value);
}

function isSupportedFont(value) {
  return WATER_WORLD_FONT_IDS.includes(value);
}

function normalizeSavedConfig(saved) {
  const normalized = createDefaultWaterWorldConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }
  if (typeof saved.preset === "string" && isSupportedPreset(saved.preset)) {
    normalized.preset = saved.preset;
  }
  return normalized;
}

function resolveWaterWorldTimePlacement(timeText, clockConfig) {
  const fontSize = Math.max(
    1,
    Math.min(3, Number(clockConfig.time.fontSize) || 1),
  );
  const width = getClockTextWidth(timeText, clockConfig.font, fontSize);
  const height = getClockTextHeight(clockConfig.font, fontSize);
  const maxX = Math.max(0, 64 - width);
  const maxY = Math.max(0, 64 - height);
  let x = Number(clockConfig.time.x);
  let y = Number(clockConfig.time.y);

  if (clockConfig.time.align === "center") {
    x -= Math.floor(width / 2);
  } else if (clockConfig.time.align === "right") {
    x -= width;
  }

  if (!Number.isFinite(x)) {
    x = 0;
  }
  if (!Number.isFinite(y)) {
    y = 0;
  }

  return {
    x: Math.max(0, Math.min(maxX, Math.round(x))),
    y: Math.max(0, Math.min(maxY, Math.round(y))),
    fontSize,
  };
}

function drawWaterWorldTimeToPixels(timeText, clockConfig, pixelMap) {
  const placement = resolveWaterWorldTimePlacement(timeText, clockConfig);

  drawClockTextToPixels(
    timeText,
    placement.x,
    placement.y,
    clockConfig.time.color,
    pixelMap,
    clockConfig.font,
    placement.fontSize,
    "left",
  );
}

function resolveAlignedClockX(text, fontId, fontSize, align, x) {
  const width = getClockTextWidth(text, fontId, fontSize);
  if (align === "center") {
    return x - Math.floor(width / 2);
  }
  if (align === "right") {
    return x - width;
  }
  return x;
}

function normalizeSavedClockConfig(saved) {
  const normalized = createDefaultClockConfig();
  if (!saved || typeof saved !== "object") {
    return normalized;
  }

  if (typeof saved.font === "string" && isSupportedFont(saved.font)) {
    normalized.font = saved.font;
  }
  if (typeof saved.showSeconds === "boolean") {
    normalized.showSeconds = saved.showSeconds;
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    normalized.hourFormat = saved.hourFormat;
  }

  if (saved.time && typeof saved.time === "object") {
    if (saved.time.show === true) {
      normalized.time.show = true;
    }
    if (
      Number.isFinite(Number(saved.time.fontSize)) &&
      Number(saved.time.fontSize) >= 1 &&
      Number(saved.time.fontSize) <= 3
    ) {
      normalized.time.fontSize = Number(saved.time.fontSize);
    }
    if (Number.isFinite(Number(saved.time.x))) {
      normalized.time.x = Math.max(0, Math.min(63, Number(saved.time.x)));
    }
    if (Number.isFinite(Number(saved.time.y))) {
      normalized.time.y = Math.max(0, Math.min(63, Number(saved.time.y)));
    }
    if (typeof saved.time.color === "string") {
      normalized.time.color = saved.time.color;
    }
    if (
      saved.time.align === "left" ||
      saved.time.align === "center" ||
      saved.time.align === "right"
    ) {
      normalized.time.align = saved.time.align;
    }
  }

  if (saved.date && typeof saved.date === "object") {
    if (saved.date.show === true || saved.date.show === false) {
      normalized.date.show = saved.date.show;
    }
    if (
      Number.isFinite(Number(saved.date.fontSize)) &&
      Number(saved.date.fontSize) >= 1 &&
      Number(saved.date.fontSize) <= 3
    ) {
      normalized.date.fontSize = Number(saved.date.fontSize);
    }
    if (Number.isFinite(Number(saved.date.x))) {
      normalized.date.x = Math.max(0, Math.min(63, Number(saved.date.x)));
    }
    if (Number.isFinite(Number(saved.date.y))) {
      normalized.date.y = Math.max(0, Math.min(63, Number(saved.date.y)));
    }
    if (typeof saved.date.color === "string") {
      normalized.date.color = saved.date.color;
    }
    if (
      saved.date.align === "left" ||
      saved.date.align === "center" ||
      saved.date.align === "right"
    ) {
      normalized.date.align = saved.date.align;
    }
  }

  if (saved.week && typeof saved.week === "object") {
    if (saved.week.show === true || saved.week.show === false) {
      normalized.week.show = saved.week.show;
    }
    if (Number.isFinite(Number(saved.week.x))) {
      normalized.week.x = Math.max(0, Math.min(63, Number(saved.week.x)));
    }
    if (Number.isFinite(Number(saved.week.y))) {
      normalized.week.y = Math.max(0, Math.min(63, Number(saved.week.y)));
    }
    if (typeof saved.week.color === "string") {
      normalized.week.color = saved.week.color;
    }
    if (
      saved.week.align === "left" ||
      saved.week.align === "center" ||
      saved.week.align === "right"
    ) {
      normalized.week.align = saved.week.align;
    }
  }

  if (saved.image && typeof saved.image === "object") {
    if (saved.image.show === true || saved.image.show === false) {
      normalized.image.show = saved.image.show;
    }
    if (Number.isFinite(Number(saved.image.x))) {
      normalized.image.x = Math.max(0, Math.min(63, Number(saved.image.x)));
    }
    if (Number.isFinite(Number(saved.image.y))) {
      normalized.image.y = Math.max(0, Math.min(63, Number(saved.image.y)));
    }
    if (Number.isFinite(Number(saved.image.width))) {
      normalized.image.width = Math.max(0, Math.min(64, Number(saved.image.width)));
    }
    if (Number.isFinite(Number(saved.image.height))) {
      normalized.image.height = Math.max(0, Math.min(64, Number(saved.image.height)));
    }
  }

  return normalized;
}

function pickRandomWaterWorldColorThemeId(currentThemeId) {
  const candidates = WATER_WORLD_COLOR_THEME_OPTIONS.filter((theme) => {
    return theme.id !== currentThemeId;
  });
  const pool =
    candidates.length > 0 ? candidates : WATER_WORLD_COLOR_THEME_OPTIONS;
  const nextIndex = Math.floor(Math.random() * pool.length);
  return pool[nextIndex].id;
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
      waterOptions: WATER_WORLD_OPTIONS,
      colorThemeOptions: WATER_WORLD_COLOR_THEME_OPTIONS,
      colorThemeId: DEFAULT_WATER_WORLD_COLOR_THEME_ID,
      config: createDefaultWaterWorldConfig(),
      clockConfig: createDefaultClockConfig(),
      fontOptions: WATER_WORLD_FONT_OPTIONS,
      currentTab: 1,
      tabDefinitions: [
        { index: 1, label: "水世界", icon: "prompt" },
        { index: 2, label: "时间", icon: "time" },
        { index: 3, label: "字体", icon: "text" },
      ],
      presetColors: PRESET_COLORS,
    };
  },
  computed: {
    currentPreviewPixels() {
      return this.currentPreviewMap;
    },
    previewCanvasBoxStyle() {
      return {
        height: `${this.previewContainerSize.height}px`,
      };
    },
    effectiveTimeSection() {
      return {
        ...this.clockConfig.time,
        show: true,
      };
    },
  },
  watch: {
    config: {
      deep: true,
      handler() {
        this.persistLocalState();
        this.schedulePreviewRefresh();
      },
    },
    clockConfig: {
      deep: true,
      handler() {
        this.persistLocalState();
        this.schedulePreviewRefresh();
      },
    },
    colorThemeId() {
      this.schedulePreviewRefresh();
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    const savedConfig = normalizeSavedConfig(
      uni.getStorageSync(WATER_WORLD_CONFIG_KEY),
    );
    this.config = savedConfig;
    this.clockConfig = normalizeSavedClockConfig(
      uni.getStorageSync(WATER_WORLD_CLOCK_CONFIG_KEY),
    );
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
  },
  async onShow() {
    if (!this.deviceStore || !this.deviceStore.connected) {
      return;
    }
    const status = await this.deviceStore.syncDeviceStatus();
    this.syncPresetFromDeviceStatus(status);
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
    persistLocalState() {
      uni.setStorageSync(WATER_WORLD_CONFIG_KEY, {
        preset: this.config.preset,
      });
      uni.setStorageSync(WATER_WORLD_CLOCK_CONFIG_KEY, {
        font: this.clockConfig.font,
        showSeconds: this.clockConfig.showSeconds,
        hourFormat: this.clockConfig.hourFormat,
        time: {
          show: this.clockConfig.time.show,
          fontSize: this.clockConfig.time.fontSize,
          x: this.clockConfig.time.x,
          y: this.clockConfig.time.y,
          color: this.clockConfig.time.color,
          align: this.clockConfig.time.align,
        },
        date: {
          show: this.clockConfig.date.show,
          fontSize: this.clockConfig.date.fontSize,
          x: this.clockConfig.date.x,
          y: this.clockConfig.date.y,
          color: this.clockConfig.date.color,
          align: this.clockConfig.date.align,
        },
        week: {
          show: this.clockConfig.week.show,
          x: this.clockConfig.week.x,
          y: this.clockConfig.week.y,
          color: this.clockConfig.week.color,
          align: this.clockConfig.week.align,
        },
        image: {
          show: this.clockConfig.image.show,
          x: this.clockConfig.image.x,
          y: this.clockConfig.image.y,
          width: this.clockConfig.image.width,
          height: this.clockConfig.image.height,
        },
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
        this.previewState = createWaterWorldPreviewState(
          this.config,
          this.colorThemeId,
        );
        this.currentPreviewMap = this.buildPreviewPixels();
        this.startPreviewPlayback();
      }, 60);
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (!this.previewState) {
        return;
      }
      const playNext = () => {
        if (!this.previewState) {
          return;
        }
        this.previewTimer = setTimeout(() => {
          if (!this.previewState) {
            return;
          }
          stepWaterWorldPreviewState(this.previewState);
          this.currentPreviewMap = this.buildPreviewPixels();
          playNext();
        }, this.previewState.frameDelay);
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
    buildPreviewPixels() {
      if (!this.previewState) {
        return new Map();
      }
      const pixels = new Map(renderWaterWorldPreviewState(this.previewState));
      const clockConfig = this.buildEffectiveClockConfig();
      const timeText = getCurrentTimeText(
        clockConfig.showSeconds,
        clockConfig.hourFormat,
      );
      drawWaterWorldTimeToPixels(timeText, clockConfig, pixels);
      return pixels;
    },
    buildEffectiveClockConfig() {
      return {
        ...this.clockConfig,
        time: {
          ...this.clockConfig.time,
          show: true,
        },
      };
    },
    hexToRgb(hex) {
      const body = hex.replace(/^#/, "");
      return {
        r: parseInt(body.slice(0, 2), 16),
        g: parseInt(body.slice(2, 4), 16),
        b: parseInt(body.slice(4, 6), 16),
      };
    },
    buildClockConfigPayload() {
      const clockConfig = this.buildEffectiveClockConfig();
      const timeText = getCurrentTimeText(
        clockConfig.showSeconds,
        clockConfig.hourFormat,
      );
      const timeX = resolveAlignedClockX(
        timeText,
        clockConfig.font,
        clockConfig.time.fontSize,
        clockConfig.time.align,
        clockConfig.time.x,
      );
      const dateX = resolveAlignedClockX(
        "00-00",
        clockConfig.font,
        clockConfig.date.fontSize,
        clockConfig.date.align,
        clockConfig.date.x,
      );
      const weekX = resolveAlignedClockX(
        "SUN",
        clockConfig.font,
        1,
        clockConfig.week.align,
        clockConfig.week.x,
      );

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
          x: dateX,
          y: clockConfig.date.y,
          color: this.hexToRgb(clockConfig.date.color),
        },
        week: {
          show: clockConfig.week.show,
          x: weekX,
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
    syncPresetFromDeviceStatus(status) {
      if (!status || typeof status !== "object") {
        return;
      }
      if (status.businessMode !== WATER_WORLD_SEND_MODE) {
        return;
      }
      if (!isBoardReadyPreset(status.effectPreset)) {
        return;
      }
      this.applyPreset(status.effectPreset);
    },
    applyPreset(preset) {
      this.config.preset = preset;
    },
    applyColorTheme(themeId) {
      if (this.isSending || this.colorThemeId === themeId) {
        return;
      }
      this.colorThemeId = themeId;
    },
    handleRandomColorTheme() {
      if (this.isSending) {
        return;
      }
      this.colorThemeId = pickRandomWaterWorldColorThemeId(this.colorThemeId);
    },
    selectFont(fontId) {
      this.clockConfig.font = fontId;
    },
    toggleShowSeconds() {
      this.clockConfig.showSeconds = !this.clockConfig.showSeconds;
    },
    setShowSeconds(showSeconds) {
      this.clockConfig.showSeconds = showSeconds;
    },
    setHourFormat(hourFormat) {
      this.clockConfig.hourFormat = hourFormat;
    },
    handleTimeAdjust(key, delta, min, max) {
      const currentValue = this.clockConfig.time[key];
      const nextMax = key === "x" || key === "y" ? Math.min(max, 63) : max;
      const nextValue = Math.max(min, Math.min(nextMax, currentValue + delta));
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
    async handleSend() {
      if (!this.guardBeforeSend(this.deviceStore && this.deviceStore.connected)) {
        return;
      }

      const previousMode = this.deviceStore.deviceMode;
      let expectedMode = WATER_WORLD_SEND_MODE;
      this.beginSendUi();

      try {
        const sendPlan = buildWaterWorldSendPlan(this.config.preset);
        expectedMode = sendPlan.deviceMode;
        const ws = this.deviceStore.getWebSocket();
        await ws.setAmbientEffect(sendPlan.command, {
          clockConfig: this.buildClockConfigPayload(),
        });
        const status = await this.deviceStore.syncDeviceStatus();
        this.syncPresetFromDeviceStatus(status);
        this.showSendSuccess();
      } catch (error) {
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode,
        });
        this.showSendFailure(error);
      } finally {
        this.endSendUi();
      }
    },
  },
};
</script>

<style scoped>
.water-world-page {
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

.water-tab-panel {
  padding-top: 0;
}

.water-section-card {
  background-color: transparent;
  border: 0;
  padding: 8rpx 12rpx 14rpx;
  margin-bottom: 16rpx;
  box-shadow: none;
}

.route-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12rpx;
}

.water-option {
  min-height: 88rpx;
  padding: 10rpx 8rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
}

.water-option .glx-feature-option__label {
  text-align: center;
  font-size: 24rpx;
  line-height: 1.2;
  white-space: nowrap;
}

.water-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
  margin-top: 16rpx;
}

.water-toolbar-label {
  flex: 0 0 auto;
  font-size: 22rpx;
  line-height: 1.2;
  font-weight: 800;
  color: var(--text-secondary);
}

.water-random-btn {
  flex: 1;
  min-width: 0;
  height: 72rpx !important;
  padding: 0 14rpx !important;
}

.water-random-btn text {
  font-size: 22rpx !important;
}

.color-theme-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12rpx;
  margin-top: 12rpx;
}

.water-color-option {
  min-height: 88rpx;
  padding: 10rpx 8rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
  box-sizing: border-box;
}

.water-color-swatches {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6rpx;
}

.water-color-dot {
  width: 22rpx;
  height: 22rpx;
  border: 2rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.water-color-option .glx-feature-option__label {
  font-size: 20rpx;
  line-height: 1.2;
  white-space: nowrap;
}

.water-option.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  color: var(--nb-ink) !important;
}

.water-option.glx-feature-option.active .glx-feature-option__label,
.water-color-option.glx-feature-option.active .glx-feature-option__label {
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

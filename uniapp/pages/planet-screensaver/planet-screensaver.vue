<template>
  <view class="planet-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">星球屏保</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container" :style="previewCanvasBoxStyle">
        <PixelCanvas
          v-if="previewCanvasReady && !shouldHidePreview"
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
          canvas-id="planetPreviewCanvas"
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
        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="glx-panel-title">星球类型</text>
          </view>
          <view class="preset-grid">
            <view
              v-for="preset in presetOptions"
              :key="preset.id"
              class="glx-feature-option glx-feature-option--scene"
              :class="{ active: config.preset === preset.id }"
              @click="handlePresetSelect(preset.id)"
            >
              <text class="glx-feature-option__label">{{ preset.label }}</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="glx-panel-title">参数</text>
          </view>

          <view class="bottom-action-row">
            <view
              class="action-btn-sm glx-secondary-action"
              :class="{ disabled: isSending }"
              @click="handleRandomColor"
            >
              <Icon name="palette" :size="32" color="var(--nb-ink)" />
              <text>随机颜色</text>
            </view>
            <view
              class="action-btn-sm glx-secondary-action"
              :class="{ disabled: isSending }"
              @click="handleRandomPlanet"
            >
              <Icon name="refresh" :size="32" color="var(--nb-ink)" />
              <text>随机星球</text>
            </view>
          </view>

          <view class="option-stack">
            <text class="form-label">星球大小</text>
            <view class="option-row option-row-triple">
              <view
                v-for="option in sizeOptions"
                :key="option.id"
                class="option-btn glx-feature-option"
                :class="{ active: config.size === option.id }"
                @click="handleSizeSelect(option.id)"
              >
                <text class="glx-feature-option__label">{{
                  option.label
                }}</text>
              </view>
            </view>
          </view>

          <view class="option-stack">
            <text class="form-label">自转方向</text>
            <view class="option-row option-row-double">
              <view
                v-for="option in directionOptions"
                :key="option.id"
                class="option-btn glx-feature-option"
                :class="{ active: config.direction === option.id }"
                @click="handleDirectionSelect(option.id)"
              >
                <text class="glx-feature-option__label">{{
                  option.label
                }}</text>
              </view>
            </view>
          </view>

          <view class="form-row">
            <text class="form-label">转速 {{ config.speed }}</text>
            <GlxStepper
              :value="config.speed"
              :min="PLANET_PREVIEW_MIN_SPEED"
              :max="PLANET_PREVIEW_MAX_SPEED"
              :step="1"
              @change="handleSpeedChange"
            />
          </view>
        </view>
      </view>
    </scroll-view>

    <view
      v-if="isSending"
      class="glx-device-sending-overlay"
      @touchmove.stop.prevent
    >
      <view class="glx-device-sending-card">
        <view class="glx-device-sending-spinner"></view>
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
import PixelCanvas from "../../components/PixelCanvas.vue";
import GlxStepper from "../../components/GlxStepper.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import {
  PLANET_SCREEN_PRESETS,
  PLANET_PREVIEW_MIN_SPEED,
  PLANET_PREVIEW_MAX_SPEED,
  PLANET_PREVIEW_PLAYBACK_INTERVAL_MS,
  PLANET_SIZE_OPTIONS,
  PLANET_DIRECTION_OPTIONS,
  createDefaultPlanetPreviewConfig,
  createRandomPlanetPreviewSeed,
  createRandomPlanetColorSeed,
  getPlanetPreviewCycleDuration,
  buildPlanetScreensaverPreviewFrame,
} from "../../utils/planetScreensaverPreview.js";

export default {
  mixins: [statusBarMixin, deviceSendUxMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    GlxStepper,
  },
  data() {
    const config = createDefaultPlanetPreviewConfig();
    return {
      deviceStore: null,
      toast: null,
      sendPreviewKind: "native",
      PLANET_PREVIEW_MIN_SPEED,
      PLANET_PREVIEW_MAX_SPEED,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewPlaybackStartedAt: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      presetOptions: PLANET_SCREEN_PRESETS,
      sizeOptions: PLANET_SIZE_OPTIONS,
      directionOptions: PLANET_DIRECTION_OPTIONS,
      config,
    };
  },
  computed: {
    currentPreviewPixels() {
      if (this.previewFrameMaps.length === 0) {
        return new Map();
      }
      return this.previewFrameMaps[0];
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
    isDeviceConnected() {
      if (!this.deviceStore) {
        return false;
      }
      return this.deviceStore.isConnected;
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
  },
  onShow() {
    if (this.previewCanvasReady) {
      this.startPreviewPlayback();
    }
  },
  onHide() {
    this.cleanupPreviewTimers();
  },
  onUnload() {
    this.cleanupPreviewTimers();
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },
    async handleSend() {
      if (!this.guardBeforeSend(this.isDeviceConnected)) {
        return;
      }

      this.beginSendUi();
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setPlanetScreensaver(this.config);
        await this.deviceStore.syncAndRequireBusinessMode(
          "planet_screensaver",
          "设备未进入星球屏保模式",
        );
        this.showSendSuccess();
      } catch (error) {
        console.error("发送星球屏保失败:", error);
        this.showSendFailure(error);
      } finally {
        this.endSendUi();
      }
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
            .boundingClientRect((data) => {
              if (!data || !data.width) {
                this.previewCanvasReady = true;
                this.startPreviewPlayback();
                return;
              }
              const fitZoom = Math.max(2, Math.floor((data.width * 0.96) / 64));
              this.previewContainerSize = {
                width: data.width,
                height: data.width,
              };
              this.previewZoom = fitZoom;
              this.previewOffset = {
                x: (data.width - 64 * fitZoom) / 2,
                y: (data.width - 64 * fitZoom) / 2,
              };
              this.previewCanvasReady = true;
              this.startPreviewPlayback();
            })
            .exec();
        }, 80);
      });
    },
    renderPreviewFrame(progress) {
      const frameMap = buildPlanetScreensaverPreviewFrame(
        this.config,
        progress,
      );
      this.previewFrameMaps = [frameMap];
    },
    schedulePreviewRefresh() {
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
      }
      this.previewRefreshTimer = setTimeout(() => {
        this.previewRefreshTimer = null;
        this.startPreviewPlayback();
      }, 40);
    },
    handlePresetSelect(presetId) {
      if (this.config.preset === presetId) {
        return;
      }
      this.config.preset = presetId;
      this.schedulePreviewRefresh();
    },
    handleSizeSelect(sizeId) {
      if (sizeId === this.config.size) {
        return;
      }
      this.config.size = sizeId;
      this.schedulePreviewRefresh();
    },
    handleRandomColor() {
      this.config.colorSeed = createRandomPlanetColorSeed();
      this.schedulePreviewRefresh();
    },
    handleRandomPlanet() {
      this.config.seed = createRandomPlanetPreviewSeed();
      this.schedulePreviewRefresh();
    },
    handleDirectionSelect(directionId) {
      if (directionId === this.config.direction) {
        return;
      }
      this.config.direction = directionId;
      this.schedulePreviewRefresh();
    },
    handleSpeedChange(event) {
      const nextValue = Number(event?.detail?.value);
      if (!Number.isFinite(nextValue)) {
        return;
      }
      const speed = Math.min(
        PLANET_PREVIEW_MAX_SPEED,
        Math.max(PLANET_PREVIEW_MIN_SPEED, Math.round(nextValue)),
      );
      if (speed === this.config.speed) {
        return;
      }
      this.config.speed = speed;
      this.schedulePreviewRefresh();
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (!this.previewCanvasReady) {
        return;
      }
      this.previewPlaybackStartedAt = Date.now();
      this.renderPreviewFrame(0);

      const tick = () => {
        const cycleDuration = getPlanetPreviewCycleDuration(this.config.speed);
        const elapsed = Date.now() - this.previewPlaybackStartedAt;
        let progress = 0;
        if (cycleDuration > 0) {
          progress = (elapsed % cycleDuration) / cycleDuration;
        }
        this.renderPreviewFrame(progress);
        this.previewTimer = setTimeout(
          tick,
          PLANET_PREVIEW_PLAYBACK_INTERVAL_MS,
        );
      };

      this.previewTimer = setTimeout(tick, PLANET_PREVIEW_PLAYBACK_INTERVAL_MS);
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
  },
};
</script>

<style scoped>
.planet-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.preset-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12rpx;
}

.option-stack {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.option-row {
  display: grid;
  gap: 12rpx;
}

.option-row-double {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.option-row-triple {
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.option-btn.glx-feature-option {
  min-height: 88rpx;
  padding: 10rpx 8rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  background: var(--nb-surface) !important;
  border: 2rpx solid var(--nb-ink) !important;
  box-shadow: var(--nb-shadow-soft) !important;
}

.option-btn.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  box-shadow: var(--nb-shadow-soft) !important;
}

.option-btn.glx-feature-option .glx-feature-option__label {
  font-size: 24rpx;
  line-height: 1.2;
  font-weight: 800;
  color: var(--text-secondary) !important;
}

.option-btn.glx-feature-option.active .glx-feature-option__label {
  color: var(--nb-ink) !important;
  font-weight: 900 !important;
}

.glx-secondary-action {
  background: var(--nb-surface) !important;
  border-color: var(--nb-ink) !important;
}

.bottom-action-row {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12rpx;
}

.bottom-action-row .action-btn-sm {
  width: 100% !important;
  min-width: 0 !important;
  height: 72rpx !important;
  padding: 0 12rpx !important;
}

.bottom-action-row .action-btn-sm text {
  font-size: 22rpx !important;
}

.form-row:last-child {
  margin-bottom: 0;
}
</style>

<template>
  <view class="ambient-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--nb-ink)"
        />
      </view>
      <text class="nav-title glx-topbar__title">像素场景集</text>
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
          canvas-id="ambientPreviewCanvas"
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

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" :style="{ height: contentHeight }">
      <view class="content-wrapper glx-scroll-stack">
        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">场景选择</text>
          </view>
          <view class="scene-grid">
            <view
              v-for="preset in presets"
              :key="preset.value"
              class="glx-feature-option glx-feature-option--scene"
              :class="{ active: config.preset === preset.value }"
              @click="applyPreset(preset)"
            >
              <text class="glx-feature-option__label">{{ preset.label }}</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">场景参数</text>
          </view>

          <view class="form-row">
            <text class="form-label">速度 {{ config.speed }}</text>
            <GlxStepper
              :value="config.speed"
              :min="1"
              :max="10"
              :step="1"
              @change="handleSpeedChange"
            />
          </view>

          <view v-if="!isRainPreset" class="form-row">
            <text class="form-label">强度 {{ config.intensity }}</text>
            <GlxSlider
              :value="config.intensity"
              :min="10"
              :max="100"
              :step="1"
              @change="handleIntensityChange"
            />
          </view>

          <view v-if="isRainPreset" class="form-row">
            <text class="form-label">密度 {{ config.density }}</text>
            <GlxSlider
              :value="config.density"
              :min="10"
              :max="100"
              :step="1"
              @change="handleDensityChange"
            />
          </view>

          <view v-if="isRainPreset" class="form-row color-picker-row">
            <text class="form-label">颜色</text>
            <ColorPanelPicker
              :value="config.color"
              label="雨滴颜色"
              :preset-colors="rainPresetColors"
              @change="handleRainColorChange"
            />
          </view>

          <view class="form-row inline-row">
            <text class="form-label">循环</text>
            <GlxSwitch class="glx-row-switch" :checked="config.loop" @change="config.loop = $event.detail.value" />
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
        <GlxInlineLoader
          class="glx-device-sending-spinner"
          variant="chase"
          size="lg"
        />
        <text class="glx-device-sending-title">正在发送场景...</text>
        <text class="glx-device-sending-tip">设备应用完成前请勿切换页面</text>
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
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import GlxInlineLoader from "../../components/GlxInlineLoader.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import PixelPreviewBoard from "../../components/PixelPreviewBoard.vue";
import GlxSlider from "../../components/GlxSlider.vue";
import GlxStepper from "../../components/GlxStepper.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import GlxSwitch from "../../components/GlxSwitch.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { buildAmbientPreviewFrames } from "../../utils/ambientEffectPreview.js";

const AMBIENT_CONFIG_KEY = "ambient_effect_config";
const SOURCE_AMBIENT_PRESETS = [
  { value: "clock_scene", label: "场景时钟" },
  { value: "starfield", label: "星空漂移" },
  { value: "metablob", label: "液体模拟" },
  { value: "digital_rain", label: "数字雨" },
  { value: "neon_tunnel", label: "霓虹隧道" },
  { value: "boids", label: "群游粒子" },
  { value: "falling_sand", label: "流沙" },
  { value: "sorting_visualizer", label: "排序柱阵" },
  { value: "bouncing_logo", label: "弹跳徽标" },
  { value: "game_of_life", label: "生命游戏" },
  { value: "julia_set", label: "朱莉亚集" },
  { value: "wave_pattern", label: "波纹图案" },
  { value: "watermelon_plasma", label: "西瓜等离子" },
  { value: "rain_scene", label: "雨幕" },
  { value: "sparks", label: "火花" },
];

function createDefaultAmbientConfig() {
  return {
    preset: "digital_rain",
    speed: 6,
    intensity: 72,
    density: 72,
    color: "#64c8ff",
    loop: true,
  };
}

function normalizeAmbientConfig(saved) {
  const base = createDefaultAmbientConfig();
  if (!saved || typeof saved !== "object") {
    return base;
  }

  const normalized = {
    preset: base.preset,
    speed: base.speed,
    intensity: base.intensity,
    density: base.density,
    color: base.color,
    loop: base.loop,
  };

  if (
    typeof saved.preset === "string" &&
    SOURCE_AMBIENT_PRESETS.some((item) => item.value === saved.preset)
  ) {
    normalized.preset = saved.preset;
  }

  if (Number.isFinite(Number(saved.speed))) {
    const speed = Math.round(Number(saved.speed));
    if (speed >= 1 && speed <= 10) {
      normalized.speed = speed;
    }
  }

  if (Number.isFinite(Number(saved.intensity))) {
    const intensity = Math.round(Number(saved.intensity));
    if (intensity >= 10 && intensity <= 100) {
      normalized.intensity = intensity;
    }
  }

  if (Number.isFinite(Number(saved.density))) {
    const density = Math.round(Number(saved.density));
    if (density >= 10 && density <= 100) {
      normalized.density = density;
    }
  }

  if (typeof saved.color === "string") {
    normalized.color = saved.color;
  }

  if (typeof saved.loop === "boolean") {
    normalized.loop = saved.loop;
  }

  return normalized;
}

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    GlxInlineLoader,
    PixelCanvas,
    PixelPreviewBoard,
    GlxSlider,
    GlxStepper,
    ColorPanelPicker,
    GlxSwitch,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      isToastVisible: false,
      isSendPreviewFrozen: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      sendingPreviewPixels: new Map(),
      sendingPreviewTick: 0,
      previewFrameMaps: [],
      previewFrameIndex: 0,
      previewTimer: null,
      presets: SOURCE_AMBIENT_PRESETS,
      config: createDefaultAmbientConfig(),
      rainPresetColors: [
        { hex: "#64c8ff" },
        { hex: "#89dcff" },
        { hex: "#36cfff" },
        { hex: "#7fd8ff" },
        { hex: "#8ee7f2" },
        { hex: "#4f7fff" },
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
    previewInterval() {
      return Math.max(70, 164 - Number(this.config.speed) * 10);
    },
    shouldShowSendingSnapshot() {
      return this.isSendPreviewFrozen;
    },
    previewCanvasBoxStyle() {
      const size = this.previewContainerSize && this.previewContainerSize.height
        ? this.previewContainerSize.height
        : 320;
      return {
        height: `${size}px`,
      };
    },
    isRainPreset() {
      return this.config.preset === "rain_scene";
    },
  },
  watch: {
    config: {
      deep: true,
      handler() {
        this.refreshPreview();
      },
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    this.config = normalizeAmbientConfig(uni.getStorageSync(AMBIENT_CONFIG_KEY));
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initPreviewCanvas();
  },
  onUnload() {
    this.stopPreview();
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
    ensureSendPreviewSnapshot() {
      if (this.isSendPreviewFrozen) {
        return;
      }

      this.captureSendingPreview();
      this.isSendPreviewFrozen = true;
    },
    prepareSendToastUi() {
      this.ensureSendPreviewSnapshot();
    },
    beginSendUi() {
      this.prepareSendToastUi();
      this.isSending = true;
    },
    endSendUi() {
      this.isSending = false;
      if (!this.isToastVisible) {
        this.releaseSendingSnapshot();
      }
    },
    handleBack() {
      uni.navigateBack();
    },
    handleToastShow() {
      this.isToastVisible = true;
    },
    handleToastHide() {
      this.isToastVisible = false;
      if (!this.isSending) {
        this.releaseSendingSnapshot();
      }
    },
    releaseSendingSnapshot() {
      if (!this.isSendPreviewFrozen) {
        return;
      }
      this.isSendPreviewFrozen = false;
      this.clearSendingPreview();
    },
    initPreviewCanvas() {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".preview-canvas-container")
        .boundingClientRect((rect) => {
          if (rect && rect.width) {
            this.previewContainerSize = {
              width: rect.width,
              height: rect.width,
            };
            const fitZoom = Math.max(2, Math.floor((rect.width * 0.96) / 64));
            this.previewZoom = fitZoom;
            this.previewOffset = {
              x: (rect.width - 64 * fitZoom) / 2,
              y: (rect.width - 64 * fitZoom) / 2,
            };
          }
          this.previewCanvasReady = true;
          this.refreshPreview();
        })
        .exec();
    },
    applyPreset(preset) {
      this.config.preset = preset.value;
    },
    handleSpeedChange(event) {
      this.config.speed = Number(event.detail.value);
    },
    handleIntensityChange(event) {
      this.config.intensity = Number(event.detail.value);
    },
    handleDensityChange(event) {
      this.config.density = Number(event.detail.value);
    },
    handleRainColorChange(value) {
      this.config.color = value;
    },
    refreshPreview() {
      if (!this.previewCanvasReady) {
        return;
      }
      this.previewFrameMaps = buildAmbientPreviewFrames(this.config);
      this.previewFrameIndex = 0;
      this.startPreview();
    },
    startPreview() {
      this.stopPreview();
      if (this.previewFrameMaps.length <= 1) {
        return;
      }
      this.previewTimer = setInterval(() => {
        this.previewFrameIndex =
          (this.previewFrameIndex + 1) % this.previewFrameMaps.length;
      }, this.previewInterval);
    },
    stopPreview() {
      if (this.previewTimer) {
        clearInterval(this.previewTimer);
        this.previewTimer = null;
      }
    },
    async saveAndApply() {
      if (this.isSending) {
        return;
      }
      if (!this.deviceStore.connected) {
        this.prepareSendToastUi();
        this.toast.showError("设备未连接");
        return;
      }

      const previousMode = this.deviceStore.deviceMode;
      this.beginSendUi();
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setAmbientEffect(this.config);
        uni.setStorageSync(AMBIENT_CONFIG_KEY, this.config);
        this.toast.showSuccess("已保存并应用");
      } catch (error) {
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode: "led_matrix_showcase",
        });
        console.error("应用场景失败:", error);
        this.toast.showError("发送失败：" + error.message);
      } finally {
        this.endSendUi();
      }
    },
  },
};
</script>

<style scoped>
.ambient-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-secondary);
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
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.scene-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
}

.form-row:last-child {
  margin-bottom: 0;
}

.form-row.inline-row {
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
}

</style>

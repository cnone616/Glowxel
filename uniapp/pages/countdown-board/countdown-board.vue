<template>
  <view class="countdown-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">沙漏倒计时</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container" :style="previewCanvasBoxStyle">
        <PixelCanvas
          v-if="previewCanvasReady && previewCanvasVisible"
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
          canvas-id="countdownPreviewCanvas"
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
            <text>{{ isSending ? "发送中" : "发送" }}</text>
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
            <text class="card-title glx-panel-title">快捷时长</text>
          </view>
          <view class="preset-grid">
            <view
              v-for="preset in quickPresets"
              :key="preset.label"
              class="glx-feature-option glx-feature-option--tile"
              :class="{ active: activePresetLabel === preset.label }"
              @click="applyQuickPreset(preset)"
            >
              <text class="piece-name">{{ preset.label }}</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">倒计时设置</text>
          </view>

          <view class="form-row">
            <text class="form-label">小时</text>
            <GlxStepper
              :value="config.hours"
              :min="0"
              :max="99"
              :step="1"
              @change="handleHoursChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">分钟</text>
            <GlxStepper
              :value="config.minutes"
              :min="0"
              :max="59"
              :step="1"
              @change="handleMinutesChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">秒钟</text>
            <GlxStepper
              :value="config.seconds"
              :min="0"
              :max="59"
              :step="1"
              @change="handleSecondsChange"
            />
          </view>

        </view>
      </view>
    </scroll-view>

    <view v-if="isSending" class="glx-sending-overlay" @touchmove.stop.prevent>
      <view class="glx-sending-modal">
        <view class="glx-sending-spinner"></view>
        <text class="glx-sending-title">正在传输数据...</text>
        <text class="glx-sending-tip">请勿切换网络或关闭程序</text>
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
import PixelCanvas from "../../components/PixelCanvas.vue";
import GlxStepper from "../../components/GlxStepper.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import {
  buildCountdownBoardPreviewFrames,
  createDefaultCountdownBoardConfig,
  normalizeCountdownBoardConfig,
  resolveCountdownBoardPreviewFrameInterval,
} from "../../utils/countdownBoardPreview.js";

const COUNTDOWN_CONFIG_KEY = "countdown_board_config";
const QUICK_PRESETS = [
  { label: "1分钟", hours: 0, minutes: 1, seconds: 0 },
  { label: "5分钟", hours: 0, minutes: 5, seconds: 0 },
  { label: "10分钟", hours: 0, minutes: 10, seconds: 0 },
  { label: "25分钟", hours: 0, minutes: 25, seconds: 0 },
];

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
    GlxStepper,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      isToastVisible: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewCanvasVisible: true,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewFrameIndex: 0,
      previewTimer: null,
      config: createDefaultCountdownBoardConfig(),
      quickPresets: QUICK_PRESETS,
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
      const size = this.previewContainerSize && this.previewContainerSize.height
        ? this.previewContainerSize.height
        : 320;
      return {
        height: `${size}px`,
      };
    },
    activePresetLabel() {
      const matched = this.quickPresets.find(
        (preset) =>
          preset.hours === this.config.hours &&
          preset.minutes === this.config.minutes &&
          preset.seconds === this.config.seconds,
      );
      return matched ? matched.label : "";
    },
  },
  watch: {
    config: {
      deep: true,
      handler() {
        uni.setStorageSync(COUNTDOWN_CONFIG_KEY, this.config);
        this.refreshPreview();
      },
    },
  },
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    const savedConfig = normalizeCountdownBoardConfig(
      uni.getStorageSync(COUNTDOWN_CONFIG_KEY),
    );
    this.config = {
      ...savedConfig,
      progress: 100,
    };
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
    handleBack() {
      uni.navigateBack();
    },
    syncPreviewCanvasVisibility() {
      const nextVisible = !this.isSending && !this.isToastVisible;
      if (this.previewCanvasVisible === nextVisible) {
        return;
      }
      this.previewCanvasVisible = nextVisible;
      if (nextVisible) {
        this.$nextTick(() => {
          this.refreshPreview();
        });
      }
    },
    handleToastShow() {
      this.isToastVisible = true;
      this.syncPreviewCanvasVisibility();
    },
    handleToastHide() {
      this.isToastVisible = false;
      this.syncPreviewCanvasVisibility();
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
    refreshPreview() {
      if (!this.previewCanvasReady) {
        return;
      }
      this.previewFrameMaps = buildCountdownBoardPreviewFrames(this.config);
      this.previewFrameIndex = 0;
      this.startPreview();
    },
    startPreview() {
      this.stopPreview();
      if (this.previewFrameMaps.length <= 1) {
        return;
      }
      const frameInterval = resolveCountdownBoardPreviewFrameInterval(this.config);
      this.previewTimer = setInterval(() => {
        this.previewFrameIndex =
          (this.previewFrameIndex + 1) % this.previewFrameMaps.length;
      }, frameInterval);
    },
    stopPreview() {
      if (this.previewTimer) {
        clearInterval(this.previewTimer);
        this.previewTimer = null;
      }
    },
    applyQuickPreset(preset) {
      this.config.hours = preset.hours;
      this.config.minutes = preset.minutes;
      this.config.seconds = preset.seconds;
      this.config.progress = 100;
    },
    handleHoursChange(event) {
      this.config.hours = Number(event.detail.value);
    },
    handleMinutesChange(event) {
      this.config.minutes = Number(event.detail.value);
    },
    handleSecondsChange(event) {
      this.config.seconds = Number(event.detail.value);
    },
    async saveAndApply() {
      if (this.isSending) {
        this.toast.showInfo("正在传输中，请等待完成");
        return;
      }
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }

      this.isSending = true;
      this.syncPreviewCanvasVisibility();
      try {
        this.config.progress = 100;
        const ws = this.deviceStore.getWebSocket();
        await ws.setCountdownBoard(this.config);
        await this.deviceStore.syncAndRequireMode(
          {
            businessMode: "countdown",
            mode: "animation",
          },
          "设备未进入倒计时模式",
        );
        uni.setStorageSync(COUNTDOWN_CONFIG_KEY, this.config);
        this.toast.showSuccess("已保存并应用");
      } catch (error) {
        console.error("应用沙漏倒计时失败:", error);
        this.toast.showError("发送失败：" + error.message);
      } finally {
        this.isSending = false;
        this.syncPreviewCanvasVisibility();
      }
    },
  },
};
</script>

<style scoped>
.countdown-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-primary);
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

.preset-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12rpx;
}

.card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.form-row:last-child {
  margin-bottom: 0;
}
</style>

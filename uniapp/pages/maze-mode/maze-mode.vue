<template>
  <view class="maze-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">迷宫漫游</text>
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
          canvas-id="mazePreviewCanvas"
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
        <view class="maze-section">
          <view class="maze-section-head">
            <text class="maze-section-title">信息框</text>
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">背景颜色</text>
            <ColorPanelPicker
              :value="config.panelBgColor"
              label="背景颜色"
              :preset-colors="panelPresetColors"
              @change="handleColorChange('panelBgColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">边框颜色</text>
            <ColorPanelPicker
              :value="config.borderColor"
              label="边框颜色"
              :preset-colors="borderPresetColors"
              @change="handleColorChange('borderColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">时间颜色</text>
            <ColorPanelPicker
              :value="config.timeColor"
              label="时间颜色"
              :preset-colors="textPresetColors"
              @change="handleColorChange('timeColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">月份/日期颜色</text>
            <ColorPanelPicker
              :value="config.dateColor"
              label="月份/日期颜色"
              :preset-colors="textPresetColors"
              @change="handleColorChange('dateColor', $event)"
            />
          </view>
        </view>

        <view class="maze-section">
          <view class="maze-section-head">
            <text class="maze-section-title">生成与寻路</text>
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">生成阶段路径颜色</text>
            <ColorPanelPicker
              :value="config.generationPathColor"
              label="生成阶段路径颜色"
              :preset-colors="stagePresetColors"
              @change="handleColorChange('generationPathColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">寻路已搜索颜色</text>
            <ColorPanelPicker
              :value="config.searchVisitedColor"
              label="寻路已搜索颜色"
              :preset-colors="stagePresetColors"
              @change="handleColorChange('searchVisitedColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">寻路待搜索颜色</text>
            <ColorPanelPicker
              :value="config.searchFrontierColor"
              label="寻路待搜索颜色"
              :preset-colors="stagePresetColors"
              @change="handleColorChange('searchFrontierColor', $event)"
            />
          </view>
        </view>

        <view class="maze-section">
          <view class="maze-section-head">
            <text class="maze-section-title">完成路径</text>
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">最终路径起始色</text>
            <ColorPanelPicker
              :value="config.solvedPathStartColor"
              label="最终路径起始色"
              :preset-colors="stagePresetColors"
              @change="handleColorChange('solvedPathStartColor', $event)"
            />
          </view>
          <view class="form-row color-picker-row">
            <text class="form-label">最终路径结束色</text>
            <ColorPanelPicker
              :value="config.solvedPathEndColor"
              label="最终路径结束色"
              :preset-colors="stagePresetColors"
              @change="handleColorChange('solvedPathEndColor', $event)"
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
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { buildLedMatrixPreviewSequence } from "../../utils/ledMatrixShowcase.js";
import {
  cloneMazeModeConfig,
  createDefaultMazeModeConfig,
  createMazeModeConfig,
  readSavedMazeModeConfig,
  writeSavedMazeModeConfig,
} from "../../utils/mazeModeConfig.js";

export default {
  mixins: [statusBarMixin, deviceSendUxMixin],
  components: {
    Icon,
    Toast,
    GlxInlineLoader,
    PixelCanvas,
    PixelPreviewBoard,
    ColorPanelPicker,
  },
  data() {
    const defaultConfig = createDefaultMazeModeConfig();
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
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
      config: cloneMazeModeConfig(defaultConfig),
      panelPresetColors: [
        { hex: "#05070f" },
        { hex: "#0c1220" },
        { hex: "#101826" },
        { hex: "#16161b" },
        { hex: "#1c2430" },
        { hex: "#2a1d12" },
      ],
      borderPresetColors: [
        { hex: "#182c4c" },
        { hex: "#1d3b66" },
        { hex: "#20528a" },
        { hex: "#2c4f7c" },
        { hex: "#44618e" },
        { hex: "#7c3aed" },
      ],
      textPresetColors: [
        { hex: "#ffd400" },
        { hex: "#ffe066" },
        { hex: "#ff6464" },
        { hex: "#ff8fab" },
        { hex: "#7dd3fc" },
        { hex: "#ffffff" },
      ],
      stagePresetColors: [
        { hex: "#4f4f55" },
        { hex: "#70ff9c" },
        { hex: "#ff4444" },
        { hex: "#1a60ff" },
        { hex: "#42bcff" },
        { hex: "#ffd166" },
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
    config: {
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
      const nextConfig = createMazeModeConfig(config);
      if (!nextConfig) {
        return;
      }
      this.config = cloneMazeModeConfig(nextConfig);
    },
    buildCurrentConfig() {
      return createMazeModeConfig(this.config);
    },
    handleBack() {
      uni.navigateBack();
    },
    handleColorChange(field, value) {
      this.config = {
        ...this.config,
        [field]: value,
      };
    },
    loadSavedConfig() {
      const savedConfig = readSavedMazeModeConfig();
      if (savedConfig) {
        this.applyConfig(savedConfig);
        return;
      }
      this.applyConfig(createDefaultMazeModeConfig());
    },
    saveConfig() {
      const currentConfig = this.buildCurrentConfig();
      if (!currentConfig) {
        throw new Error("迷宫颜色配置无效");
      }
      this.config = writeSavedMazeModeConfig(currentConfig);
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
                this.schedulePreviewRefresh();
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
              this.schedulePreviewRefresh();
            })
            .exec();
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
      const currentConfig = this.buildCurrentConfig();
      if (!currentConfig) {
        this.resetPreviewFrames();
        return;
      }
      const previewSequence = buildLedMatrixPreviewSequence({
        demoId: "maze",
        speed: currentConfig.speed,
        mazeSizeMode: currentConfig.mazeSizeMode,
        showClock: currentConfig.showClock,
        panelBgColor: currentConfig.panelBgColor,
        borderColor: currentConfig.borderColor,
        timeColor: currentConfig.timeColor,
        dateColor: currentConfig.dateColor,
        generationPathColor: currentConfig.generationPathColor,
        searchVisitedColor: currentConfig.searchVisitedColor,
        searchFrontierColor: currentConfig.searchFrontierColor,
        solvedPathStartColor: currentConfig.solvedPathStartColor,
        solvedPathEndColor: currentConfig.solvedPathEndColor,
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
    async saveAndApply() {
      if (!this.guardBeforeSend(this.deviceStore.connected)) {
        return;
      }

      const previousMode = this.deviceStore.deviceMode;
      this.beginSendUi();
      try {
        const currentConfig = this.buildCurrentConfig();
        if (!currentConfig) {
          throw new Error("迷宫颜色配置无效");
        }
        const ws = this.deviceStore.getWebSocket();
        await ws.startMaze(currentConfig);
        this.saveConfig();
        this.showSendSuccess();
      } catch (error) {
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode: "maze",
        });
        console.error("发送迷宫漫游失败:", error);
        this.showSendFailure(error);
      } finally {
        this.endSendUi();
      }
    },
  },
};
</script>

<style scoped>
.maze-page {
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
  display: flex;
  flex-direction: column;
  gap: 26rpx;
  padding-bottom: calc(44rpx + env(safe-area-inset-bottom));
}

.maze-section {
  display: flex;
  flex-direction: column;
}

.maze-section-head {
  display: flex;
  align-items: center;
  margin-bottom: 6rpx;
}

.maze-section-title {
  color: var(--nb-ink);
  font-size: 24rpx;
  line-height: 1.15;
  font-weight: 800;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.color-picker-row {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}
</style>

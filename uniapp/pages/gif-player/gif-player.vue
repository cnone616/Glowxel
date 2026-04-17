<template>
  <view class="gif-player-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">GIF/场景播放器</text>
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
          canvas-id="gifPlayerPreviewCanvas"
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
            <text class="card-title glx-panel-title">离线素材</text>
          </view>
          <view class="scene-grid">
            <view
              v-for="item in sceneItems"
              :key="item.id"
              class="glx-feature-option glx-feature-option--scene"
              :class="{ active: selectedSceneId === item.id }"
              @click="handleSceneSelect(item.id)"
            >
              <text class="glx-feature-option__label">{{ item.label }}</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">参数</text>
          </view>
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
            <text class="form-label">强度 {{ intensity }}</text>
            <GlxStepper
              :value="intensity"
              :min="10"
              :max="100"
              :step="1"
              @change="handleIntensityChange"
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
  buildGifPlayerAnimationPayload,
  buildGifPlayerPreviewSequence,
  getGifPlayerSceneItems,
  resolveSceneById,
} from "../../utils/gif-player/localPlayer.js";

const GIF_PLAYER_CONFIG_KEY = "gif_player_config";

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
      previewFrameDelays: [],
      previewFrameIndex: 0,
      previewTimer: null,
      previewRefreshTimer: null,
      sceneItems: getGifPlayerSceneItems(),
      selectedSceneId: "matrix_fire",
      speed: 6,
      intensity: 72,
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
  },
  watch: {
    selectedSceneId() {
      this.resetPreviewFrames();
      this.schedulePreviewRefresh();
    },
    speed() {
      this.schedulePreviewRefresh();
    },
    intensity() {
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
          this.schedulePreviewRefresh();
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
      const previewSequence = buildGifPlayerPreviewSequence({
        sceneId: this.selectedSceneId,
        speed: this.speed,
        intensity: this.intensity,
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
    handleSceneSelect(sceneId) {
      this.selectedSceneId = sceneId;
    },
    handleSpeedChange(event) {
      this.speed = Number(event.detail.value);
    },
    handleIntensityChange(event) {
      this.intensity = Number(event.detail.value);
    },
    loadSavedConfig() {
      const saved = uni.getStorageSync(GIF_PLAYER_CONFIG_KEY);
      if (!saved || typeof saved !== "object") {
        return;
      }
      if (typeof saved.sceneId === "string" && resolveSceneById(saved.sceneId)) {
        this.selectedSceneId = saved.sceneId;
      }
      if (Number.isFinite(Number(saved.speed))) {
        this.speed = Number(saved.speed);
      }
      if (Number.isFinite(Number(saved.intensity))) {
        this.intensity = Number(saved.intensity);
      }
    },
    saveConfig() {
      uni.setStorageSync(GIF_PLAYER_CONFIG_KEY, {
        sceneId: this.selectedSceneId,
        speed: this.speed,
        intensity: this.intensity,
      });
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
        const ws = this.deviceStore.getWebSocket();
        const payload = buildGifPlayerAnimationPayload({
          sceneId: this.selectedSceneId,
          speed: this.speed,
          intensity: this.intensity,
        });
        await ws.setGifAnimation(payload.animationData);
        await ws.setMode("gif_player");
        const status = await ws.getStatus();
        if (
          status.businessMode !== "gif_player" ||
          status.mode !== "animation" ||
          !Number.isFinite(Number(status.animationFrames)) ||
          Number(status.animationFrames) <= 0
        ) {
          throw new Error("设备未成功加载 GIF 动画");
        }
        this.deviceStore.setDeviceMode("gif_player", { businessMode: true });
        this.saveConfig();
        this.toast.showSuccess("离线素材已发送到设备");
      } catch (error) {
        console.error("发送 GIF 素材失败:", error);
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
.gif-player-page {
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

.scene-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10rpx;
}

.card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

</style>

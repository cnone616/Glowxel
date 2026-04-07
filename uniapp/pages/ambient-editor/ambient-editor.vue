<template>
  <view class="ambient-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="header">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <view class="nav-title">
        <text class="project-name">像素屏保</text>
      </view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container">
        <PixelCanvas
          v-if="previewCanvasReady"
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
      </view>
      <view class="preview-caption">
        <view class="preview-caption-info">
          <view class="preview-status-chip" :class="previewStatusClass">
            <text class="preview-status-chip-text">{{ previewStatusLabel }}</text>
          </view>
          <text class="preview-title">64 x 64 场景预览</text>
          <text class="preview-subtitle">{{ previewSubtitle }}</text>
        </view>
        <view class="preview-actions">
          <view class="action-btn-sm" :class="{ disabled: isSending }" @click="saveDraft">
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
            <text>保存</text>
          </view>
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="saveAndApply"
          >
            <Icon name="link" :size="36" color="#ffffff" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">屏保选择</text>
            <text class="card-subtitle">按成熟像素屏保项目分类选择</text>
          </view>
          <view class="card-tip-banner">
            <text class="card-tip-banner-text"
              >当前页面只保留成熟屏保方向，不再维护旧的泛化氛围效果。</text
            >
          </view>
          <view class="piece-grid">
            <view
              v-for="preset in presets"
              :key="preset.value"
              class="piece-item"
              :class="{ active: config.preset === preset.value }"
              @click="applyPreset(preset)"
            >
              <view class="piece-top">
                <text class="piece-tag">{{ preset.tag }}</text>
              </view>
              <text class="piece-name">{{ preset.label }}</text>
              <text class="piece-hint">{{ preset.hint }}</text>
            </view>
          </view>
        </view>

        <view class="card">
          <view class="card-title-section">
            <text class="card-title">场景参数</text>
          </view>

          <view class="form-row">
            <text class="form-label">速度 {{ config.speed }}</text>
            <slider
              :value="config.speed"
              min="1"
              max="10"
              step="1"
              activeColor="#4F7FFF"
              @change="handleSpeedChange"
            />
          </view>

          <view class="form-row">
            <text class="form-label">强度 {{ config.intensity }}</text>
            <slider
              :value="config.intensity"
              min="10"
              max="100"
              step="1"
              activeColor="#4F7FFF"
              @change="handleIntensityChange"
            />
          </view>

          <view class="form-row inline-row">
            <text class="form-label">循环</text>
            <view class="toggle-switch" @click="config.loop = !config.loop">
              <view class="switch-track" :class="{ active: config.loop }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import { buildAmbientPreviewFrames } from "../../utils/ambientEffectPreview.js";

const AMBIENT_CONFIG_KEY = "ambient_effect_config";

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    PixelCanvas,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewFrameIndex: 0,
      previewTimer: null,
      presets: [
        { value: "aurora_flow", label: "极光流幕", tag: "led-matrix", hint: "偏 led-matrix 一类的柔和流场，适合长时间桌面待机" },
        { value: "plasma_wave", label: "等离子波", tag: "SmartMatrix", hint: "更接近 SmartMatrix 常见的能量流场和高彩动态" },
        { value: "matrix_rain", label: "矩阵数字雨", tag: "led-matrix", hint: "直接对标矩阵雨场景，列流和字符闪烁更明显" },
        { value: "firefly_swarm", label: "萤火屏保", tag: "pixel-art", hint: "偏像素艺术氛围，适合做有生命感的桌面屏保" },
        { value: "meteor_shower", label: "流星穿屏", tag: "led-matrix", hint: "更接近场景动画里的斜向拖尾和星空冲刷感" },
        { value: "ocean_current", label: "海流波纹", tag: "gif-scene", hint: "偏 GIF 场景底座思路，适合作为柔和背景" },
        { value: "neon_grid", label: "霓虹网格", tag: "WLED", hint: "对标 WLED 一类的线网扫描和赛博流光" },
        { value: "sunset_blush", label: "日落霞光", tag: "pixel-art", hint: "偏像素风天空场景，适合作为静态桌搭背景" },
        { value: "starfield_drift", label: "星野漂移", tag: "led-matrix", hint: "暗底星点漂移，更接近成熟信息屏的克制背景层" },
      ],
      config: {
        preset: "aurora_flow",
        speed: 6,
        intensity: 72,
        loop: true,
      },
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
    previewSubtitle() {
      const current = this.presets.find((item) => item.value === this.config.preset);
      if (!current) {
        return "预览当前场景在 64x64 上的动态效果";
      }
      return `${current.tag} · ${current.hint} · 速度 ${this.config.speed} · 强度 ${this.config.intensity}`;
    },
    previewStatusLabel() {
      if (this.isSending) {
        return "发送中";
      }
      if (!this.previewCanvasReady) {
        return "预览加载中";
      }
      return "场景预览";
    },
    previewStatusClass() {
      if (this.isSending) {
        return "is-sending";
      }
      if (!this.previewCanvasReady) {
        return "is-loading";
      }
      return "is-preview";
    },
    previewInterval() {
      return Math.max(70, 164 - Number(this.config.speed) * 10);
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
    const saved = uni.getStorageSync(AMBIENT_CONFIG_KEY);
    if (saved && typeof saved === "object") {
      this.config = {
        ...this.config,
        ...saved,
      };
    }
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
    initPreviewCanvas() {
      const query = uni.createSelectorQuery().in(this);
      query
        .select(".preview-canvas-container")
        .boundingClientRect((rect) => {
          if (rect) {
            this.previewContainerSize = {
              width: rect.width,
              height: rect.height,
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
    saveDraft() {
      uni.setStorageSync(AMBIENT_CONFIG_KEY, this.config);
      this.toast.showSuccess("草稿已保存");
    },
    async saveAndApply() {
      if (this.isSending) {
        return;
      }
      if (!this.deviceStore.connected) {
        this.toast.showError("设备未连接");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.setAmbientEffect(this.config);
        uni.setStorageSync(AMBIENT_CONFIG_KEY, this.config);
        this.deviceStore.setDeviceMode("ambient_effect", { businessMode: true });
        this.toast.showSuccess("已保存并应用");
      } catch (error) {
        console.error("应用场景失败:", error);
        this.toast.showError("发送失败");
      } finally {
        this.isSending = false;
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

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-title {
  display: flex;
  align-items: center;
  justify-content: center;
}

.project-name {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.canvas-section {
  padding: 24rpx 24rpx 16rpx;
  background: linear-gradient(180deg, rgba(17, 24, 39, 0.98), rgba(10, 14, 24, 0.98));
  border-bottom: 2rpx solid rgba(148, 163, 184, 0.14);
}

.preview-canvas-container {
  width: 100%;
  height: 420rpx;
  border-radius: 28rpx;
  overflow: hidden;
  background: radial-gradient(circle at 50% 32%, rgba(59, 130, 246, 0.18), rgba(15, 23, 42, 0.98));
  border: 2rpx solid rgba(148, 163, 184, 0.2);
}

.preview-caption {
  margin-top: 20rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 20rpx;
}

.preview-caption-info {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.preview-status-chip {
  display: inline-flex;
  align-self: flex-start;
  padding: 8rpx 16rpx;
  border-radius: 999rpx;
  background: rgba(59, 130, 246, 0.16);
}

.preview-status-chip.is-sending {
  background: rgba(245, 158, 11, 0.18);
}

.preview-status-chip.is-loading {
  background: rgba(148, 163, 184, 0.16);
}

.preview-status-chip-text {
  font-size: 20rpx;
  color: #dbeafe;
}

.preview-title {
  font-size: 28rpx;
  font-weight: 600;
  color: #f8fafc;
}

.preview-subtitle {
  font-size: 22rpx;
  color: #94a3b8;
}

.preview-actions {
  display: flex;
  gap: 16rpx;
}

.action-btn-sm {
  min-width: 132rpx;
  height: 80rpx;
  padding: 0 24rpx;
  border-radius: 20rpx;
  background: rgba(255, 255, 255, 0.08);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
}

.action-btn-sm.primary {
  background: linear-gradient(135deg, #2563eb, #3b82f6);
}

.action-btn-sm.disabled {
  opacity: 0.45;
}

.action-btn-sm text {
  font-size: 24rpx;
  color: var(--color-text-primary);
}

.action-btn-sm.primary text {
  color: #ffffff;
}

.content {
  flex: 1;
}

.content-wrapper {
  padding: 24rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.card {
  background: var(--bg-elevated);
  border: 2rpx solid var(--border-primary);
  border-radius: 28rpx;
  padding: 24rpx;
}

.card-title-section {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  margin-bottom: 20rpx;
}

.card-title {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.card-subtitle {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.card-tip-banner {
  margin-bottom: 18rpx;
  padding: 16rpx 20rpx;
  border-radius: 18rpx;
  background: rgba(79, 127, 255, 0.08);
  border: 2rpx solid rgba(79, 127, 255, 0.14);
}

.card-tip-banner-text {
  font-size: 22rpx;
  line-height: 1.6;
  color: var(--color-text-secondary);
}

.piece-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16rpx;
}

.piece-item {
  border-radius: 22rpx;
  padding: 18rpx;
  background: rgba(15, 23, 42, 0.72);
  border: 2rpx solid rgba(148, 163, 184, 0.16);
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.piece-item.active {
  border-color: rgba(59, 130, 246, 0.9);
  box-shadow: 0 0 0 2rpx rgba(59, 130, 246, 0.18);
}

.piece-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.piece-tag {
  font-size: 20rpx;
  color: #93c5fd;
}

.piece-name {
  font-size: 26rpx;
  font-weight: 600;
  color: #f8fafc;
}

.piece-hint {
  font-size: 22rpx;
  color: #94a3b8;
  line-height: 1.5;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
  margin-bottom: 20rpx;
}

.form-row:last-child {
  margin-bottom: 0;
}

.form-row.inline-row {
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
}

.form-label {
  font-size: 24rpx;
  color: var(--color-text-primary);
}

.toggle-switch {
  display: inline-flex;
  align-items: center;
}

.switch-track {
  width: 92rpx;
  height: 52rpx;
  border-radius: 999rpx;
  background: rgba(148, 163, 184, 0.28);
  padding: 4rpx;
  transition: background 0.2s ease;
}

.switch-track.active {
  background: rgba(59, 130, 246, 0.9);
}

.switch-thumb {
  width: 44rpx;
  height: 44rpx;
  border-radius: 50%;
  background: #ffffff;
  transition: transform 0.2s ease;
}

.switch-track.active .switch-thumb {
  transform: translateX(40rpx);
}
</style>

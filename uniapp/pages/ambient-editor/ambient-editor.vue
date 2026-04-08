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
          <text class="preview-title">预览效果</text>
        </view>
        <view class="preview-actions">
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
        <view v-show="currentTab === 0" class="tab-panel">
          <view class="card">
            <view class="card-title-section">
              <text class="card-title">屏保选择</text>
            </view>
            <view class="scene-grid">
              <view
                v-for="preset in presets"
                :key="preset.value"
                class="scene-option"
                :class="{ active: config.preset === preset.value }"
                @click="applyPreset(preset)"
              >
                <text class="scene-option-name">{{ preset.label }}</text>
              </view>
            </view>
          </view>

          <view class="card">
            <view class="card-title-section">
              <text class="card-title">当前场景</text>
              <text class="card-subtitle">{{ currentPresetTag }}</text>
            </view>
            <text class="card-tip-text">{{ currentPresetHint }}</text>
          </view>
        </view>

        <view v-show="currentTab === 1" class="tab-panel">
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
      </view>
    </scroll-view>

    <view class="bottom-tabs">
      <view
        v-for="(tab, index) in tabs"
        :key="tab"
        class="bottom-tab-item"
        :class="{ active: currentTab === index }"
        @click="currentTab = index"
      >
        <Icon
          :name="tabIconNames[index]"
          :size="36"
          :color="currentTab === index ? '#4F7FFF' : 'var(--text-secondary)'"
        />
        <text class="bottom-tab-text">{{ tab }}</text>
      </view>
    </view>

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
      contentHeight: "calc(100vh - 88rpx - 520rpx - 112rpx)",
      previewCanvasReady: false,
      previewZoom: 4,
      previewOffset: { x: 0, y: 0 },
      previewContainerSize: { width: 320, height: 320 },
      previewFrameMaps: [],
      previewFrameIndex: 0,
      previewTimer: null,
      currentTab: 0,
      tabs: ["场景", "参数"],
      tabIconNames: ["browse", "setting"],
      presets: [
        { value: "digital_rain", label: "数字雨", tag: "字符", hint: "科技感最强，适合常亮待机" },
        { value: "starfield", label: "星空漂移", tag: "夜间", hint: "暗底星点缓慢闪烁，适合夜里展示" },
        { value: "neon_tunnel", label: "霓虹隧道", tag: "纵深", hint: "中间向里收，画面更有空间感" },
        { value: "metablob", label: "流体团块", tag: "流体", hint: "像液体一样聚合分离，层次更自然" },
        { value: "falling_sand", label: "流沙", tag: "颗粒", hint: "细颗粒往下落，适合做静置屏保" },
        { value: "sparks", label: "火花", tag: "暖色", hint: "从底部往外冒，动态更有生命感" },
        { value: "wave_pattern", label: "波纹图案", tag: "规律", hint: "图案化流动，适合简洁桌搭风格" },
        { value: "rain_scene", label: "雨幕", tag: "天气", hint: "偏真实雨幕感，整体更克制" },
        { value: "watermelon_plasma", label: "西瓜等离子", tag: "高彩", hint: "红绿交错的高彩流动，画面更活" },
        { value: "boids", label: "群游粒子", tag: "集群", hint: "像小鱼群一样绕场流动，更有生命感" },
        { value: "bouncing_logo", label: "弹跳徽标", tag: "品牌", hint: "九宫格徽标在屏里弹跳，更适合展示台面" },
        { value: "sorting_visualizer", label: "排序柱阵", tag: "算法", hint: "柱条不断交换重排，节奏感更强" },
        { value: "clock_scene", label: "场景时钟", tag: "时钟", hint: "用 led-matrix 场景时钟的方式展示当前时间" },
        { value: "countdown_scene", label: "倒计时", tag: "读数", hint: "大字倒计时配进度条，适合展示状态" },
        { value: "weather_scene", label: "天气场景", tag: "信息", hint: "天气图标和温度同屏，更像内容场景" },
        { value: "game_of_life", label: "生命游戏", tag: "细胞", hint: "经典细胞自动机，适合做算法屏保" },
        { value: "julia_set", label: "朱莉亚集", tag: "分形", hint: "分形纹理持续变形，细节更多" },
        { value: "reaction_diffusion", label: "反应扩散", tag: "生长", hint: "像细胞纹路一样蔓延，质感更强" },
      ],
      config: {
        preset: "digital_rain",
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
    currentPreset() {
      const current = this.presets.find((item) => item.value === this.config.preset);
      if (!current) {
        return this.presets[0];
      }
      return current;
    },
    currentPresetTag() {
      return this.currentPreset ? this.currentPreset.tag : "";
    },
    currentPresetHint() {
      return this.currentPreset ? this.currentPreset.hint : "";
    },
    previewStatusLabel() {
      if (this.isSending) {
        return "发送中";
      }
      if (!this.previewCanvasReady) {
        return "加载中";
      }
      return "就绪";
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

.tab-panel {
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

.card-tip-text {
  font-size: 24rpx;
  line-height: 1.6;
  color: var(--text-secondary);
}

.scene-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.scene-option {
  width: calc(33.333% - 11rpx);
  min-height: 80rpx;
  padding: 0 12rpx;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.scene-option.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.scene-option-name {
  font-size: 24rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 1.4;
}

.scene-option.active .scene-option-name {
  color: var(--accent-primary);
  font-weight: 600;
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

.bottom-tabs {
  display: flex;
  flex-shrink: 0;
  padding: 12rpx 16rpx;
  padding-bottom: calc(12rpx + env(safe-area-inset-bottom));
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  gap: 8rpx;
}

.bottom-tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4rpx;
}

.bottom-tab-item.active {
  background-color: var(--bg-tertiary);
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: var(--accent-primary);
  font-weight: 500;
}
</style>

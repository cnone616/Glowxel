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
      <text class="nav-title glx-topbar__title">像素屏保</text>
      <view class="nav-right"></view>
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
            <Icon name="link" :size="36" color="#000000" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" :style="{ height: contentHeight }">
      <view class="content-wrapper glx-scroll-stack">
        <view v-show="currentTab === 0" class="tab-panel glx-tab-panel">
          <view class="card glx-panel-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">屏保选择</text>
            </view>
            <view class="scene-grid">
              <view
                v-for="preset in presets"
                :key="preset.value"
                class="scene-option glx-feature-option"
                :class="{ active: config.preset === preset.value }"
                @click="applyPreset(preset)"
              >
                <text class="scene-option-name">{{ preset.label }}</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">当前场景</text>
              <text class="card-subtitle glx-panel-subtitle">{{ currentPresetTag }}</text>
            </view>
            <text class="card-tip-text">{{ currentPresetHint }}</text>
          </view>
        </view>

        <view v-show="currentTab === 1" class="tab-panel glx-tab-panel">
          <view class="card glx-panel-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">场景参数</text>
            </view>

            <view class="form-row">
              <text class="form-label">速度 {{ config.speed }}</text>
              <GlxSlider
                :value="config.speed"
                :min="1"
                :max="10"
                :step="1"
                @change="handleSpeedChange"
              />
            </view>

            <view class="form-row">
              <text class="form-label">强度 {{ config.intensity }}</text>
              <GlxSlider
                :value="config.intensity"
                :min="10"
                :max="100"
                :step="1"
                @change="handleIntensityChange"
              />
            </view>

            <view class="form-row inline-row">
              <text class="form-label">循环</text>
              <view class="toggle-switch" @click="config.loop = !config.loop">
                <view class="glx-switch-shell" :class="{ active: config.loop }">
                  <view class="glx-switch-thumb"></view>
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
          :color="currentTab === index ? '#000000' : '#6b7280'"
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
import GlxSlider from "../../components/GlxSlider.vue";
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
    GlxSlider,
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

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000000;
  border-bottom: 2rpx solid var(--nb-ink);
}

.preview-canvas-container {
  width: 100%;
  aspect-ratio: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  background: #000000;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12rpx;
  padding: 10rpx 16rpx 12rpx;
  background: var(--bg-tertiary);
}

.preview-caption-info {
  display: flex;
  flex-direction: column;
  gap: 0;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.action-btn-sm {
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  border-radius: 0;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--nb-ink);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
}

.action-btn-sm.primary {
  background: var(--nb-yellow);
}

.action-btn-sm.disabled {
  opacity: 0.45;
}

.action-btn-sm text {
  font-size: 24rpx;
  color: var(--nb-ink);
  font-weight: 600;
}

.action-btn-sm.primary text {
  color: #000000;
}

.content {
  flex: 1;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.content-wrapper {
  padding: 0 0 56rpx;
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.tab-panel {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.card {
  background: transparent;
  border: 0 !important;
  border-radius: 0;
  padding-top: 16rpx;
  box-shadow: none !important;
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8rpx;
  margin-bottom: 14rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.card-subtitle {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.card-tip-text {
  font-size: 22rpx;
  line-height: 1.65;
  color: var(--text-secondary);
}

.scene-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14rpx;
}

.scene-option {
  width: auto;
  min-height: 92rpx;
  padding: 0 10rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.scene-option.active {
  background: var(--nb-yellow);
  border-color: var(--nb-ink);
  box-shadow: none;
}

.scene-option-name {
  font-size: 22rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 1.3;
}

.scene-option.active .scene-option-name {
  color: #000000;
  font-weight: 700;
  font-size: 23rpx;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  margin-bottom: 16rpx;
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
  color: var(--nb-ink);
}

.toggle-switch {
  display: inline-flex;
  align-items: center;
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
}

.bottom-tab-item.active {
  background-color: transparent;
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: #000000;
  font-weight: 700;
  font-size: 22rpx;
}

.bottom-tab-item.active :deep(.iconfont) {
  color: #000000 !important;
  font-size: 40rpx !important;
}
</style>

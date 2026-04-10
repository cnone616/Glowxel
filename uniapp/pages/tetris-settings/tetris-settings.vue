<template>
  <view class="tetris-page glx-page-shell">
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
      <text class="nav-title glx-topbar__title">俄罗斯方块时钟</text>
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
          canvas-id="tetrisPreviewCanvas"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">预览效果</text>
        </view>
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

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" :style="{ height: contentHeight }">
      <view class="content-wrapper glx-scroll-stack">
        <view class="tab-panel glx-tab-panel">
          <view class="card glx-panel-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">效果风格</text>
              <text class="card-subtitle glx-panel-subtitle">{{ currentSceneLabel }}</text>
            </view>
            <view class="scene-grid">
              <view
                v-for="preset in scenePresets"
                :key="preset.key"
                class="scene-card glx-scene-tile"
                :class="{ active: config.sceneKey === preset.key }"
                @click="applyScenePreset(preset)"
              >
                <text class="scene-card-title">{{ preset.label }}</text>
                <text class="scene-card-desc">{{ preset.desc }}</text>
              </view>
            </view>
          </view>

          <view class="card glx-panel-card">
            <view class="card-title-section glx-panel-head">
              <text class="card-title glx-panel-title">风格说明</text>
            </view>
            <text class="summary-text">{{ currentSceneDesc }}</text>
          </view>
        </view>
      </view>
    </scroll-view>

    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import { buildTetrisPreviewFrames } from "../../utils/tetrisClockPreview.js";

const DEFAULT_PIECES = [0, 1, 2, 3, 4, 5, 6];

const SCENE_PRESETS = [
  {
    key: "clock_classic",
    label: "标准拼时",
    desc: "完整轮廓慢慢拼合，适合做主展示。",
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
  {
    key: "clock_compact",
    label: "快速拼时",
    desc: "格子更细，拼得更快，数字轮廓更完整。",
    clearMode: true,
    cellSize: 1,
    speed: "fast",
    showClock: true,
    pieces: [0, 1, 2, 5, 6],
  },
  {
    key: "clock_bold",
    label: "大块拼时",
    desc: "块更大，远看更醒目，适合桌面远距显示。",
    clearMode: true,
    cellSize: 3,
    speed: "slow",
    showClock: true,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  },
  {
    key: "clock_clean",
    label: "规整拼时",
    desc: "只保留更利落的块型，画面更干净。",
    clearMode: true,
    cellSize: 2,
    speed: "slow",
    showClock: true,
    pieces: [0, 1, 5, 6],
  },
];

function clonePieces(source) {
  return source.slice();
}

function createDefaultConfig() {
  return {
    sceneKey: "clock_classic",
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: clonePieces(DEFAULT_PIECES),
  };
}

function samePieces(left, right) {
  if (!Array.isArray(left) || !Array.isArray(right)) {
    return false;
  }
  if (left.length !== right.length) {
    return false;
  }
  for (let index = 0; index < left.length; index += 1) {
    if (left[index] !== right[index]) {
      return false;
    }
  }
  return true;
}

function findSceneKey(config) {
  for (let index = 0; index < SCENE_PRESETS.length; index += 1) {
    const preset = SCENE_PRESETS[index];
    if (
      preset.clearMode === config.clearMode &&
      preset.cellSize === config.cellSize &&
      preset.speed === config.speed &&
      preset.showClock === config.showClock &&
      samePieces(preset.pieces, config.pieces)
    ) {
      return preset.key;
    }
  }
  return "";
}

function normalizeSavedConfig(saved) {
  const config = createDefaultConfig();
  if (!saved || typeof saved !== "object") {
    return config;
  }

  if (typeof saved.clearMode === "boolean") {
    config.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    config.cellSize = saved.cellSize;
  }
  if (
    saved.speed === "slow" ||
    saved.speed === "normal" ||
    saved.speed === "fast"
  ) {
    config.speed = saved.speed;
  }
  if (typeof saved.showClock === "boolean") {
    config.showClock = saved.showClock;
  }
  if (Array.isArray(saved.pieces) && saved.pieces.length > 0) {
    const nextPieces = saved.pieces
      .filter((item) => Number.isInteger(item) && item >= 0 && item <= 6)
      .sort((left, right) => left - right);
    if (nextPieces.length > 0) {
      config.pieces = nextPieces;
    }
  }

  config.sceneKey = findSceneKey(config);
  return config;
}

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
      scenePresets: SCENE_PRESETS,
      config: createDefaultConfig(),
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
      if (this.config.speed === "slow") {
        return 300;
      }
      if (this.config.speed === "fast") {
        return 80;
      }
      return 150;
    },
    currentScene() {
      const current = this.scenePresets.find((item) => item.key === this.config.sceneKey);
      if (current) {
        return current;
      }
      return null;
    },
    currentSceneLabel() {
      if (this.currentScene) {
        return this.currentScene.label;
      }
      return "自定义";
    },
    currentSceneDesc() {
      if (this.currentScene && this.currentScene.desc) {
        return this.currentScene.desc;
      }
      return "当前风格将直接按预设展示，不再额外叠加旧版自定义玩法。";
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
    const saved = uni.getStorageSync("tetris_config");
    this.config = normalizeSavedConfig(saved);
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
      const systemInfo = uni.getSystemInfoSync();
      const statusBarHeight = systemInfo.statusBarHeight || 0;
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
            const nextHeight =
              systemInfo.windowHeight - statusBarHeight - 88 - rect.width;
            this.contentHeight = `${Math.max(120, nextHeight)}px`;
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
      this.previewFrameMaps = buildTetrisPreviewFrames(this.config);
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
    applyScenePreset(preset) {
      this.config.sceneKey = preset.key;
      this.config.clearMode = preset.clearMode;
      this.config.cellSize = preset.cellSize;
      this.config.speed = preset.speed;
      this.config.showClock = preset.showClock;
      this.config.pieces = clonePieces(preset.pieces);
    },
    async saveAndApply() {
      if (this.isSending) {
        return;
      }

      uni.setStorageSync("tetris_config", this.config);

      if (!this.deviceStore.connected) {
        this.toast.showInfo("请先连接设备");
        return;
      }

      this.isSending = true;
      try {
        const ws = this.deviceStore.getWebSocket();
        const speedMap = {
          slow: 300,
          normal: 150,
          fast: 80,
        };
        const speed = speedMap[this.config.speed];
        if (speed === undefined) {
          this.toast.showError("速度参数无效");
          return;
        }

        await ws.send({
          cmd: "set_mode",
          mode: "tetris",
          clearMode: this.config.clearMode,
          cellSize: this.config.cellSize,
          speed,
          showClock: this.config.showClock,
          pieces: this.config.pieces,
        });

        this.toast.showSuccess("俄罗斯方块时钟已发送到设备");
        const pages = getCurrentPages();
        if (pages.length >= 2) {
          const prev = pages[pages.length - 2];
          if (prev && prev.deviceMode !== undefined) {
            prev.deviceMode = "tetris";
          }
        }
        this.deviceStore.setDeviceMode("tetris", { businessMode: true });
      } catch (error) {
        console.error("发送俄罗斯方块时钟失败:", error);
        this.toast.showError("发送失败");
      } finally {
        this.isSending = false;
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

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000000;
  border-bottom: 2rpx solid var(--nb-ink);
  flex-shrink: 0;
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
  flex: 1;
  min-width: 0;
}

.preview-title {
  font-size: 24rpx;
  font-weight: 700;
  color: var(--text-primary);
}

.action-btn-sm {
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  background-color: var(--bg-tertiary);
}

.action-btn-sm.primary {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.action-btn-sm.disabled {
  opacity: 0.6;
}

.action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: #000000;
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
  padding-top: 16rpx;
  border: 0 !important;
  box-shadow: none !important;
  background: transparent !important;
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

.scene-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.scene-card {
  width: calc(50% - 8rpx);
  min-height: 152rpx;
  padding: 16rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 14rpx;
  box-sizing: border-box;
}

.scene-card.active {
  background: var(--nb-yellow);
  border-color: var(--nb-ink);
  box-shadow: none;
}

.scene-card-title {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.scene-card-desc {
  font-size: 21rpx;
  line-height: 1.55;
  color: var(--text-secondary);
}

.scene-card.active .scene-card-title,
.scene-card.active .scene-card-desc {
  color: #000000;
  font-weight: 700;
}

.summary-text {
  font-size: 22rpx;
  line-height: 1.7;
  color: var(--text-secondary);
}

</style>

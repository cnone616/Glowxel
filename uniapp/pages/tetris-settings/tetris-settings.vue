<template>
  <view class="tetris-page glx-page-shell">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar glx-topbar glx-page-shell__fixed">
      <view class="nav-left" @click="handleBack">
        <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
      </view>
      <text class="nav-title glx-topbar__title">俄罗斯方块屏保</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="preview-canvas-container" :style="previewCanvasBoxStyle">
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
          canvas-id="tetrisScreensaverPreviewCanvas"
        />
      </view>
      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-title">预览效果</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
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
        <view class="card glx-panel-card glx-editor-card tetris-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">模式</text>
          </view>
          <view class="option-row option-row-double">
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.clearMode }"
              @click="config.clearMode = true"
            >
              <text>消除模式</text>
            </view>
            <view
              class="option-btn glx-feature-option"
              :class="{ active: !config.clearMode }"
              @click="config.clearMode = false"
            >
              <text>满屏模式</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card tetris-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">方块大小</text>
          </view>
          <view class="option-row option-row-triple">
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.cellSize === 1 }"
              @click="config.cellSize = 1"
            >
              <text>小 (1px)</text>
            </view>
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.cellSize === 2 }"
              @click="config.cellSize = 2"
            >
              <text>中 (2px)</text>
            </view>
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.cellSize === 3 }"
              @click="config.cellSize = 3"
            >
              <text>大 (3px)</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card tetris-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">下落速度</text>
          </view>
          <view class="option-row option-row-triple">
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.speed === 'slow' }"
              @click="config.speed = 'slow'"
            >
              <text>慢</text>
            </view>
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.speed === 'normal' }"
              @click="config.speed = 'normal'"
            >
              <text>中</text>
            </view>
            <view
              class="option-btn glx-feature-option"
              :class="{ active: config.speed === 'fast' }"
              @click="config.speed = 'fast'"
            >
              <text>快</text>
            </view>
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card tetris-section-card">
          <view class="card-title-section glx-panel-head inline-row">
            <text class="card-title glx-panel-title">显示时间</text>
            <GlxSwitch class="glx-row-switch" :checked="config.showClock" @change="config.showClock = $event.detail.value" />
          </view>
        </view>

        <view class="card glx-panel-card glx-editor-card tetris-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">方块类型</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="(piece, idx) in pieceTypes"
              :key="idx"
              class="glx-feature-option glx-feature-option--tile"
              :class="{ active: config.pieces.includes(idx) }"
              @click="togglePiece(idx)"
            >
              <view class="piece-preview" :style="{ backgroundColor: piece.color }"></view>
              <text class="piece-name">{{ piece.name }}</text>
            </view>
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
import GlxSwitch from "../../components/GlxSwitch.vue";
import {
  createTetrisScreensaverPreviewState,
  renderTetrisScreensaverPreviewState,
  stepTetrisScreensaverPreviewState,
} from "../../utils/tetrisScreensaverPreview.js";

const TETRIS_SPEED_OPTIONS = {
  slow: 300,
  normal: 150,
  fast: 80,
};

function createDefaultConfig() {
  return {
    clearMode: true,
    cellSize: 2,
    speed: "normal",
    showClock: true,
    pieces: [0, 1, 2, 3, 4, 5, 6],
  };
}

function normalizeSavedConfig(saved) {
  const base = createDefaultConfig();
  if (!saved || typeof saved !== "object") {
    return base;
  }

  const normalized = createDefaultConfig();
  if (typeof saved.clearMode === "boolean") {
    normalized.clearMode = saved.clearMode;
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    normalized.cellSize = saved.cellSize;
  }
  if (saved.speed === "slow" || saved.speed === "normal" || saved.speed === "fast") {
    normalized.speed = saved.speed;
  }
  if (typeof saved.showClock === "boolean") {
    normalized.showClock = saved.showClock;
  }
  if (Array.isArray(saved.pieces)) {
    const pieces = saved.pieces
      .map((item) => Number(item))
      .filter((item) => Number.isInteger(item) && item >= 0 && item < 7);
    if (pieces.length > 0) {
      normalized.pieces = Array.from(new Set(pieces)).sort((left, right) => left - right);
    }
  }
  return normalized;
}

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast, PixelCanvas, GlxSwitch },
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
      previewTimer: null,
      previewRefreshTimer: null,
      config: createDefaultConfig(),
      pieceTypes: [
        { name: "I", color: "#00F0F0" },
        { name: "O", color: "#F0F000" },
        { name: "T", color: "#A000F0" },
        { name: "S", color: "#00F000" },
        { name: "Z", color: "#F00000" },
        { name: "J", color: "#0000F0" },
        { name: "L", color: "#F0A000" },
      ],
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
    const saved = uni.getStorageSync("tetris_config");
    this.config = normalizeSavedConfig(saved);
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
    schedulePreviewRefresh() {
      if (!this.previewCanvasReady) {
        return;
      }
      if (this.previewRefreshTimer) {
        clearTimeout(this.previewRefreshTimer);
        this.previewRefreshTimer = null;
      }
      this.previewRefreshTimer = setTimeout(() => {
        this.previewState = createTetrisScreensaverPreviewState(this.config);
        this.currentPreviewMap = renderTetrisScreensaverPreviewState(this.previewState);
        this.startPreviewPlayback();
      }, 80);
    },
    startPreviewPlayback() {
      this.stopPreviewPlayback();
      if (!this.previewState) {
        return;
      }
      const playNext = () => {
        const delay = Number.isFinite(this.previewState.frameDelay)
          ? this.previewState.frameDelay
          : 110;
        this.previewTimer = setTimeout(() => {
          if (!this.previewState) {
            return;
          }
          stepTetrisScreensaverPreviewState(this.previewState);
          this.currentPreviewMap = renderTetrisScreensaverPreviewState(this.previewState);
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
      this.previewState = null;
    },
    togglePiece(idx) {
      const pieceIndex = this.config.pieces.indexOf(idx);
      if (pieceIndex >= 0) {
        if (this.config.pieces.length <= 1) {
          this.toast.showError("至少保留一种方块");
          return;
        }
        this.config.pieces.splice(pieceIndex, 1);
      } else {
        this.config.pieces.push(idx);
        this.config.pieces.sort();
      }
    },
    async saveAndApply() {
      uni.setStorageSync("tetris_config", this.config);
      try {
        const ws = this.deviceStore.getWebSocket();
        await ws.waitForCommand(
          {
            cmd: "set_mode",
            mode: "tetris",
            clearMode: this.config.clearMode,
            cellSize: this.config.cellSize,
            speed: TETRIS_SPEED_OPTIONS[this.config.speed],
            showClock: this.config.showClock,
            pieces: this.config.pieces,
          },
          "tetris started",
          5000,
        );
        const status = await ws.getStatus();
        if (status.effectMode !== "tetris") {
          throw new Error("设备未进入俄罗斯方块屏保");
        }
        this.deviceStore.setDeviceMode("tetris", { businessMode: true });
        this.toast.showSuccess("已应用");
        const pages = getCurrentPages();
        if (pages.length >= 2) {
          const prev = pages[pages.length - 2];
          if (prev && prev.deviceMode !== undefined) {
            prev.deviceMode = "tetris";
            uni.setStorageSync("device_mode", "tetris");
          }
        }
      } catch (err) {
        console.error("发送失败:", err);
        this.toast.showError("发送失败");
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

.tetris-section-card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.option-row-double {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.option-row-triple {
  grid-template-columns: repeat(3, minmax(0, 1fr));
  box-sizing: border-box;
}

.option-btn.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  color: var(--nb-ink) !important;
}

.option-btn.glx-feature-option.active text {
  color: var(--nb-ink) !important;
  font-weight: 900 !important;
}

.inline-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.piece-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10rpx;
}

.piece-preview {
  width: 44rpx;
  height: 44rpx;
  border: 2rpx solid var(--nb-ink);
  box-sizing: border-box;
}

.piece-name {
  font-size: 22rpx;
  line-height: 1;
}

.glx-feature-option.active .piece-name {
  color: var(--nb-ink);
  font-weight: 700;
}
</style>

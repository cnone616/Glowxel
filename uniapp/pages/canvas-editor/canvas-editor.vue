<template>
  <view class="canvas-page glx-page-shell">
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
      <text class="nav-title glx-topbar__title">画板模式</text>
      <view class="nav-right"></view>
    </view>

    <view class="canvas-section">
      <view class="canvas-container">
        <PixelCanvas
          v-if="canvasReady && !shouldShowSendingSnapshot"
          :width="64"
          :height="64"
          :pixels="pixels"
          :zoom="zoom"
          :offset-x="pan.x"
          :offset-y="pan.y"
          :canvas-width="containerSize.width"
          :canvas-height="containerSize.height"
          :grid-visible="true"
          :allow-single-touch-pan="currentTool === 'move'"
          :is-dark-mode="true"
          canvas-id="deviceCanvasEditor"
          @pixel-click="handlePixelClick"
          @pan="handlePan"
          @zoom="handlePinchZoom"
        />
        <PixelPreviewBoard
          v-else-if="canvasReady && shouldShowSendingSnapshot"
          :width="64"
          :height="64"
          :pixels="sendingPreviewPixels"
          :refresh-token="sendingPreviewTick"
          :zoom="zoom"
          :offset-x="pan.x"
          :offset-y="pan.y"
          :grid-visible="true"
          :is-dark-mode="true"
        />
      </view>

      <view class="preview-caption glx-preview-panel">
        <view class="preview-caption-info glx-preview-panel__info">
          <text class="preview-caption-title">预览效果</text>
        </view>
        <view class="preview-actions">
          <view
            class="action-btn-sm primary glx-primary-action"
            :class="{ disabled: isSending }"
            @click="publishCanvas"
          >
            <Icon name="link" :size="36" color="var(--nb-ink)" />
            <text>发送</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content glx-scroll-region glx-page-shell__content" :style="{ height: contentHeight }">
      <view class="content-wrapper glx-scroll-stack">
        <view class="action-grid">
          <view
            class="panel-btn glx-action-tile"
            :class="{ disabled: historyIndex <= 0 }"
            @click="handleUndo"
          >
            <Icon name="back" :size="36" />
            <text>撤销</text>
          </view>
          <view
            class="panel-btn glx-action-tile"
            :class="{ disabled: historyIndex >= history.length - 1 }"
            @click="handleRedo"
          >
            <Icon name="forward" :size="36" />
            <text>重做</text>
          </view>
          <view class="panel-btn glx-action-tile" @click="handleZoom(-1)">
            <Icon name="zoom-out" :size="36" />
            <text>缩小</text>
          </view>
          <view class="panel-btn glx-action-tile" @click="handleZoom(1)">
            <Icon name="zoom-in" :size="36" />
            <text>放大</text>
          </view>
          <view class="panel-btn glx-action-tile" @click="handleFit">
            <Icon name="fullscreen-expand" :size="36" />
            <text>适配</text>
          </view>
          <view class="panel-btn danger glx-action-tile" @click="clearCanvas">
            <Icon name="delete" :size="36" />
            <text>清空</text>
          </view>
        </view>
        <view class="card glx-panel-card glx-editor-card canvas-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">绘制工具</text>
          </view>

          <view class="tool-grid">
            <view
              class="tool-card glx-tool-tile"
              :class="{ active: currentTool === 'move' }"
              @click="setTool('move')"
            >
              <Icon name="move" :size="40" />
              <text class="tool-card-label">拖动</text>
            </view>
            <view
              class="tool-card glx-tool-tile"
              :class="{ active: currentTool === 'pencil' }"
              @click="setTool('pencil')"
            >
              <Icon name="edit" :size="40" />
              <text class="tool-card-label">绘画</text>
            </view>
            <view
              class="tool-card eraser glx-tool-tile"
              :class="{ active: currentTool === 'eraser' }"
              @click="setTool('eraser')"
            >
              <Icon name="delete" :size="40" />
              <text class="tool-card-label">擦除</text>
            </view>
          </view>
        </view>
        <view v-if="currentTool !== 'move'" class="card glx-panel-card glx-editor-card canvas-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">笔触大小</text>
          </view>
          <view class="option-row">
            <view
              v-for="size in brushSizeOptions"
              :key="size"
              class="option-btn glx-feature-option"
              :class="{ active: brushSize === size }"
              @click="brushSize = size"
            >
              <text class="glx-feature-option__label">{{ size }}x{{ size }}</text>
            </view>
          </view>
        </view>

        <view v-if="currentTool !== 'move'" class="card glx-panel-card glx-editor-card canvas-section-card">
          <view class="card-title-section glx-panel-head">
            <text class="card-title glx-panel-title">画笔颜色</text>
          </view>
          <view class="color-panel-wrap">
            <ColorPanelPicker
              :value="selectedColor"
              label="画笔颜色"
              :preset-colors="[]"
              @input="handleColorSelect"
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

const CANVAS_PIXELS_KEY = "canvas_mode_pixels";

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
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      deviceCanvasModeReady: false,
      currentTool: "pencil",
      brushSize: 1,
      brushSizeOptions: [1, 2, 3, 4],
      selectedColor: "#64c8ff",
      zoom: 4,
      pan: { x: 0, y: 0 },
      containerSize: { width: 320, height: 320 },
      canvasReady: false,
      pixels: new Map(),
      sendingPreviewPixels: new Map(),
      sendingPreviewTick: 0,
      history: [],
      historyIndex: -1,
    };
  },
  computed: {},
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    this.loadPixels();
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    this.initCanvas();
  },
  methods: {
    captureSendingPreview() {
      this.sendingPreviewPixels = new Map(this.pixels);
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
    setTool(newTool) {
      if (this.currentTool !== newTool) {
        this.currentTool = newTool;
      }
    },

    handleBack() {
      uni.navigateBack();
    },

    initCanvas() {
      setTimeout(() => {
        const systemInfo = uni.getSystemInfoSync();
        const statusBarHeight = systemInfo.statusBarHeight || 0;
        const query = uni.createSelectorQuery().in(this);

        query.select(".canvas-section").boundingClientRect((sectionRect) => {
          if (!sectionRect || !sectionRect.height) {
            return;
          }

          const nextHeight =
            systemInfo.windowHeight - statusBarHeight - 88 - sectionRect.height;
          this.contentHeight = `${Math.max(120, nextHeight)}px`;
        });

        query.select(".canvas-container").boundingClientRect((data) => {
          if (!data || data.width <= 0) {
            return;
          }

          this.containerSize = { width: data.width, height: data.height };
          const fitZoomW = (data.width * 0.96) / 64;
          const fitZoomH = (data.height * 0.96) / 64;
          const fitZoom = Math.floor(Math.min(fitZoomW, fitZoomH, 20));

          this.zoom = Math.max(2, fitZoom);
          this.pan = {
            x: (data.width - 64 * this.zoom) / 2,
            y: (data.height - 64 * this.zoom) / 2,
          };
          this.canvasReady = true;

          if (this.history.length === 0) {
            this.history = [new Map(this.pixels)];
            this.historyIndex = 0;
          }
        });

        query.exec();
      }, 120);
    },

    loadPixels() {
      const savedPixels = uni.getStorageSync(CANVAS_PIXELS_KEY);
      if (Array.isArray(savedPixels)) {
        this.pixels = new Map(savedPixels);
      } else {
        this.pixels = new Map();
      }
      this.history = [new Map(this.pixels)];
      this.historyIndex = 0;
    },

    persistPixels() {
      uni.setStorageSync(CANVAS_PIXELS_KEY, Array.from(this.pixels.entries()));
    },

    pushHistory(newPixels) {
      const newHistory = this.history.slice(0, this.historyIndex + 1);
      newHistory.push(new Map(newPixels));
      if (newHistory.length > 50) {
        newHistory.shift();
      }
      this.history = newHistory;
      this.historyIndex = newHistory.length - 1;
    },

    handlePixelClick(x, y) {
      if (this.currentTool === "move") {
        return;
      }

      const newPixels = new Map(this.pixels);
      const updates = [];
      const startX = x - Math.floor((this.brushSize - 1) / 2);
      const startY = y - Math.floor((this.brushSize - 1) / 2);

      for (let offsetX = 0; offsetX < this.brushSize; offsetX += 1) {
        for (let offsetY = 0; offsetY < this.brushSize; offsetY += 1) {
          const pixelX = startX + offsetX;
          const pixelY = startY + offsetY;

          if (pixelX < 0 || pixelX >= 64 || pixelY < 0 || pixelY >= 64) {
            continue;
          }

          const key = `${pixelX},${pixelY}`;

          if (this.currentTool === "eraser") {
            if (!newPixels.has(key)) {
              continue;
            }
            newPixels.delete(key);
            updates.push({ x: pixelX, y: pixelY, color: "#000000" });
            continue;
          }

          if (newPixels.get(key) === this.selectedColor) {
            continue;
          }

          newPixels.set(key, this.selectedColor);
          updates.push({ x: pixelX, y: pixelY, color: this.selectedColor });
        }
      }

      if (updates.length === 0) {
        return;
      }

      this.pixels = newPixels;
      this.pushHistory(newPixels);
      this.persistPixels();
    },

    handleUndo() {
      if (this.historyIndex <= 0) {
        return;
      }
      this.historyIndex -= 1;
      this.pixels = new Map(this.history[this.historyIndex]);
      this.persistPixels();
    },

    handleRedo() {
      if (this.historyIndex >= this.history.length - 1) {
        return;
      }
      this.historyIndex += 1;
      this.pixels = new Map(this.history[this.historyIndex]);
      this.persistPixels();
    },

    handleZoom(delta) {
      const centerX = this.containerSize.width / 2;
      const centerY = this.containerSize.height / 2;
      const newZoom = Math.max(1, Math.min(20, this.zoom + delta));
      const scale = newZoom / this.zoom;
      this.pan = {
        x: centerX - (centerX - this.pan.x) * scale,
        y: centerY - (centerY - this.pan.y) * scale,
      };
      this.zoom = newZoom;
    },

    handlePan(dx, dy) {
      this.pan = { x: this.pan.x + dx, y: this.pan.y + dy };
    },

    handlePinchZoom(delta, centerX, centerY) {
      const newZoom = Math.max(1, Math.min(20, this.zoom + delta));
      const scale = newZoom / this.zoom;
      this.pan = {
        x: centerX - (centerX - this.pan.x) * scale,
        y: centerY - (centerY - this.pan.y) * scale,
      };
      this.zoom = newZoom;
    },

    handleFit() {
      const fitZoomW = (this.containerSize.width * 0.96) / 64;
      const fitZoomH = (this.containerSize.height * 0.96) / 64;
      const fitZoom = Math.floor(Math.min(fitZoomW, fitZoomH, 20));
      this.zoom = Math.max(2, fitZoom);
      this.pan = {
        x: (this.containerSize.width - 64 * this.zoom) / 2,
        y: (this.containerSize.height - 64 * this.zoom) / 2,
      };
    },

    handleColorSelect(color) {
      if (!color) {
        return;
      }
      this.selectedColor = color;
    },

    clearCanvas() {
      this.pixels = new Map();
      this.pushHistory(this.pixels);
      this.persistPixels();
      this.toast.showInfo("画板已清空");
    },

    async publishCanvas() {
      this.persistPixels();

      if (
        !this.guardBeforeSend(
          this.deviceStore.connected,
          "已保存到本地，请先连接设备",
          "info",
        )
      ) {
        return;
      }

      await this.saveAndApply();
    },

    async saveAndApply() {
      this.persistPixels();

      if (!this.deviceStore.connected) {
        return;
      }

      const previousMode = this.deviceStore.deviceMode;
      this.beginSendUi();
      try {
        await this.ensureDeviceCanvasMode();

        if (this.pixels.size === 0) {
          await this.deviceStore.sendSparseImage([], 64, 64);
          this.showSendSuccess();
          return;
        }

        const sparsePixels = [];
        this.pixels.forEach((color, key) => {
          const [x, y] = key.split(",").map(Number);
          const rgb = this.hexToRgb(color);
          sparsePixels.push(x, y, rgb.r, rgb.g, rgb.b);
        });

        await this.deviceStore.sendSparseImage(sparsePixels, 64, 64);
        this.showSendSuccess();
      } catch (err) {
        await this.deviceStore.rollbackBusinessMode(previousMode, {
          expectedMode: "canvas",
        });
        console.error("画板发送失败:", err);
        this.showSendFailure(err);
      } finally {
        this.endSendUi();
      }
    },

    async ensureDeviceCanvasMode() {
      if (!this.deviceStore.connected) {
        return;
      }

      if (this.deviceCanvasModeReady) {
        return;
      }

      await this.deviceStore.ensureCanvasMode();
      this.deviceCanvasModeReady = true;
    },

    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result
        ? {
            r: parseInt(result[1], 16),
            g: parseInt(result[2], 16),
            b: parseInt(result[3], 16),
          }
        : { r: 0, g: 0, b: 0 };
    },
  },

  onUnload() {
  },

  onHide() {
  },
};
</script>

<style scoped>
.canvas-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.status-bar {
  background-color: #1a1a1a;
}

.canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
  background-color: #000000;
  touch-action: none;
}

.action-btn-sm.disabled,
.panel-btn.disabled {
  opacity: 0.4;
}

.sending-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.sending-modal {
  min-width: 420rpx;
  padding: 60rpx 50rpx;
  border-radius: 0;
  background: var(--bg-elevated);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
}

.sending-spinner {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  line-height: 1;
}

.sending-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.sending-tip {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.content {
  flex: 1;
  min-height: 0;
  background: var(--bg-tertiary);
  padding: 16rpx 20rpx 0;
}

.canvas-section-card {
  background: transparent !important;
  border: 0 !important;
  box-shadow: none !important;
}

.option-btn.glx-feature-option.active {
  background: var(--nb-yellow) !important;
  border-color: var(--nb-ink) !important;
  color: var(--nb-ink) !important;
}

.option-btn.glx-feature-option.active .glx-feature-option__label {
  color: var(--nb-ink) !important;
  font-weight: 900 !important;
}

.tool-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 10rpx;
}

.tool-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6rpx;
  min-height: 104rpx;
  padding: 12rpx 10rpx;
  border-radius: 0;
  background-color: #ffffff;
  border: 2rpx solid #000000;
  color: var(--text-primary);
}

.tool-card.active {
  background: var(--nb-yellow);
  border-color: var(--nb-ink);
  color: #000000;
  box-shadow: none;
}

.tool-card.eraser.active {
  background: var(--nb-yellow);
  border-color: var(--nb-ink);
  color: #000000;
  box-shadow: none;
}

.tool-card-label {
  font-size: 22rpx;
  font-weight: 600;
  color: currentColor;
}

.action-grid {
  display: grid;
  grid-template-columns: repeat(6, minmax(0, 1fr));
  gap: 10rpx;
}

.panel-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6rpx;
  min-height: 88rpx;
  padding: 10rpx 8rpx;
  border-radius: 0;
  background: #ffffff;
  border: 2rpx solid #000000;
  color: var(--text-primary);
}

.panel-btn text {
  font-size: 20rpx;
  color: currentColor;
  font-weight: 600;
  line-height: 1.1;
}

.panel-btn.danger {
  color: var(--error-color);
  border-color: rgba(255, 100, 100, 0.28);
}

.color-panel-wrap :deep(.color-panel-picker) {
  background: #ffffff;
  border: 0;
}
</style>

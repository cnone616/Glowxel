<template>
  <view class="canvas-page">
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
        <text class="project-name">画板模式</text>
      </view>
    </view>

    <view class="canvas-section">
      <view v-if="!canvasHidden" class="canvas-container">
        <PixelCanvas
          v-if="canvasReady"
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
      </view>
      <view v-else class="canvas-container canvas-placeholder"></view>

      <view class="preview-caption">
        <view class="preview-caption-info">
          <text class="preview-caption-title">64 x 64 模拟预览</text>
          <text class="preview-caption-text">{{ previewCaptionText }}</text>
        </view>
        <view class="preview-actions">
          <view class="action-btn-sm" @click="saveLocal">
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
            <text>保存</text>
          </view>
          <view
            class="action-btn-sm primary"
            :class="{ disabled: isSending }"
            @click="publishCanvas"
          >
            <Icon name="link" :size="36" color="#fff" />
            <text>{{ isSending ? "发送中" : "发送" }}</text>
          </view>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <view class="action-grid">
          <view
            class="panel-btn"
            :class="{ disabled: historyIndex <= 0 }"
            @click="handleUndo"
          >
            <Icon name="back" :size="36" />
            <text>撤销</text>
          </view>
          <view
            class="panel-btn"
            :class="{ disabled: historyIndex >= history.length - 1 }"
            @click="handleRedo"
          >
            <Icon name="forward" :size="36" />
            <text>重做</text>
          </view>
          <view class="panel-btn" @click="handleZoom(-1)">
            <Icon name="zoom-out" :size="36" />
            <text>缩小</text>
          </view>
          <view class="panel-btn" @click="handleZoom(1)">
            <Icon name="zoom-in" :size="36" />
            <text>放大</text>
          </view>
          <view class="panel-btn" @click="handleFit">
            <Icon name="fullscreen-expand" :size="36" />
            <text>适配</text>
          </view>
          <view class="panel-btn danger" @click="clearCanvas">
            <Icon name="delete" :size="36" />
            <text>清空</text>
          </view>
        </view>
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">绘制工具</text>
          </view>

          <view class="tool-grid">
            <view
              class="tool-card"
              :class="{ active: currentTool === 'move' }"
              @click="setTool('move')"
            >
              <Icon name="move" :size="40" />
              <text class="tool-card-label">拖动</text>
            </view>
            <view
              class="tool-card"
              :class="{ active: currentTool === 'pencil' }"
              @click="setTool('pencil')"
            >
              <Icon name="edit" :size="40" />
              <text class="tool-card-label">绘画</text>
            </view>
            <view
              class="tool-card eraser"
              :class="{ active: currentTool === 'eraser' }"
              @click="setTool('eraser')"
            >
              <Icon name="delete" :size="40" />
              <text class="tool-card-label">擦除</text>
            </view>
          </view>
        </view>
        <view v-if="currentTool !== 'move'" class="card">
          <view class="card-title-section">
            <text class="card-title">笔触大小</text>
            <text class="card-subtitle">{{ brushSize }} x {{ brushSize }}</text>
          </view>
          <view class="option-row">
            <view
              v-for="size in brushSizeOptions"
              :key="size"
              class="option-btn"
              :class="{ active: brushSize === size }"
              @click="brushSize = size"
            >
              <text>{{ size }}x{{ size }}</text>
            </view>
          </view>
        </view>

        <view v-if="currentTool !== 'move'" class="card">
          <view class="card-title-section">
            <text class="card-title">画笔颜色</text>
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

    <Toast ref="toastRef" @show="canvasHidden = true" @hide="onToastHide" />
  </view>
</template>

<script>
import statusBarMixin from "../../mixins/statusBar.js";
import Icon from "../../components/Icon.vue";
import Toast from "../../components/Toast.vue";
import PixelCanvas from "../../components/PixelCanvas.vue";
import ColorPanelPicker from "../../components/ColorPanelPicker.vue";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";

const CANVAS_PIXELS_KEY = "canvas_mode_pixels";

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast, PixelCanvas, ColorPanelPicker },
  data() {
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx - 520rpx)",
      isSending: false,
      livePreviewEnabled: true,
      deviceCanvasModeReady: false,
      syncQueue: [],
      syncTimer: null,
      currentTool: "pencil",
      brushSize: 1,
      brushSizeOptions: [1, 2, 3, 4],
      selectedColor: "#64c8ff",
      zoom: 4,
      pan: { x: 0, y: 0 },
      containerSize: { width: 320, height: 320 },
      canvasReady: false,
      canvasHidden: false,
      pixels: new Map(),
      history: [],
      historyIndex: -1,
    };
  },
  computed: {
    previewCaptionText() {
      if (this.currentTool === "move") {
        return "拖动画布查看细节，缩放后可继续调整位置";
      }

      const toolLabel = this.currentTool === "pencil" ? "绘画" : "擦除";
      return `${toolLabel}模式，当前笔触 ${this.brushSize} x ${this.brushSize}`;
    },
  },
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
    setTool(newTool) {
      if (this.currentTool !== newTool) {
        this.currentTool = newTool;
      }
    },

    handleBack() {
      this.flushQueuedSync();
      uni.navigateBack();
    },
    onToastHide() {
      this.canvasHidden = false;
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
          const fitZoomW = (data.width * 0.9) / 64;
          const fitZoomH = (data.height * 0.9) / 64;
          const fitZoom = Math.min(fitZoomW, fitZoomH, 20);

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
      this.queuePixelBatchSync(updates);
    },

    handleUndo() {
      if (this.historyIndex <= 0) {
        return;
      }
      this.historyIndex -= 1;
      this.pixels = new Map(this.history[this.historyIndex]);
      this.persistPixels();
      this.syncCurrentCanvasToDevice();
    },

    handleRedo() {
      if (this.historyIndex >= this.history.length - 1) {
        return;
      }
      this.historyIndex += 1;
      this.pixels = new Map(this.history[this.historyIndex]);
      this.persistPixels();
      this.syncCurrentCanvasToDevice();
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
      const fitZoomW = (this.containerSize.width * 0.9) / 64;
      const fitZoomH = (this.containerSize.height * 0.9) / 64;
      const fitZoom = Math.min(fitZoomW, fitZoomH, 20);
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
      this.queueClearSync();
      this.toast.showInfo("画板已清空");
    },

    saveLocal() {
      this.persistPixels();
      this.toast.showSuccess("已保存到本地");
    },

    async publishCanvas() {
      if (this.isSending) {
        return;
      }

      this.persistPixels();

      if (!this.deviceStore.connected) {
        this.toast.showInfo("已保存到本地，请先连接设备");
        return;
      }

      await this.saveAndApply();
    },

    async saveAndApply() {
      this.persistPixels();

      if (!this.deviceStore.connected) {
        return;
      }

      this.isSending = true;
      try {
        await this.ensureDeviceCanvasMode();
        const ws = this.deviceStore.getWebSocket();

        if (this.pixels.size === 0) {
          await ws.send({ cmd: "clear_canvas" });
          this.deviceStore.setDeviceMode("canvas", { businessMode: false });
          this.toast.showSuccess("已清空并发布到设备");
          return;
        }

        const sparsePixels = [];
        this.pixels.forEach((color, key) => {
          const [x, y] = key.split(",").map(Number);
          const rgb = this.hexToRgb(color);
          sparsePixels.push(x, y, rgb.r, rgb.g, rgb.b);
        });

        await this.deviceStore.sendSparseImage(sparsePixels, 64, 64);
        await ws.send({ cmd: "save_canvas" });
        this.deviceStore.setDeviceMode("canvas", { businessMode: false });
        this.toast.showSuccess("已发布到设备");
      } catch (err) {
        console.error("画板发送失败:", err);
        this.toast.showError("发送失败");
      } finally {
        this.isSending = false;
      }
    },

    queuePixelBatchSync(updates) {
      if (!this.livePreviewEnabled || !this.deviceStore.connected) {
        return;
      }

      const mergedUpdates = new Map();
      this.syncQueue.forEach((item) => {
        mergedUpdates.set(`${item.x},${item.y}`, item);
      });
      updates.forEach((item) => {
        const rgb = this.hexToRgb(item.color);
        mergedUpdates.set(`${item.x},${item.y}`, {
          x: item.x,
          y: item.y,
          r: rgb.r,
          g: rgb.g,
          b: rgb.b,
        });
      });
      this.syncQueue = Array.from(mergedUpdates.values());

      if (this.syncTimer) {
        clearTimeout(this.syncTimer);
      }

      this.syncTimer = setTimeout(() => {
        this.flushQueuedSync();
      }, 80);
    },

    queueClearSync() {
      if (!this.livePreviewEnabled || !this.deviceStore.connected) {
        return;
      }

      if (this.syncTimer) {
        clearTimeout(this.syncTimer);
        this.syncTimer = null;
      }

      this.syncQueue = [];
      this.flushQueuedSync(true);
    },

    async flushQueuedSync(clearOnly = false) {
      if (!this.deviceStore.connected) {
        return;
      }

      if (!clearOnly && this.syncQueue.length === 0) {
        return;
      }

      if (this.syncTimer) {
        clearTimeout(this.syncTimer);
        this.syncTimer = null;
      }

      try {
        await this.ensureDeviceCanvasMode();

        if (clearOnly) {
          await this.deviceStore.clear();
          return;
        }

        const queue = this.syncQueue.slice();
        this.syncQueue = [];
        const pixelData = [];
        queue.forEach((item) => {
          pixelData.push(item.x, item.y, item.r, item.g, item.b);
        });
        await this.deviceStore.sendPartialUpdate(pixelData, 64, 64);
      } catch (err) {
        console.error("实时预览同步失败:", err);
      }
    },

    async syncCurrentCanvasToDevice() {
      if (!this.livePreviewEnabled || !this.deviceStore.connected) {
        return;
      }

      if (this.syncTimer) {
        clearTimeout(this.syncTimer);
        this.syncTimer = null;
      }

      this.syncQueue = [];

      try {
        await this.ensureDeviceCanvasMode();

        if (this.pixels.size === 0) {
          await this.deviceStore.clear();
          return;
        }

        const sparsePixels = [];
        this.pixels.forEach((color, key) => {
          const [x, y] = key.split(",").map(Number);
          const rgb = this.hexToRgb(color);
          sparsePixels.push(x, y, rgb.r, rgb.g, rgb.b);
        });

        await this.deviceStore.sendSparseImage(sparsePixels, 64, 64);
      } catch (err) {
        console.error("整屏同步失败:", err);
      }
    },

    async ensureDeviceCanvasMode() {
      if (!this.deviceStore.connected) {
        return;
      }

      if (this.deviceCanvasModeReady) {
        return;
      }

      const ws = this.deviceStore.getWebSocket();
      await ws.setMode("canvas");
      this.deviceStore.setDeviceMode("canvas", { businessMode: false });
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
    this.flushQueuedSync();
    if (this.syncTimer) {
      clearTimeout(this.syncTimer);
      this.syncTimer = null;
    }
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

.header {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
  flex-shrink: 0;
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
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.project-name {
  font-size: 33rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.canvas-section {
  display: flex;
  flex-direction: column;
  background: #000000;
  border-bottom: 2rpx solid var(--border-primary);
  flex-shrink: 0;
}

.canvas-container {
  width: 100%;
  aspect-ratio: 1;
  position: relative;
  overflow: hidden;
  background-color: #000000;
  touch-action: none;
}

.canvas-placeholder {
  background-color: #000000;
}

.preview-caption {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 14rpx 20rpx 18rpx;
  background: var(--bg-tertiary);
  border-bottom: 1rpx solid var(--border-color);
}

.preview-caption-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.preview-caption-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.preview-caption-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-actions {
  display: flex;
  align-items: center;
  gap: 12rpx;
  flex-shrink: 0;
}

.action-btn-sm {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 14rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn-sm.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.action-btn-sm.disabled,
.panel-btn.disabled {
  opacity: 0.4;
}

.preview-actions .action-btn-sm {
  width: auto;
  min-width: 118rpx;
  height: 64rpx;
  padding: 0 18rpx;
  gap: 10rpx;
  border-radius: 18rpx;
}

.preview-actions .action-btn-sm text {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
  line-height: 1;
}

.preview-actions .action-btn-sm.primary text {
  color: #ffffff;
}

.content {
  flex: 1;
  min-height: 0;
  background: var(--bg-tertiary);
  padding: 20rpx;
}

.content-wrapper {
  padding-bottom: 48rpx;
}

.card {
  padding-top: 20rpx;
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8rpx;
  margin-bottom: 16rpx;
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
  border-radius: 14rpx;
  background-color: #f7f7f7;
  border: 1rpx solid #e6e6e6;
  color: var(--text-primary);
}

.tool-card.active {
  background: #eef4ff;
  border-color: #4f7fff;
  color: var(--accent-primary);
  box-shadow: none;
}

.tool-card.eraser.active {
  background: #fff1f1;
  border-color: var(--error-color);
  color: var(--error-color);
  box-shadow: none;
}

.tool-card-label {
  font-size: 22rpx;
  font-weight: 600;
  color: currentColor;
}

.option-row {
  display: flex;
  align-items: center;
  gap: 10rpx;
}

.option-btn {
  flex: 1;
  min-width: 0;
  height: 62rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 14rpx;
  background: #f7f7f7;
  border: 1rpx solid #e6e6e6;
}

.option-btn text {
  font-size: 23rpx;
  color: var(--text-secondary);
}

.option-btn.active {
  background: #eef4ff;
  border-color: var(--accent-primary);
  box-shadow: none;
}

.option-btn.active text {
  color: var(--accent-primary);
  font-weight: 600;
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
  border-radius: 14rpx;
  background: #f7f7f7;
  border: 1rpx solid #e6e6e6;
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

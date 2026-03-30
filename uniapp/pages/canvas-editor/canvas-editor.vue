<template>
  <view class="canvas-page">
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->

    <view class="navbar">
      <view class="nav-left" @click="handleBack">
        <Icon
          name="direction-left"
          :size="32"
          color="var(--color-text-primary)"
        />
      </view>
      <text class="nav-title">画板模式</text>
    </view>

    <scroll-view scroll-y class="content">
      <view class="card">
        <view class="card-title-section">
          <text class="card-title">画板预览</text>
          <text class="card-subtitle">64 x 64 像素</text>
        </view>

        <view class="canvas-shell">
          <view class="canvas-container">
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
              :grid-visible="gridVisible"
              :allow-single-touch-pan="currentTool === 'move'"
              :is-dark-mode="true"
              canvas-id="deviceCanvasEditor"
              @pixel-click="handlePixelClick"
              @pan="handlePan"
              @zoom="handlePinchZoom"
            />
          </view>
        </view>

        <view class="toolbar">
          <view class="toolbar-group">
            <view
              class="tool-btn"
              :class="{ disabled: historyIndex <= 0 }"
              @click="handleUndo"
            >
              <Icon name="back" :size="28" />
            </view>
            <view
              class="tool-btn"
              :class="{ disabled: historyIndex >= history.length - 1 }"
              @click="handleRedo"
            >
              <Icon name="forward" :size="28" />
            </view>
          </view>

          <view class="toolbar-group">
            <view
              class="tool-btn"
              :class="{ active: currentTool === 'pencil' }"
              @click="currentTool = 'pencil'"
            >
              <Icon name="edit" :size="28" />
            </view>
            <view
              class="tool-btn"
              :class="{ active: currentTool === 'eraser' }"
              @click="currentTool = 'eraser'"
            >
              <Icon name="ashbin" :size="28" />
            </view>
            <view
              class="tool-btn"
              :class="{ active: currentTool === 'move' }"
              @click="currentTool = 'move'"
            >
              <Icon name="move" :size="28" />
            </view>
          </view>

          <view class="toolbar-group">
            <view class="tool-btn" @click="handleZoom(-1)">
              <Icon name="zoom-out" :size="28" />
            </view>
            <view class="tool-btn" @click="handleZoom(1)">
              <Icon name="zoom-in" :size="28" />
            </view>
            <view class="tool-btn" @click="handleFit">
              <Icon name="fullscreen-expand" :size="28" />
            </view>
            <view
              class="tool-btn"
              :class="{ active: gridVisible }"
              @click="gridVisible = !gridVisible"
            >
              <Icon name="a-Grid-ninejiugongge" :size="28" />
            </view>
          </view>
        </view>
      </view>

      <view class="card">
        <view class="card-title-section">
          <text class="card-title">常用颜色</text>
          <text class="card-subtitle">点击即可切换画笔颜色</text>
        </view>

        <view class="quick-color-grid">
          <view
            v-for="item in quickColors"
            :key="item.hex"
            class="quick-color-item"
            :class="{ active: selectedColor === item.hex }"
            @click="selectedColor = item.hex"
          >
            <view
              class="quick-color-dot"
              :style="{ backgroundColor: item.hex }"
            ></view>
            <text class="quick-color-name">{{ item.name }}</text>
          </view>
        </view>
      </view>

      <view class="card palette-card">
        <view class="card-title-section">
          <text class="card-title">完整色板</text>
          <text class="card-subtitle">当前颜色 {{ selectedColorCode }}</text>
        </view>
        <ColorPalette
          :colors="paletteColors"
          :selected-color="selectedColor"
          @select-color="handlePaletteSelect"
        />
      </view>

      <view class="save-section">
        <view class="secondary-btn" @click="clearCanvas">
          <text class="secondary-btn-text">清空画板</text>
        </view>
        <view class="save-btn" :class="{ disabled: isSending }" @click="saveAndApply">
          <text class="save-btn-text">{{
            isSending ? "发送中..." : "保存并应用"
          }}</text>
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
import ColorPalette from "../../components/ColorPalette.vue";
import { ARTKAL_COLORS_FULL } from "../../data/artkal-colors-full.js";
import { useDeviceStore } from "../../store/device.js";
import { useToast } from "../../composables/useToast.js";

const CANVAS_PIXELS_KEY = "canvas_mode_pixels";

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast, PixelCanvas, ColorPalette },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isSending: false,
      livePreviewEnabled: true,
      deviceCanvasModeReady: false,
      syncQueue: [],
      syncTimer: null,
      currentTool: "pencil",
      selectedColor: "#64c8ff",
      gridVisible: true,
      zoom: 4,
      pan: { x: 0, y: 0 },
      containerSize: { width: 320, height: 320 },
      canvasReady: false,
      pixels: new Map(),
      history: [],
      historyIndex: -1,
      paletteColors: ARTKAL_COLORS_FULL,
      quickColors: [
        { name: "青色", hex: "#64c8ff" },
        { name: "绿色", hex: "#00ff9d" },
        { name: "黄色", hex: "#ffdc00" },
        { name: "橙色", hex: "#ffa500" },
        { name: "红色", hex: "#ff6464" },
        { name: "紫色", hex: "#c864ff" },
        { name: "蓝色", hex: "#6464ff" },
        { name: "白色", hex: "#ffffff" },
      ],
    };
  },
  computed: {
    selectedColorCode() {
      const matchedColor = this.paletteColors.find(
        (item) => item.hex === this.selectedColor,
      );
      if (!matchedColor) {
        return this.selectedColor;
      }
      return matchedColor.code;
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
    handleBack() {
      this.flushQueuedSync();
      uni.navigateBack();
    },

    initCanvas() {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this);
        query
          .select(".canvas-container")
          .boundingClientRect((data) => {
            if (!data || data.width <= 0) {
              return;
            }

            this.containerSize = { width: data.width, height: data.width };
            const fitZoom = Math.floor((data.width * 0.9) / 64);
            this.zoom = Math.max(2, fitZoom);
            this.pan = {
              x: (data.width - 64 * this.zoom) / 2,
              y: (data.width - 64 * this.zoom) / 2,
            };
            this.canvasReady = true;

            if (this.history.length === 0) {
              this.history = [new Map(this.pixels)];
              this.historyIndex = 0;
            }
          })
          .exec();
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

      const key = `${x},${y}`;
      const newPixels = new Map(this.pixels);
      let previewColor = null;

      if (this.currentTool === "eraser") {
        if (!newPixels.has(key)) {
          return;
        }
        newPixels.delete(key);
        previewColor = "#000000";
      } else {
        if (newPixels.get(key) === this.selectedColor) {
          return;
        }
        newPixels.set(key, this.selectedColor);
        previewColor = this.selectedColor;
      }

      this.pixels = newPixels;
      this.pushHistory(newPixels);
      this.persistPixels();
      this.queuePixelSync(x, y, previewColor);
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
      const size = this.containerSize.width;
      const fitZoom = Math.floor((size * 0.9) / 64);
      this.zoom = Math.max(2, fitZoom);
      this.pan = {
        x: (size - 64 * this.zoom) / 2,
        y: (size - 64 * this.zoom) / 2,
      };
    },

    handlePaletteSelect(color) {
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

    async saveAndApply() {
      this.persistPixels();

      if (!this.deviceStore.connected) {
        this.toast.showSuccess("已保存到本地");
        return;
      }

      this.isSending = true;
      try {
        await this.ensureDeviceCanvasMode();
        const ws = this.deviceStore.getWebSocket();

        if (this.pixels.size === 0) {
          await ws.send({ cmd: "clear_canvas" });
          this.deviceStore.setDeviceMode("canvas", { businessMode: false });
          this.toast.showSuccess("已清空并保存到设备");
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
        this.toast.showSuccess("已保存并应用到设备");
      } catch (err) {
        console.error("画板发送失败:", err);
        this.toast.showError("发送失败");
      } finally {
        this.isSending = false;
      }
    },

    queuePixelSync(x, y, color) {
      if (!this.livePreviewEnabled || !this.deviceStore.connected) {
        return;
      }

      const rgb = this.hexToRgb(color);
      const nextQueue = this.syncQueue.filter(
        (item) => item.x !== x || item.y !== y,
      );
      nextQueue.push({ x, y, r: rgb.r, g: rgb.g, b: rgb.b });
      this.syncQueue = nextQueue;

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
  min-height: 100vh;
  background-color: #f5f5f7;
}

.status-bar {
  background-color: #1a1a1a;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 1rpx solid rgba(0, 0, 0, 0.05);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 24rpx;
  top: 50%;
  transform: translateY(-50%);
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.nav-title {
  font-size: 34rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.content {
  height: calc(100vh - 88rpx - var(--status-bar-height, 0px));
  padding: 24rpx;
  box-sizing: border-box;
}

.card {
  background-color: #ffffff;
  border-radius: 24rpx;
  padding: 28rpx;
  margin-bottom: 24rpx;
  box-shadow: 0 12rpx 32rpx rgba(26, 26, 26, 0.06);
}

.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24rpx;
}

.card-title {
  font-size: 30rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.card-subtitle {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.canvas-shell {
  background: linear-gradient(180deg, #101010 0%, #000000 100%);
  border-radius: 24rpx;
  padding: 20rpx;
}

.canvas-container {
  width: 100%;
  aspect-ratio: 1;
  border-radius: 20rpx;
  overflow: hidden;
  background-color: #000000;
}

.toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
  margin-top: 24rpx;
}

.toolbar-group {
  display: flex;
  gap: 12rpx;
}

.tool-btn {
  width: 72rpx;
  height: 72rpx;
  border-radius: 18rpx;
  background-color: #f4f6fa;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #4a5568;
}

.tool-btn.active {
  background-color: rgba(79, 127, 255, 0.12);
  color: #4f7fff;
}

.tool-btn.disabled {
  opacity: 0.4;
}

.quick-color-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 16rpx;
}

.quick-color-item {
  border-radius: 18rpx;
  background-color: #f6f8fc;
  padding: 18rpx 12rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10rpx;
  border: 2rpx solid transparent;
}

.quick-color-item.active {
  border-color: #4f7fff;
  background-color: rgba(79, 127, 255, 0.08);
}

.quick-color-dot {
  width: 44rpx;
  height: 44rpx;
  border-radius: 50%;
  border: 2rpx solid rgba(0, 0, 0, 0.08);
}

.quick-color-name {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.palette-card :deep(.color-palette-container) {
  min-height: 720rpx;
}

.save-section {
  display: flex;
  gap: 16rpx;
  margin-top: 8rpx;
  margin-bottom: 40rpx;
}

.secondary-btn,
.save-btn {
  flex: 1;
  height: 96rpx;
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.secondary-btn {
  background-color: #ffffff;
  border: 2rpx solid rgba(79, 127, 255, 0.18);
}

.save-btn {
  background: linear-gradient(135deg, #4f7fff 0%, #6c94ff 100%);
  box-shadow: 0 18rpx 36rpx rgba(79, 127, 255, 0.22);
}

.save-btn.disabled {
  opacity: 0.6;
}

.secondary-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #4f7fff;
}

.save-btn-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #ffffff;
}
</style>

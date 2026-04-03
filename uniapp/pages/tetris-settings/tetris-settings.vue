<template>
  <view class="tetris-page">
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
        <text class="project-name">方块屏保</text>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <view class="action-strip">
          <view class="action-strip-info">
            <text class="action-strip-title">调整完成后发送到设备</text>
            <text class="action-strip-text">当前配置会作为方块屏保显示</text>
          </view>
          <view class="preview-actions">
            <view
              class="action-btn-sm primary"
              :class="{ disabled: isSending }"
              @click="saveAndApply"
            >
              <Icon name="link" :size="32" color="#fff" />
              <text>{{ isSending ? "发送中" : "发送" }}</text>
            </view>
          </view>
        </view>

        <!-- 模式 -->
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">模式</text>
          </view>
          <view class="option-row">
            <view
              class="option-btn"
              :class="{ active: config.clearMode }"
              @click="config.clearMode = true"
            >
              <text>消除模式</text>
            </view>
            <view
              class="option-btn"
              :class="{ active: !config.clearMode }"
              @click="config.clearMode = false"
            >
              <text>满屏模式</text>
            </view>
          </view>
        </view>

        <!-- 方块大小 -->
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">方块大小</text>
          </view>
          <view class="option-row">
            <view
              class="option-btn"
              :class="{ active: config.cellSize === 1 }"
              @click="config.cellSize = 1"
            >
              <text>小 (1px)</text>
            </view>
            <view
              class="option-btn"
              :class="{ active: config.cellSize === 2 }"
              @click="config.cellSize = 2"
            >
              <text>中 (2px)</text>
            </view>
            <view
              class="option-btn"
              :class="{ active: config.cellSize === 3 }"
              @click="config.cellSize = 3"
            >
              <text>大 (3px)</text>
            </view>
          </view>
        </view>

        <!-- 下落速度 -->
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">下落速度</text>
          </view>
          <view class="option-row">
            <view
              class="option-btn"
              :class="{ active: config.speed === 'slow' }"
              @click="config.speed = 'slow'"
            >
              <text>慢</text>
            </view>
            <view
              class="option-btn"
              :class="{ active: config.speed === 'normal' }"
              @click="config.speed = 'normal'"
            >
              <text>中</text>
            </view>
            <view
              class="option-btn"
              :class="{ active: config.speed === 'fast' }"
              @click="config.speed = 'fast'"
            >
              <text>快</text>
            </view>
          </view>
        </view>

        <!-- 显示时间 -->
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">显示时间</text>
            <view
              class="toggle-switch"
              @click="config.showClock = !config.showClock"
            >
              <view class="switch-track" :class="{ active: config.showClock }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
        </view>

        <!-- 方块类型 -->
        <view class="card">
          <view class="card-title-section">
            <text class="card-title">方块类型</text>
            <text class="card-subtitle">至少选择一种</text>
          </view>
          <view class="piece-grid">
            <view
              v-for="(piece, idx) in pieceTypes"
              :key="idx"
              class="piece-item"
              :class="{ active: config.pieces.includes(idx) }"
              @click="togglePiece(idx)"
            >
              <view
                class="piece-preview"
                :style="{ backgroundColor: piece.color }"
              ></view>
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

const TETRIS_SHAPES = {
  0: [
    [1, 0],
    [1, 1],
    [1, 2],
    [1, 3],
  ],
  1: [
    [0, 0],
    [1, 0],
    [0, 1],
    [1, 1],
  ],
  2: [
    [1, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
  3: [
    [1, 0],
    [2, 0],
    [0, 1],
    [1, 1],
  ],
  4: [
    [0, 0],
    [1, 0],
    [1, 1],
    [2, 1],
  ],
  5: [
    [0, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
  6: [
    [2, 0],
    [0, 1],
    [1, 1],
    [2, 1],
  ],
};

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      contentHeight: "calc(100vh - 88rpx)",
      isSending: false,
      config: {
        clearMode: true,
        cellSize: 2,
        speed: "normal",
        showClock: true,
        pieces: [0, 1, 2, 3, 4, 5, 6],
      },
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
  onLoad() {
    this.deviceStore = useDeviceStore();
    this.deviceStore.init();
    this.toast = useToast();
    const saved = uni.getStorageSync("tetris_config");
    if (saved) {
      this.config = { ...this.config, ...saved };
    }
  },
  onReady() {
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef);
    }
    const systemInfo = uni.getSystemInfoSync();
    const statusBarHeight = systemInfo.statusBarHeight || 0;
    const reservedHeight = 88;
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - reservedHeight}px`;
  },
  methods: {
    handleBack() {
      uni.navigateBack();
    },
    togglePiece(idx) {
      const i = this.config.pieces.indexOf(idx);
      if (i >= 0) {
        if (this.config.pieces.length <= 1) {
          this.toast.showError("至少保留一种方块");
          return;
        }
        this.config.pieces.splice(i, 1);
      } else {
        this.config.pieces.push(idx);
        this.config.pieces.sort();
      }
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
        const speedMap = { slow: 300, normal: 150, fast: 80 };
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
        this.toast.showSuccess("已应用");
        const pages = getCurrentPages();
        if (pages.length >= 2) {
          const prev = pages[pages.length - 2];
          if (prev && prev.deviceMode !== undefined) {
            prev.deviceMode = "tetris";
          }
        }
        this.deviceStore.setDeviceMode("tetris", { businessMode: true });
      } catch (err) {
        console.error("发送失败:", err);
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
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.project-name {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.content {
  flex: 1;
}

.content-wrapper {
  padding: 24rpx;
}

.action-strip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  padding: 14rpx 20rpx 18rpx;
  margin-bottom: 20rpx;
  border-radius: 24rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
}

.action-strip-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.action-strip-title {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.action-strip-text {
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

.action-btn-sm.disabled {
  opacity: 0.6;
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
  color: #ffffff;
  line-height: 1;
}

.card {
  background: var(--bg-elevated);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  padding: 28rpx;
  margin-bottom: 24rpx;
}
.card-title-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20rpx;
}
.card-title {
  font-size: 30rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.card-subtitle {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.option-row {
  display: flex;
  gap: 16rpx;
}

.option-btn {
  flex: 1;
  height: 72rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s;
}

.option-btn text {
  font-size: 26rpx;
  color: var(--text-secondary);
}

.option-btn.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.option-btn.active text {
  color: var(--accent-primary);
  font-weight: 600;
}

.toggle-switch {
  cursor: pointer;
}

.switch-track {
  width: 88rpx;
  height: 48rpx;
  border-radius: 24rpx;
  background: var(--border-primary);
  display: flex;
  align-items: center;
  padding: 4rpx;
  transition: all 0.3s;
}

.switch-track.active {
  background: var(--accent-primary);
}

.switch-thumb {
  width: 40rpx;
  height: 40rpx;
  border-radius: 20rpx;
  background: #fff;
  transition: all 0.3s;
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.15);
}

.switch-track.active .switch-thumb {
  margin-left: 40rpx;
}

.piece-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 16rpx;
}

.piece-item {
  width: calc(25% - 12rpx);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 0;
  border-radius: 16rpx;
  background: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s;
}

.piece-item.active {
  background: rgba(79, 127, 255, 0.14);
  border-color: var(--accent-primary);
  box-shadow: 0 8rpx 18rpx rgba(79, 127, 255, 0.12);
}

.piece-preview {
  width: 48rpx;
  height: 48rpx;
  border-radius: 10rpx;
}

.piece-name {
  font-size: 24rpx;
  color: var(--text-secondary);
  font-weight: 600;
}

.piece-item.active .piece-name {
  color: var(--accent-primary);
}

</style>

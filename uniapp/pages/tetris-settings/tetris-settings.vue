<template>
  <view class="tetris-page">
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
      <text class="nav-title">方块屏保设置</text>
    </view>

    <scroll-view scroll-y class="content">
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

      <!-- 保存按钮 -->
      <view class="save-section">
        <view class="save-btn" @click="saveAndApply">
          <text class="save-btn-text">保存并应用</text>
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

export default {
  mixins: [statusBarMixin],
  components: { Icon, Toast },
  data() {
    return {
      deviceStore: null,
      toast: null,
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
    this.toast = useToast();
    const saved = uni.getStorageSync("tetris_config");
    if (saved) {
      this.config = { ...this.config, ...saved };
    }
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
      uni.setStorageSync("tetris_config", this.config);
      try {
        const ws = this.deviceStore.getWebSocket();
        const speedMap = { slow: 300, normal: 150, fast: 80 };
        await ws.send({
          cmd: "set_mode",
          mode: "tetris",
          clearMode: this.config.clearMode,
          cellSize: this.config.cellSize,
          speed: speedMap[this.config.speed] || 150,
          showClock: this.config.showClock,
          pieces: this.config.pieces,
        });
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
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.content {
  padding: 24rpx;
}
.card {
  background: #fff;
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
  color: #1a1a1a;
}
.card-subtitle {
  font-size: 24rpx;
  color: #999;
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
  background: #f5f5f7;
  transition: all 0.2s;
}
.option-btn text {
  font-size: 26rpx;
  color: #666;
}
.option-btn.active {
  background: #eef2ff;
  border: 2rpx solid #4f7fff;
}
.option-btn.active text {
  color: #4f7fff;
  font-weight: 600;
}
.option-btn:active {
  transform: scale(0.96);
}
.toggle-switch {
  cursor: pointer;
}
.switch-track {
  width: 88rpx;
  height: 48rpx;
  border-radius: 24rpx;
  background: #ddd;
  display: flex;
  align-items: center;
  padding: 4rpx;
  transition: all 0.3s;
}
.switch-track.active {
  background: #4f7fff;
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
  background: #f5f5f7;
  transition: all 0.2s;
}
.piece-item.active {
  background: #eef2ff;
  border: 2rpx solid #4f7fff;
}
.piece-item:active {
  transform: scale(0.96);
}
.piece-preview {
  width: 48rpx;
  height: 48rpx;
  border-radius: 10rpx;
}
.piece-name {
  font-size: 24rpx;
  color: #666;
  font-weight: 600;
}
.piece-item.active .piece-name {
  color: #4f7fff;
}
.save-section {
  padding: 20rpx 0 60rpx;
}
.save-btn {
  height: 88rpx;
  background: #4f7fff;
  border-radius: 20rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}
.save-btn:active {
  opacity: 0.85;
}
.save-btn-text {
  font-size: 32rpx;
  font-weight: 600;
  color: #fff;
}
</style>

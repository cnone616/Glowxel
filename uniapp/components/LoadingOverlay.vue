<template>
  <view v-if="visible" class="glx-loading-overlay">
    <view class="glx-loading-card">
      <view class="glx-loading-indicator">
        <view class="glx-loading-dot glx-loading-dot--1"></view>
        <view class="glx-loading-dot glx-loading-dot--2"></view>
        <view class="glx-loading-dot glx-loading-dot--3"></view>
      </view>
      <text class="glx-loading-text">{{ message }}</text>
    </view>
  </view>
</template>

<script>
import { bindLoadingInstance, unbindLoadingInstance } from '../composables/useToast.js'

export default {
  data() {
    return {
      visible: false,
      message: '加载中...'
    }
  },

  mounted() {
    bindLoadingInstance(this)
  },

  beforeDestroy() {
    unbindLoadingInstance(this)
  },

  methods: {
    show(message = '加载中...') {
      this.message = message
      this.visible = true
    },

    hide() {
      this.visible = false
    }
  }
}
</script>

<style scoped>
.glx-loading-overlay {
  position: fixed;
  inset: 0;
  z-index: 10001;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  box-sizing: border-box;
  background: rgba(255, 255, 255, 0.72);
}

.glx-loading-card {
  width: 100%;
  max-width: 420rpx;
  padding: 36rpx 32rpx;
  border: 4rpx solid #000000;
  box-shadow: 10rpx 10rpx 0 #000000;
  background: #ffffff;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 22rpx;
  box-sizing: border-box;
}

.glx-loading-indicator {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.glx-loading-dot {
  width: 24rpx;
  height: 24rpx;
  border: 3rpx solid #000000;
  background: #f2cf4a;
  animation: glxLoadingBounce 0.9s ease-in-out infinite;
}

.glx-loading-dot--2 {
  animation-delay: 0.12s;
}

.glx-loading-dot--3 {
  animation-delay: 0.24s;
}

.glx-loading-text {
  font-size: 28rpx;
  font-weight: 900;
  line-height: 1.4;
  color: #000000;
  text-align: center;
}

@keyframes glxLoadingBounce {
  0%,
  80%,
  100% {
    transform: translateY(0);
    opacity: 0.45;
  }

  40% {
    transform: translateY(-8rpx);
    opacity: 1;
  }
}
</style>

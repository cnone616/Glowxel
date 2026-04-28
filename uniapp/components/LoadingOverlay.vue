<template>
  <view v-if="visible" class="glx-loading-overlay">
    <view class="glx-loading-card">
      <GlxInlineLoader
        class="glx-loading-indicator"
        variant="chase"
        size="lg"
      />
      <text class="glx-loading-text">{{ message }}</text>
    </view>
  </view>
</template>

<script>
import GlxInlineLoader from './GlxInlineLoader.vue'
import { bindLoadingInstance, unbindLoadingInstance } from '../composables/useToast.js'

export default {
  components: {
    GlxInlineLoader
  },

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
  background: rgba(255, 255, 255, 0.78);
}

.glx-loading-card {
  width: 100%;
  max-width: 620rpx;
  padding: 48rpx 40rpx 44rpx;
  border: 4rpx solid #000000;
  box-shadow: 10rpx 10rpx 0 #000000;
  background: #ffffff;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  box-sizing: border-box;
}

.glx-loading-indicator {
  flex-shrink: 0;
}

.glx-loading-text {
  font-size: 30rpx;
  font-weight: 900;
  line-height: 1.4;
  color: #000000;
  text-align: center;
}
</style>

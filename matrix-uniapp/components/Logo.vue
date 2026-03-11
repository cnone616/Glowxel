<template>
  <view class="logo-container" :style="{ gap: showText ? '24rpx' : '0' }">
    <!-- Logo 图标 - 使用 matrix-logo.png -->
    <image 
      src="/static/matrix-logo.png" 
      class="logo-image"
      :style="{ width: size + 'rpx', height: size + 'rpx' }"
      mode="aspectFit"
    />
    
    <!-- MATRIX 文字 -->
    <view v-if="showText" class="logo-text">
      <text class="text-content" :style="{ fontSize: textSize + 'rpx' }">MATRIX</text>
    </view>
  </view>
</template>

<script>
export default {
  props: {
    size: {
      type: Number,
      default: 64
    },
    showText: {
      type: Boolean,
      default: true
    }
  },
  
  computed: {
    textSize() {
      return this.size * 0.7
    }
  }
}
</script>

<style scoped>
.logo-container {
  display: flex;
  align-items: center;
}

.logo-image {
  flex-shrink: 0;
}

.logo-text {
  user-select: none;
}

.text-content {
  color: #4F7FFF;
  font-family: monospace;
  font-weight: bold;
  letter-spacing: 0.15em;
  line-height: 1;
}
</style>

<template>
  <view class="logo-container" :style="{ gap: showText ? '24rpx' : '0' }">
    <!-- Logo 图标 -->
    <image
      src="/static/RenLight-logo.png"
      class="logo-image"
      :style="{ width: size + 'rpx', height: size + 'rpx' }"
      mode="aspectFit"
    />

    <!-- RenLight 渐变文字 -->
    <text
      v-if="showText"
      class="logo-text"
      :style="{ fontSize: textSize + 'rpx' }"
      >RenLight</text
    >
  </view>
</template>

<script>
export default {
  props: {
    size: {
      type: Number,
      default: 64,
    },
    showText: {
      type: Boolean,
      default: true,
    },
  },

  computed: {
    textSize() {
      return this.size * 0.7;
    },
  },
};
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
  background: linear-gradient(
    to right,
    rgb(249, 115, 22),
    rgb(239, 68, 68),
    rgb(251, 191, 36),
    rgb(59, 130, 246)
  );
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  font-family: monospace;
  font-weight: bold;
  letter-spacing: 0.1em;
  line-height: 1;
  user-select: none;
}
</style>

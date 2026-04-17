<template>
  <view class="glx-logo" :class="{ 'glx-logo--compact': !showText }">
    <image
      src="/static/glowxel-icon.png"
      class="glx-logo__image"
      :style="{ width: size + 'rpx', height: size + 'rpx' }"
      mode="aspectFit"
    />
    <view v-if="showText" class="glx-logo__copy">
      <text class="glx-logo__title">光格像素工坊</text>
      <text class="glx-logo__tag">CLEANROOM</text>
    </view>
  </view>
</template>

<script>
export default {
  name: "GlxLogo",
  props: {
    size: {
      type: Number,
      default: 72,
    },
    showText: {
      type: Boolean,
      default: true,
    },
  },
};
</script>

<style scoped lang="scss">
.glx-logo {
  display: inline-flex;
  align-items: center;
  gap: 20rpx;
}

.glx-logo--compact {
  gap: 0;
}

.glx-logo__image {
  flex-shrink: 0;
}

.glx-logo__copy {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.glx-logo__title {
  color: var(--glx-text);
  font-size: 32rpx;
  font-weight: 800;
  line-height: 1;
  letter-spacing: 1rpx;
}

.glx-logo__tag {
  align-self: flex-start;
  padding: 8rpx 16rpx;
  border-radius: 999rpx;
  background: rgba(74, 134, 239, 0.12);
  color: var(--glx-blue-deep);
  font-size: 18rpx;
  font-weight: 700;
  line-height: 1;
  letter-spacing: 2rpx;
}
</style>

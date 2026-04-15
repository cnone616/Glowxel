<template>
  <view class="logo-container" :style="{ gap: logoGap }">
    <image
      src="/static/glowxel-icon.png"
      class="logo-image"
      :style="{ width: size + 'rpx', height: size + 'rpx' }"
      mode="aspectFit"
    />

    <view v-if="showText" class="logo-copy">
      <text class="logo-title" :style="{ fontSize: titleSize + 'rpx' }">
        光格像素工坊
      </text>
      <text class="logo-tag" :style="{ fontSize: tagSize + 'rpx' }">
        PIXEL STUDIO
      </text>
    </view>
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
    logoGap() {
      return this.showText ? `${Math.round(this.size * 0.22)}rpx` : "0";
    },
    titleSize() {
      return Math.round(this.size * 0.42);
    },
    tagSize() {
      return Math.max(18, Math.round(this.size * 0.18));
    },
  },
};
</script>

<style scoped>
.logo-container {
  display: flex;
  align-items: center;
  padding-bottom: 24rpx;
}

.logo-image {
  flex-shrink: 0;
}

.logo-copy {
  display: flex;
  flex-direction: column;
  gap: 10rpx;
}

.logo-title {
  color: #111111;
  font-weight: 900;
  letter-spacing: 2rpx;
  line-height: 1;
}

.logo-tag {
  align-self: flex-start;
  padding: 6rpx 14rpx 4rpx;
  color: #111111;
  font-weight: 900;
  letter-spacing: 3rpx;
  line-height: 1;
  border: 4rpx solid #111111;
  background: #ffd23f;
  /* box-shadow: 6rpx 6rpx 0 #111111; */
  user-select: none;
}
</style>

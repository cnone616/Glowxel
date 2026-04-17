<template>
  <view v-if="visible" class="glx-loading-mask">
    <view class="glx-loading-mask__panel">
      <GlxLogo :size="92" />
      <view class="glx-loading-mask__meta">
        <text class="glx-loading-mask__title">{{ title }}</text>
        <text class="glx-loading-mask__desc">{{ description }}</text>
      </view>
    </view>
  </view>
</template>

<script>
import GlxLogo from "./GlxLogo.vue";

export default {
  name: "GlxLoadingMask",
  components: {
    GlxLogo,
  },
  props: {
    visible: {
      type: Boolean,
      default: false,
    },
    title: {
      type: String,
      default: "正在整理页面",
    },
    description: {
      type: String,
      default: "先用 Glowxel Logo 占位，后续接正式动效。",
    },
  },
};
</script>

<style scoped lang="scss">
.glx-loading-mask {
  position: fixed;
  inset: 0;
  z-index: 80;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 40rpx;
  background: rgba(238, 247, 247, 0.92);
}

.glx-loading-mask__panel {
  width: 100%;
  max-width: 620rpx;
  padding: 42rpx 38rpx;
  border: 2rpx solid rgba(158, 188, 188, 0.4);
  border-radius: 36rpx;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 26rpx 80rpx rgba(23, 50, 54, 0.12);
}

.glx-loading-mask__meta {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.glx-loading-mask__title {
  color: var(--glx-text);
  font-size: 30rpx;
  font-weight: 800;
  line-height: 1.2;
}

.glx-loading-mask__desc {
  color: var(--glx-text-secondary);
  font-size: 24rpx;
  line-height: 1.6;
}
</style>

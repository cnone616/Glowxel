<template>
  <button class="glx-back-button" @click="handleClick">
    <GlxIcon name="direction-left" :size="26" />
  </button>
</template>

<script>
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxBackButton",
  components: {
    GlxIcon,
  },
  props: {
    fallbackUrl: {
      type: String,
      default: "/pages/tab-home/index",
    },
  },
  methods: {
    handleClick() {
      const pages = getCurrentPages();
      if (pages.length > 1) {
        uni.navigateBack();
        return;
      }
      uni.reLaunch({
        url: this.fallbackUrl,
      });
    },
  },
};
</script>

<style scoped lang="scss">
.glx-back-button {
  width: 72rpx;
  height: 72rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 24rpx;
  border: 2rpx solid rgba(158, 188, 188, 0.5);
  background: rgba(255, 255, 255, 0.94);
  color: var(--glx-text);
  box-shadow: 0 12rpx 28rpx rgba(20, 50, 54, 0.08);
}
</style>

<template>
  <view class="glx-sub-header">
    <view class="glx-sub-header__safe" :style="{ height: layout.statusBarHeight + 'px' }"></view>
    <view class="glx-sub-header__bar" :style="{ minHeight: layout.headerRowHeight + 'px' }">
      <view class="glx-sub-header__side" :style="{ width: layout.titleSideWidth + 'px' }">
        <GlxBackButton :fallback-url="fallbackUrl" />
      </view>
      <view class="glx-sub-header__center">
        <text class="glx-sub-header__title">{{ title }}</text>
        <text v-if="subtitle" class="glx-sub-header__subtitle">{{ subtitle }}</text>
      </view>
      <view class="glx-sub-header__side" :style="{ width: layout.titleSideWidth + 'px' }"></view>
    </view>
  </view>
</template>

<script>
import GlxBackButton from "./GlxBackButton.vue";
import { getLayoutMetrics } from "../../utils/layout";

export default {
  name: "GlxSubHeader",
  components: {
    GlxBackButton,
  },
  props: {
    title: {
      type: String,
      required: true,
    },
    subtitle: {
      type: String,
      default: "",
    },
    fallbackUrl: {
      type: String,
      default: "/pages/tab-home/index",
    },
  },
  data() {
    return {
      layout: getLayoutMetrics(),
    };
  },
};
</script>

<style scoped lang="scss">
.glx-sub-header {
  background: rgba(238, 247, 247, 0.94);
  backdrop-filter: blur(24rpx);
}

.glx-sub-header__bar {
  display: flex;
  align-items: center;
  padding: 12rpx 24rpx 22rpx;
}

.glx-sub-header__side {
  display: flex;
  align-items: center;
}

.glx-sub-header__center {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6rpx;
}

.glx-sub-header__title {
  color: var(--glx-text);
  font-size: 34rpx;
  font-weight: 800;
  line-height: 1.2;
  text-align: center;
}

.glx-sub-header__subtitle {
  color: var(--glx-text-secondary);
  font-size: 22rpx;
  line-height: 1.4;
  text-align: center;
}
</style>

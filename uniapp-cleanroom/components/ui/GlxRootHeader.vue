<template>
  <view class="glx-root-header">
    <view class="glx-root-header__safe" :style="{ height: layout.statusBarHeight + 'px' }"></view>
    <view class="glx-root-header__bar" :style="{ minHeight: layout.headerRowHeight + 'px' }">
      <view class="glx-root-header__side" :style="{ width: layout.titleSideWidth + 'px' }"></view>
      <view class="glx-root-header__center">
        <slot name="center">
          <GlxLogo v-if="showLogo" :size="64" />
          <view v-else class="glx-root-header__copy">
            <text class="glx-root-header__title">{{ title }}</text>
            <text v-if="subtitle" class="glx-root-header__subtitle">{{ subtitle }}</text>
          </view>
        </slot>
      </view>
      <view class="glx-root-header__side" :style="{ width: layout.titleSideWidth + 'px' }"></view>
    </view>
  </view>
</template>

<script>
import GlxLogo from "./GlxLogo.vue";
import { getLayoutMetrics } from "../../utils/layout";

export default {
  name: "GlxRootHeader",
  components: {
    GlxLogo,
  },
  props: {
    title: {
      type: String,
      default: "",
    },
    subtitle: {
      type: String,
      default: "",
    },
    showLogo: {
      type: Boolean,
      default: false,
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
.glx-root-header {
  background: rgba(238, 247, 247, 0.92);
  backdrop-filter: blur(24rpx);
}

.glx-root-header__bar {
  display: flex;
  align-items: center;
  padding: 12rpx 24rpx 24rpx;
}

.glx-root-header__center {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.glx-root-header__copy {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.glx-root-header__title {
  color: var(--glx-text);
  font-size: 36rpx;
  font-weight: 800;
  line-height: 1.2;
  text-align: center;
}

.glx-root-header__subtitle {
  color: var(--glx-text-secondary);
  font-size: 22rpx;
  line-height: 1.4;
  text-align: center;
}
</style>

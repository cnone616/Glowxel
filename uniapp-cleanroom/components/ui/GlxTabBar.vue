<template>
  <view class="glx-tab-bar" :style="{ paddingBottom: layout.bottomSafeArea + 16 + 'px' }">
    <button
      v-for="item in items"
      :key="item.key"
      class="glx-tab-bar__item"
      :class="{ 'glx-tab-bar__item--active': item.key === current }"
      @click="activateTab(item)"
    >
      <GlxIcon :name="item.icon" :size="26" />
      <text class="glx-tab-bar__label">{{ item.label }}</text>
    </button>
  </view>
</template>

<script>
import { TAB_ITEMS } from "../../constants/navigation";
import { getLayoutMetrics } from "../../utils/layout";
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxTabBar",
  components: {
    GlxIcon,
  },
  props: {
    current: {
      type: String,
      required: true,
    },
  },
  data() {
    return {
      items: TAB_ITEMS,
      layout: getLayoutMetrics(),
    };
  },
  methods: {
    activateTab(item) {
      if (item.key === this.current) {
        return;
      }
      uni.reLaunch({
        url: item.route,
      });
    },
  },
};
</script>

<style scoped lang="scss">
.glx-tab-bar {
  position: fixed;
  left: 16rpx;
  right: 16rpx;
  bottom: 16rpx;
  z-index: 50;
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 12rpx;
  padding: 16rpx;
  border: 2rpx solid rgba(158, 188, 188, 0.38);
  border-radius: 34rpx;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 26rpx 74rpx rgba(20, 50, 54, 0.12);
}

.glx-tab-bar__item {
  min-height: 86rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  border-radius: 24rpx;
  color: var(--glx-text-secondary);
}

.glx-tab-bar__item--active {
  background: rgba(119, 215, 198, 0.22);
  color: var(--glx-mint-deep);
}

.glx-tab-bar__label {
  color: inherit;
  font-size: 20rpx;
  font-weight: 700;
  line-height: 1;
}
</style>

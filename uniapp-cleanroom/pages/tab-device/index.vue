<template>
  <view class="page">
    <GlxRootHeader title="设备" subtitle="控制入口和参数入口拆清楚，头部右侧留空" />
    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view class="status-card">
          <text class="status-card__title">当前演示设备</text>
          <text class="status-card__value">Glowxel Matrix 64</text>
          <text class="status-card__desc">这里先放结构化连接信息，后续再接真实设备状态。</text>
          <view class="status-card__actions">
            <GlxActionButton label="设备参数" icon="setting" tone="tool" @click="goParams" />
          </view>
        </view>

        <view class="mode-stack">
          <GlxEntryCard
            v-for="item in deviceModes"
            :key="item.key"
            icon="mobile-phone"
            :title="item.title"
            :description="item.description"
            meta="模式骨架"
            :tone="item.tone"
          />
        </view>
      </view>
    </scroll-view>

    <GlxTabBar current="device" />
    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { DEVICE_MODES } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxRootHeader from "../../components/ui/GlxRootHeader.vue";
import GlxTabBar from "../../components/ui/GlxTabBar.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxActionButton,
    GlxDialogHost,
    GlxEntryCard,
    GlxLoadingMask,
    GlxRootHeader,
    GlxTabBar,
    GlxToastHost,
  },
  data() {
    return {
      deviceModes: DEVICE_MODES,
    };
  },
  methods: {
    goParams() {
      uni.navigateTo({
        url: "/pages/device-params/index",
      });
    },
  },
};
</script>

<style scoped lang="scss">
@import "../../styles/mixins.scss";

.page {
  @include glx-page-shell;
}

.page__scroll {
  height: calc(100vh - 180rpx);
}

.page__body {
  @include glx-scroll-body(236rpx);
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.status-card {
  @include glx-panel-card;
  padding: 28rpx;
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.status-card__title {
  color: var(--glx-text-secondary);
  font-size: 22rpx;
  font-weight: 700;
  line-height: 1;
}

.status-card__value {
  color: var(--glx-text);
  font-size: 38rpx;
  font-weight: 800;
  line-height: 1.2;
}

.status-card__desc {
  @include glx-caption;
}

.status-card__actions,
.mode-stack {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}
</style>

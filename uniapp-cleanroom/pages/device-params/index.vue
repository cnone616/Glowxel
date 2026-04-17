<template>
  <view class="page">
    <GlxSubHeader title="设备参数" subtitle="参数页先按统一列表语义起骨架，不混入另一套视觉语言" fallback-url="/pages/tab-device/index" />
    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view class="param-card">
          <text class="param-card__title">显示参数</text>
          <view class="param-card__rows">
            <GlxEntryCard icon="setting" title="亮度" description="安全上限与夜间亮度" meta="0 - 178" tone="mint" />
            <GlxEntryCard icon="picture" title="主题" description="主题与屏保分组切换" meta="本地主题" tone="blue" />
          </view>
        </view>

        <view class="param-card">
          <text class="param-card__title">连接参数</text>
          <view class="param-card__rows">
            <GlxEntryCard icon="mobile-phone" title="热点配网" description="连接与网络设置" meta="独立子页" tone="amber" />
            <GlxEntryCard icon="user" title="设备命名" description="显示名称与设备别名" meta="待接真实配置" tone="mint" />
          </view>
        </view>
      </view>
    </scroll-view>

    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import pageShell from "../../mixins/pageShell";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxSubHeader from "../../components/ui/GlxSubHeader.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxDialogHost,
    GlxEntryCard,
    GlxLoadingMask,
    GlxSubHeader,
    GlxToastHost,
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
  @include glx-scroll-body(72rpx);
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.param-card {
  @include glx-panel-card;
  padding: 24rpx;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.param-card__title {
  @include glx-section-title;
  font-size: 30rpx;
}

.param-card__rows {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}
</style>

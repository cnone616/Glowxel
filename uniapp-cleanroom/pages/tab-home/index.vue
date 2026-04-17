<template>
  <view class="page">
    <GlxRootHeader show-logo />
    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view v-if="!pageReady" class="page__skeleton">
          <GlxSkeleton height="220rpx" radius="32rpx" />
          <GlxSkeleton height="160rpx" radius="32rpx" />
          <GlxSkeleton height="160rpx" radius="32rpx" />
        </view>

        <template v-else>
          <view class="hero-card">
            <text class="hero-card__eyebrow">新工程并行验证</text>
            <text class="hero-card__title">统一导航、搜索、按钮和加载骨架</text>
            <text class="hero-card__desc">
              这个页面开始承担新工程首页语义，不再借旧工程的全局类去压样式。
            </text>
            <view class="hero-card__actions">
              <GlxActionButton label="进入创作" icon="edit" @click="goCreate" />
              <GlxActionButton
                label="查看社区"
                icon="picture"
                tone="secondary"
                @click="goCommunity"
              />
            </view>
          </view>

          <view v-for="section in sections" :key="section.title" class="section">
            <view class="section__head">
              <text class="section__title">{{ section.title }}</text>
              <text class="section__desc">{{ section.description }}</text>
            </view>
            <view class="section__stack">
              <GlxEntryCard
                v-for="entry in section.entries"
                :key="entry.title"
                icon="picture"
                :title="entry.title"
                :description="section.description"
                :meta="entry.meta"
                :tone="entry.tone"
              />
            </view>
          </view>
        </template>
      </view>
    </scroll-view>

    <GlxTabBar current="home" />
    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { HOME_SECTIONS } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxRootHeader from "../../components/ui/GlxRootHeader.vue";
import GlxSkeleton from "../../components/ui/GlxSkeleton.vue";
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
    GlxSkeleton,
    GlxTabBar,
    GlxToastHost,
  },
  data() {
    return {
      sections: HOME_SECTIONS,
    };
  },
  methods: {
    goCreate() {
      uni.reLaunch({
        url: "/pages/tab-create/index",
      });
    },
    goCommunity() {
      uni.reLaunch({
        url: "/pages/tab-community/index",
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

.page__skeleton {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.hero-card {
  @include glx-panel-card;
  padding: 30rpx;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.98) 0%, rgba(245, 252, 251, 0.96) 100%);
}

.hero-card__eyebrow {
  color: var(--glx-blue-deep);
  font-size: 22rpx;
  font-weight: 700;
  line-height: 1;
}

.hero-card__title {
  color: var(--glx-text);
  font-size: 38rpx;
  font-weight: 800;
  line-height: 1.25;
}

.hero-card__desc {
  @include glx-caption;
}

.hero-card__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 14rpx;
  margin-top: 6rpx;
}

.section {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.section__head {
  display: flex;
  flex-direction: column;
  gap: 6rpx;
}

.section__title {
  @include glx-section-title;
}

.section__desc {
  @include glx-caption;
}

.section__stack {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}
</style>

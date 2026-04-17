<template>
  <view class="page">
    <GlxRootHeader title="我的" subtitle="列表入口和账号模块全部走同一套卡片语义" />
    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view class="profile-card">
          <view class="profile-card__avatar">G</view>
          <view class="profile-card__copy">
            <text class="profile-card__name">Glowxel Demo</text>
            <text class="profile-card__desc">清理旧工程样式污染后的新个人中心骨架。</text>
          </view>
          <GlxActionButton label="编辑" icon="edit" tone="edit" @click="showToast('编辑按钮保留蓝色语义', 'edit')" />
        </view>

        <view class="entry-stack">
          <GlxEntryCard
            v-for="item in profileEntries"
            :key="item.key"
            icon="user"
            :title="item.title"
            :description="item.description"
            meta="骨架页"
            :tone="item.tone"
            @click="handleEntry(item.key)"
          />
        </view>
      </view>
    </scroll-view>

    <GlxTabBar current="profile" />
    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { PROFILE_ENTRIES } from "../../constants/mock-data";
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
      profileEntries: PROFILE_ENTRIES,
    };
  },
  methods: {
    showToast(message, tone) {
      this.$refs.toastRef.show({
        message,
        tone,
      });
    },
    handleEntry(key) {
      if (key === "works") {
        uni.navigateTo({
          url: "/pages/project-works/index",
        });
        return;
      }
      this.showToast("该入口骨架已统一进新工程", "primary");
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

.profile-card {
  @include glx-panel-card;
  display: flex;
  align-items: center;
  gap: 18rpx;
  padding: 28rpx;
}

.profile-card__avatar {
  width: 96rpx;
  height: 96rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 30rpx;
  background: rgba(119, 215, 198, 0.28);
  color: var(--glx-mint-deep);
  font-size: 40rpx;
  font-weight: 800;
}

.profile-card__copy {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.profile-card__name {
  color: var(--glx-text);
  font-size: 30rpx;
  font-weight: 800;
  line-height: 1.2;
}

.profile-card__desc {
  @include glx-caption;
}

.entry-stack {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}
</style>

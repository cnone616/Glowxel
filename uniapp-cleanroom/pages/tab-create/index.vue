<template>
  <view class="page">
    <GlxRootHeader title="创作" subtitle="统一动作按钮，但保留编辑 / 删除等独立语义" />
    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view class="launch-card">
          <text class="launch-card__title">快速开始</text>
          <text class="launch-card__desc">新版创作入口会先把操作区、作品区和草稿区分清楚。</text>
          <view class="launch-card__actions">
            <GlxActionButton label="新建画布" icon="add" @click="showToast('新建画布骨架已接通', 'primary')" />
            <GlxActionButton label="导入素材" icon="picture" tone="secondary" @click="showToast('导入入口待接真实链路', 'edit')" />
          </view>
        </view>

        <view class="section">
          <text class="section__title">最近草稿</text>
          <view class="draft-stack">
            <view v-for="item in projects" :key="item.id" class="draft-card">
              <view class="draft-card__main">
                <text class="draft-card__title">{{ item.title }}</text>
                <text class="draft-card__desc">{{ item.description }}</text>
              </view>
              <view class="draft-card__actions">
                <GlxActionButton label="编辑" icon="edit" tone="edit" @click="showToast('编辑按钮走蓝色语义', 'edit')" />
                <GlxActionButton label="删除" icon="ashbin" tone="danger" @click="confirmDelete(item)" />
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <GlxTabBar current="create" />
    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { PROJECT_ENTRIES } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxRootHeader from "../../components/ui/GlxRootHeader.vue";
import GlxTabBar from "../../components/ui/GlxTabBar.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxActionButton,
    GlxDialogHost,
    GlxLoadingMask,
    GlxRootHeader,
    GlxTabBar,
    GlxToastHost,
  },
  data() {
    return {
      projects: PROJECT_ENTRIES,
    };
  },
  methods: {
    showToast(message, tone) {
      this.$refs.toastRef.show({
        message,
        tone,
      });
    },
    confirmDelete(item) {
      this.$refs.dialogRef.open({
        title: "删除草稿",
        description: `“${item.title}” 现在先走红色危险语义，不再和普通按钮混在一起。`,
        confirmLabel: "确认删除",
        confirmTone: "danger",
        onConfirm: () => {
          this.showToast("删除动作为红色危险语义", "danger");
        },
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

.launch-card,
.draft-card {
  @include glx-panel-card;
  padding: 26rpx;
}

.launch-card {
  display: flex;
  flex-direction: column;
  gap: 14rpx;
}

.launch-card__title,
.section__title {
  @include glx-section-title;
}

.launch-card__desc {
  @include glx-caption;
}

.launch-card__actions,
.draft-card__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.section {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.draft-stack {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.draft-card {
  display: flex;
  flex-direction: column;
  gap: 18rpx;
}

.draft-card__main {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.draft-card__title {
  color: var(--glx-text);
  font-size: 28rpx;
  font-weight: 800;
  line-height: 1.2;
}

.draft-card__desc {
  @include glx-caption;
}
</style>

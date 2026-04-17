<template>
  <view class="page">
    <GlxSubHeader title="我的作品" subtitle="排序入口进入搜索同层工具位" fallback-url="/pages/tab-profile/index" />
    <GlxSearchToolsRow
      v-model="searchTerm"
      placeholder="搜索作品名称或当前状态"
      :has-tools="true"
    >
      <GlxActionButton label="排序" icon="filter" tone="tool" @click="showSortHint" />
    </GlxSearchToolsRow>

    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view v-for="item in filteredProjects" :key="item.id" class="work-card">
          <view class="work-card__main">
            <text class="work-card__title">{{ item.title }}</text>
            <text class="work-card__desc">{{ item.description }}</text>
            <text class="work-card__status">{{ item.status }}</text>
          </view>
          <view class="work-card__actions">
            <GlxActionButton label="编辑" icon="edit" tone="edit" @click="showToast('编辑按钮保持蓝底白字', 'edit')" />
            <GlxActionButton label="删除" icon="ashbin" tone="danger" @click="confirmDelete(item)" />
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
import { PROJECT_ENTRIES } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxSearchToolsRow from "../../components/ui/GlxSearchToolsRow.vue";
import GlxSubHeader from "../../components/ui/GlxSubHeader.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxActionButton,
    GlxDialogHost,
    GlxLoadingMask,
    GlxSearchToolsRow,
    GlxSubHeader,
    GlxToastHost,
  },
  data() {
    return {
      searchTerm: "",
      projects: PROJECT_ENTRIES,
    };
  },
  computed: {
    filteredProjects() {
      if (!this.searchTerm) {
        return this.projects;
      }
      return this.projects.filter((item) => {
        const content = `${item.title}${item.description}${item.status}`.toLowerCase();
        return content.includes(this.searchTerm.toLowerCase());
      });
    },
  },
  methods: {
    showToast(message, tone) {
      this.$refs.toastRef.show({
        message,
        tone,
      });
    },
    showSortHint() {
      this.showToast("排序入口已固定到搜索同层工具位", "edit");
    },
    confirmDelete(item) {
      this.$refs.dialogRef.open({
        title: "删除作品",
        description: `“${item.title}” 使用红色危险语义，和普通按钮分开设计。`,
        confirmLabel: "删除",
        confirmTone: "danger",
        onConfirm: () => {
          this.showToast("删除动作仍保留红色危险语义", "danger");
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
  height: calc(100vh - 264rpx);
}

.page__body {
  @include glx-scroll-body(72rpx);
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.work-card {
  @include glx-panel-card;
  display: flex;
  flex-direction: column;
  gap: 18rpx;
  padding: 26rpx;
}

.work-card__main {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.work-card__title {
  color: var(--glx-text);
  font-size: 28rpx;
  font-weight: 800;
  line-height: 1.2;
}

.work-card__desc,
.work-card__status {
  @include glx-caption;
}

.work-card__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}
</style>

<template>
  <view class="page">
    <GlxSubHeader title="画布总览" subtitle="标题居中，左侧返回统一，右侧固定留空" fallback-url="/pages/tab-create/index" />
    <GlxSearchToolsRow v-model="searchTerm" placeholder="搜索画布编号或区域说明" />

    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <GlxEntryCard
          v-for="item in filteredBoards"
          :key="item.id"
          icon="menu"
          :title="item.title"
          :description="item.description"
          :meta="item.progress"
          tone="mint"
        />
      </view>
    </scroll-view>

    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { BOARD_ENTRIES } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxSearchToolsRow from "../../components/ui/GlxSearchToolsRow.vue";
import GlxSubHeader from "../../components/ui/GlxSubHeader.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxDialogHost,
    GlxEntryCard,
    GlxLoadingMask,
    GlxSearchToolsRow,
    GlxSubHeader,
    GlxToastHost,
  },
  data() {
    return {
      searchTerm: "",
      boards: BOARD_ENTRIES,
    };
  },
  computed: {
    filteredBoards() {
      if (!this.searchTerm) {
        return this.boards;
      }
      return this.boards.filter((item) => {
        const content = `${item.id}${item.title}${item.description}`.toLowerCase();
        return content.includes(this.searchTerm.toLowerCase());
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
</style>

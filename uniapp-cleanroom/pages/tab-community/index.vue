<template>
  <view class="page">
    <GlxRootHeader title="社区" subtitle="固定搜索工具条，右侧不再承载标题栏功能位" />
    <GlxSearchToolsRow
      v-model="searchTerm"
      placeholder="搜索作品标题、作者或灵感方向"
      :has-tools="true"
    >
      <GlxActionButton label="创作者" icon="user" tone="tool" @click="goCreators" />
    </GlxSearchToolsRow>

    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <view v-if="!pageReady" class="page__stack">
          <GlxSkeleton height="132rpx" radius="30rpx" />
          <GlxSkeleton height="132rpx" radius="30rpx" />
          <GlxSkeleton height="132rpx" radius="30rpx" />
        </view>

        <view v-else class="page__stack">
          <GlxEntryCard
            v-for="item in filteredWorks"
            :key="item.id"
            icon="picture"
            :title="item.title"
            :description="item.author"
            :meta="item.stats"
            tone="blue"
          />
        </view>
      </view>
    </scroll-view>

    <GlxTabBar current="community" />
    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { COMMUNITY_WORKS } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxRootHeader from "../../components/ui/GlxRootHeader.vue";
import GlxSearchToolsRow from "../../components/ui/GlxSearchToolsRow.vue";
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
    GlxSearchToolsRow,
    GlxSkeleton,
    GlxTabBar,
    GlxToastHost,
  },
  data() {
    return {
      searchTerm: "",
      works: COMMUNITY_WORKS,
    };
  },
  computed: {
    filteredWorks() {
      if (!this.searchTerm) {
        return this.works;
      }
      return this.works.filter((item) => {
        const content = `${item.title}${item.author}${item.stats}`.toLowerCase();
        return content.includes(this.searchTerm.toLowerCase());
      });
    },
  },
  methods: {
    goCreators() {
      uni.navigateTo({
        url: "/pages/community-users/index",
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
  @include glx-scroll-body(236rpx);
}

.page__stack {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}
</style>

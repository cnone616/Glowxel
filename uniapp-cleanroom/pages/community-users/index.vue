<template>
  <view class="page">
    <GlxSubHeader title="创作者" subtitle="用户列表也走同一搜索组件，不再单独开右上角搜索" fallback-url="/pages/tab-community/index" />
    <GlxSearchToolsRow
      v-model="searchTerm"
      placeholder="搜索创作者昵称或创作方向"
      :has-tools="true"
    >
      <GlxActionButton label="邀请" icon="add" tone="secondary" @click="showToast('邀请入口保留在搜索同层工具位', 'primary')" />
    </GlxSearchToolsRow>

    <scroll-view scroll-y class="page__scroll">
      <view class="page__body">
        <GlxEntryCard
          v-for="item in filteredUsers"
          :key="item.id"
          icon="user"
          :title="item.name"
          :description="item.tag"
          :meta="item.stats"
          tone="blue"
        />
      </view>
    </scroll-view>

    <GlxToastHost ref="toastRef" />
    <GlxDialogHost ref="dialogRef" />
    <GlxLoadingMask :visible="!pageReady" />
  </view>
</template>

<script>
import { CREATOR_LIST } from "../../constants/mock-data";
import pageShell from "../../mixins/pageShell";
import GlxActionButton from "../../components/ui/GlxActionButton.vue";
import GlxDialogHost from "../../components/ui/GlxDialogHost.vue";
import GlxEntryCard from "../../components/ui/GlxEntryCard.vue";
import GlxLoadingMask from "../../components/ui/GlxLoadingMask.vue";
import GlxSearchToolsRow from "../../components/ui/GlxSearchToolsRow.vue";
import GlxSubHeader from "../../components/ui/GlxSubHeader.vue";
import GlxToastHost from "../../components/ui/GlxToastHost.vue";

export default {
  mixins: [pageShell],
  components: {
    GlxActionButton,
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
      users: CREATOR_LIST,
    };
  },
  computed: {
    filteredUsers() {
      if (!this.searchTerm) {
        return this.users;
      }
      return this.users.filter((item) => {
        const content = `${item.name}${item.tag}${item.stats}`.toLowerCase();
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

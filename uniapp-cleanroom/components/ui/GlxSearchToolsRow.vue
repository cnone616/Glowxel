<template>
  <view class="glx-search-tools-row">
    <view class="glx-search-tools-row__search">
      <GlxSearchBar :value="value" :placeholder="placeholder" @input="$emit('input', $event)" />
    </view>
    <view v-if="hasTools" class="glx-search-tools-row__tools">
      <slot></slot>
    </view>
  </view>
</template>

<script>
import GlxSearchBar from "./GlxSearchBar.vue";

export default {
  name: "GlxSearchToolsRow",
  components: {
    GlxSearchBar,
  },
  props: {
    value: {
      type: String,
      default: "",
    },
    placeholder: {
      type: String,
      default: "搜索内容",
    },
    hasTools: {
      type: Boolean,
      default: false,
    },
  },
};
</script>

<style scoped lang="scss">
.glx-search-tools-row {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 0 24rpx 18rpx;
  background: rgba(238, 247, 247, 0.94);
}

.glx-search-tools-row__search {
  flex: 1;
  min-width: 0;
}

.glx-search-tools-row__tools {
  flex-shrink: 0;
  display: inline-flex;
  align-items: center;
  gap: 12rpx;
}
</style>

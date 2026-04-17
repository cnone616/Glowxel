<template>
  <view class="glx-search-bar">
    <GlxIcon name="search" :size="22" class="glx-search-bar__icon" />
    <input
      class="glx-search-bar__input"
      :value="value"
      :placeholder="placeholder"
      placeholder-style="color:#86A3A6;"
      @input="handleInput"
    />
    <button v-if="value" class="glx-search-bar__clear" @click="clearValue">
      <GlxIcon name="close" :size="20" />
    </button>
  </view>
</template>

<script>
import GlxIcon from "./GlxIcon.vue";

export default {
  name: "GlxSearchBar",
  components: {
    GlxIcon,
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
  },
  methods: {
    handleInput(event) {
      this.$emit("input", event.detail.value);
    },
    clearValue() {
      this.$emit("input", "");
    },
  },
};
</script>

<style scoped lang="scss">
.glx-search-bar {
  min-height: 84rpx;
  display: flex;
  align-items: center;
  gap: 14rpx;
  padding: 0 20rpx;
  border-radius: 26rpx;
  border: 2rpx solid rgba(158, 188, 188, 0.48);
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 14rpx 32rpx rgba(20, 50, 54, 0.06);
  color: var(--glx-text-secondary);
}

.glx-search-bar__icon {
  flex-shrink: 0;
}

.glx-search-bar__input {
  flex: 1;
  min-width: 0;
  height: 84rpx;
  color: var(--glx-text);
  font-size: 25rpx;
}

.glx-search-bar__clear {
  width: 44rpx;
  height: 44rpx;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 999rpx;
  background: rgba(100, 129, 133, 0.08);
  color: var(--glx-text-secondary);
}
</style>

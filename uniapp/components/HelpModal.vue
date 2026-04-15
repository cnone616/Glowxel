<template>
  <view v-if="isOpen" class="modal-overlay" @click="handleClose">
    <view class="help-modal" @click.stop>
      <!-- HelpModal -->
      <view class="help-modal__header">
        <view class="help-modal__header-icon">
          <Icon name="help" :size="32" />
        </view>
        <text class="help-modal__title">{{ title }}</text>
        <view class="help-modal__close" @click="handleClose">
          <Icon name="close" :size="32" />
        </view>
      </view>

      <!-- Content -->
      <scroll-view scroll-y class="help-modal__content">
        <view class="help-modal__items">
          <view
            v-for="(item, index) in items"
            :key="index"
            class="help-modal__item"
          >
            <view class="help-modal__item-icon">
              <Icon
                v-if="item.iconName"
                :name="item.iconName"
                :size="32"
                color="var(--nb-ink)"
              />
              <text v-else class="help-modal__item-icon-text">{{
                item.iconText || "•"
              }}</text>
            </view>
            <view class="help-modal__item-content">
              <text class="help-modal__item-title">{{ item.title }}</text>
              <text class="help-modal__item-description">{{
                item.description
              }}</text>
            </view>
          </view>
        </view>
      </scroll-view>

      <!-- Footer -->
      <view class="help-modal__footer">
        <view class="help-modal__confirm" @click="handleClose">
          <text class="help-modal__confirm-text">明白了</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from "./Icon.vue";

export default {
  components: {
    Icon,
  },

  props: {
    isOpen: {
      type: Boolean,
      default: false,
    },
    title: {
      type: String,
      default: "帮助",
    },
    items: {
      type: Array,
      default: () => [],
    },
  },

  methods: {
    handleClose() {
      this.$emit("close");
    },
  },
};
</script>

<style scoped>
.help-modal {
  width: 100%;
  max-width: 640rpx;
  max-height: 80vh;
  background-color: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  border-radius: 0;
  overflow: hidden;
  box-shadow: var(--nb-shadow-strong);
  display: flex;
  flex-direction: column;
}

.help-modal__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
  background-color: var(--nb-surface);
}

.help-modal__header-icon {
  width: 52rpx;
  height: 52rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  background: #f2cf4a;
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}

.help-modal__title {
  flex: 1;
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
  margin-left: 16rpx;
}

.help-modal__close {
  width: 52rpx;
  height: 52rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}

.help-modal__content {
  flex: 1;
  padding: 24rpx;
  overflow-y: auto;
  box-sizing: border-box;
  background: transparent;
  border: none;
  box-shadow: none;
}

/* 小程序减小padding */
/* #ifdef MP-WEIXIN */
.help-modal__content {
  padding: 24rpx;
}
/* #endif */

.help-modal__items {
  display: flex;
  flex-direction: column;
  gap: 0;
  background: transparent;
  border: none;
  box-shadow: none;
}

.help-modal__item {
  display: flex;
  gap: 20rpx;
  padding: 24rpx 4rpx;
  border-radius: 0;
  background-color: transparent;
  border: none;
  box-shadow: none;
}

.help-modal__item + .help-modal__item {
  border-top: var(--nb-border-width-control) solid var(--nb-ink);
}

.help-modal__item-icon {
  width: 80rpx;
  height: 80rpx;
  flex-shrink: 0;
  border-radius: 0;
  background-color: #f2cf4a;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  display: flex;
  align-items: center;
  justify-content: center;
}

.help-modal__item-icon-text {
  font-size: 32rpx;
  color: var(--nb-ink);
}

.help-modal__item-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.help-modal__item-title {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.help-modal__item-description {
  font-size: 24rpx;
  color: #4a4a4a;
  line-height: 1.6;
}

.help-modal__footer {
  padding: 20rpx 24rpx 24rpx;
  border-top: var(--nb-border-width-panel) solid var(--nb-ink);
  background-color: var(--nb-surface);
  box-sizing: border-box;
}

/* 小程序减小padding并添加底部安全区域 */
/* #ifdef MP-WEIXIN */
.help-modal__footer {
  padding: 24rpx;
  padding-bottom: 48rpx;
}
/* #endif */

.help-modal__confirm {
  min-height: 84rpx;
  padding: 0 24rpx;
  background-color: #f2cf4a;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: var(--nb-shadow-soft);
  box-sizing: border-box;
}

.help-modal__confirm-text {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
}
</style>

<template>
  <view v-if="isOpen" class="modal-overlay" @click="handleClose">
    <view class="modal-container" @click.stop>
      <!-- HelpModal -->
      <view class="modal-header">
        <view class="header-icon">
          <Icon name="help" :size="32" />
        </view>
        <text class="modal-title">{{ title }}</text>
        <view class="close-btn" @click="handleClose">
          <Icon name="close" :size="32" />
        </view>
      </view>

      <!-- Content -->
      <scroll-view scroll-y class="modal-content">
        <view class="help-items">
          <view v-for="(item, index) in items" :key="index" class="help-item">
            <view class="item-icon">
              <Icon
                v-if="item.iconName"
                :name="item.iconName"
                :size="32"
                color="var(--accent-color)"
              />
              <text v-else class="item-icon-text">{{
                item.iconText || "•"
              }}</text>
            </view>
            <view class="item-content">
              <text class="item-title">{{ item.title }}</text>
              <text class="item-description">{{ item.description }}</text>
            </view>
          </view>
        </view>
      </scroll-view>

      <!-- Footer -->
      <view class="modal-footer">
        <view class="confirm-btn" @click="handleClose">
          <text class="confirm-btn-text">明白了</text>
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
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.45);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 32rpx;
}

.modal-container {
  width: 100%;
  max-width: 600rpx;
  max-height: 80vh;
  background-color: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  border-radius: 0;
  overflow: hidden;
  box-shadow: var(--nb-shadow-strong);
  display: flex;
  flex-direction: column;
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
  background-color: var(--nb-surface);
}

.header-icon {
  width: 40rpx;
  height: 40rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.icon-text {
  font-size: 32rpx;
  color: var(--accent-color);
}

.modal-title {
  flex: 1;
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
  margin-left: 16rpx;
}

.close-btn {
  width: 40rpx;
  height: 40rpx;
  display: flex;
  align-items: center;
  justify-content: center;
}

.close-icon {
  font-size: 32rpx;
  color: var(--text-secondary);
}

.modal-content {
  flex: 1;
  padding: 32rpx;
  overflow-y: auto;
  box-sizing: border-box;
}

/* 小程序减小padding */
/* #ifdef MP-WEIXIN */
.modal-content {
  padding: 24rpx;
}
/* #endif */

.help-items {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.help-item {
  display: flex;
  gap: 32rpx;
  padding: 24rpx;
  border-radius: 0;
  background-color: #ffffff;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}

.item-icon {
  width: 80rpx;
  height: 80rpx;
  flex-shrink: 0;
  border-radius: 0;
  background-color: var(--nb-yellow);
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  display: flex;
  align-items: center;
  justify-content: center;
}

.item-icon-text {
  font-size: 32rpx;
  color: var(--accent-color);
}

.item-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.item-title {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
}

.item-description {
  font-size: 24rpx;
  color: #4a4a4a;
  line-height: 1.6;
}

.modal-footer {
  padding: 32rpx;
  border-top: var(--nb-border-width-panel) solid var(--nb-ink);
  background-color: var(--nb-surface);
  box-sizing: border-box;
}

/* 小程序减小padding并添加底部安全区域 */
/* #ifdef MP-WEIXIN */
.modal-footer {
  padding: 24rpx;
  padding-bottom: 48rpx;
}
/* #endif */

.confirm-btn {
  padding: 24rpx;
  background-color: var(--nb-yellow);
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: var(--nb-shadow-soft);
}

.confirm-btn-text {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
}
</style>

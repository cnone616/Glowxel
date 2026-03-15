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
          <view 
            v-for="(item, index) in items" 
            :key="index" 
            class="help-item"
          >
            <view class="item-icon">
              <text class="item-icon-text">{{ item.iconText || '•' }}</text>
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
import Icon from './Icon.vue'

export default {
  components: {
    Icon
  },
  
  props: {
    isOpen: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: '帮助'
    },
    items: {
      type: Array,
      default: () => []
    }
  },
  
  methods: {
    handleClose() {
      this.$emit('close')
    }
  }
}
</script>

<style scoped>
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
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
  background-color: var(--bg-tertiary);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
  border-radius: 32rpx;
  overflow: hidden;
  box-shadow: 0 0 60rpx rgba(0, 243, 255, 0.15);
  display: flex;
  flex-direction: column;
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-color);
  background-color: var(--bg-tertiary);
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
  font-weight: bold;
  color: var(--text-primary);
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
  border-radius: 16rpx;
  background-color: rgba(0, 243, 255, 0.05);
  border: 2rpx solid var(--border-color);
}

.item-icon {
  width: 80rpx;
  height: 80rpx;
  flex-shrink: 0;
  border-radius: 50%;
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid rgba(0, 243, 255, 0.2);
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
  font-weight: bold;
  color: var(--text-primary);
}

.item-description {
  font-size: 24rpx;
  color: var(--text-secondary);
  line-height: 1.6;
}

.modal-footer {
  padding: 32rpx;
  border-top: 2rpx solid var(--border-color);
  background-color: var(--bg-tertiary);
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
  background-color: var(--accent-color);
  border-radius: 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 8rpx 32rpx rgba(0, 243, 255, 0.2);
}

.confirm-btn-text {
  font-size: 28rpx;
  font-weight: bold;
  color: #000000;
}
</style>

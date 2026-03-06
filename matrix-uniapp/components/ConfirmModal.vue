<template>
  <view v-if="visible" class="modal-overlay" @tap="handleCancel">
    <view class="modal-content" @tap.stop>
      <view class="modal-header">
        <text class="modal-title">{{ title }}</text>
      </view>
      
      <view class="modal-body">
        <text class="modal-text">{{ content }}</text>
      </view>
      
      <view class="modal-footer">
        <view class="modal-btn cancel-btn" @tap="handleCancel">
          <text class="btn-text">{{ cancelText }}</text>
        </view>
        <view class="modal-btn confirm-btn" :class="{ 'danger': danger }" @tap="handleConfirm">
          <text class="btn-text">{{ confirmText }}</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
export default {
  props: {
    visible: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: '提示'
    },
    content: {
      type: String,
      default: ''
    },
    confirmText: {
      type: String,
      default: '确定'
    },
    cancelText: {
      type: String,
      default: '取消'
    },
    danger: {
      type: Boolean,
      default: false
    }
  },
  
  methods: {
    handleConfirm() {
      this.$emit('confirm')
      this.$emit('update:visible', false)
    },
    
    handleCancel() {
      this.$emit('cancel')
      this.$emit('update:visible', false)
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
  background-color: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(20rpx);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  padding: 64rpx;
}

.modal-content {
  width: 100%;
  max-width: 560rpx;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  border: 2rpx solid var(--border-primary);
  overflow: hidden;
  animation: scaleIn 0.2s ease-out;
}

@keyframes scaleIn {
  from {
    transform: scale(0.9);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

.modal-header {
  padding: 32rpx 32rpx 24rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
  text-align: center;
  display: block;
}

.modal-body {
  padding: 32rpx;
}

.modal-text {
  font-size: 28rpx;
  color: var(--text-secondary);
  line-height: 1.6;
  text-align: center;
  display: block;
}

.modal-footer {
  display: flex;
  border-top: 2rpx solid var(--border-primary);
}

.modal-btn {
  flex: 1;
  padding: 28rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: var(--transition-base);
}

.modal-btn:active {
  opacity: 0.7;
}

.cancel-btn {
  border-right: 2rpx solid var(--border-primary);
}

.btn-text {
  font-size: 28rpx;
  font-weight: 500;
}

.cancel-btn .btn-text {
  color: var(--text-secondary);
}

.confirm-btn .btn-text {
  color: var(--accent-primary);
}

.confirm-btn.danger .btn-text {
  color: #ff3333;
}
</style>

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
.modal-content {
  width: 100%;
  max-width: 620rpx;
  background-color: var(--nb-surface);
  border-radius: 0;
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
  animation: scaleIn 0.18s ease-out;
  box-sizing: border-box;
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
  padding: 24rpx 24rpx 20rpx;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 900;
  color: var(--nb-ink);
  text-align: center;
  display: block;
}

.modal-body {
  padding: 24rpx;
}

.modal-text {
  font-size: 26rpx;
  color: #4a4a4a;
  line-height: 1.6;
  text-align: center;
  display: block;
}

.modal-footer {
  display: flex;
  gap: 12rpx;
  padding: 18rpx 24rpx 24rpx;
  border-top: var(--nb-border-width-panel) solid var(--nb-ink);
}

.modal-btn {
  flex: 1;
  min-height: 84rpx;
  padding: 0 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--nb-surface);
  box-sizing: border-box;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
}


.cancel-btn {
  background: #ffffff;
}

.btn-text {
  font-size: 28rpx;
  font-weight: 900;
}

.cancel-btn .btn-text {
  color: var(--nb-ink);
}

.confirm-btn {
  background: #f2cf4a;
}

.confirm-btn .btn-text {
  color: var(--nb-ink);
}

.confirm-btn.danger {
  background: var(--nb-coral);
}

.confirm-btn.danger .btn-text {
  color: #ffffff;
}
</style>

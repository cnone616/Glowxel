<template>
  <view v-if="visible" class="modal-overlay" @click="handleOverlayClick">
    <view class="modal-container" @click.stop>
      <view class="modal-header">
        <text class="modal-title">{{ title }}</text>
        <view class="close-btn" :class="{ 'disabled': sending }" @click="handleCancel">
          <text class="close-icon">×</text>
        </view>
      </view>
      
      <view class="modal-body">
        <text class="modal-description">{{ description }}</text>
        
        <view class="input-wrapper">
          <textarea
            v-model="inputValue"
            :placeholder="placeholder"
            class="json-input"
            :placeholder-style="placeholderStyle"
            :maxlength="-1"
            :disabled="sending"
            auto-height
          />
        </view>
      </view>
      
      <view class="modal-footer">
        <view class="modal-btn cancel-btn" :class="{ 'disabled': sending }" @click="handleCancel">
          <text class="btn-text">取消</text>
        </view>
        <view class="modal-btn confirm-btn" :class="{ 'disabled': sending }" @click="handleConfirm">
          <text class="btn-text">{{ sending ? '传输中...' : '确定' }}</text>
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
      default: '导入配置'
    },
    description: {
      type: String,
      default: '请粘贴 JSON 配置数据'
    },
    placeholder: {
      type: String,
      default: '粘贴 JSON 数据...'
    },
    sending: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      inputValue: '',
      placeholderStyle: 'color: rgba(255, 255, 255, 0.3); font-size: 24rpx;'
    }
  },
  
  methods: {
    handleOverlayClick() {
      // 传输中不允许点击遮罩关闭
      if (this.sending) return
      this.handleCancel()
    },
    
    handleCancel() {
      // 传输中不允许取消
      if (this.sending) return
      
      this.inputValue = ''
      this.$emit('update:visible', false)
      this.$emit('cancel')
    },
    
    handleConfirm() {
      // 传输中不允许重复点击
      if (this.sending) return
      
      if (!this.inputValue.trim()) {
        this.$emit('error', '请输入 JSON 数据')
        return
      }
      
      try {
        const jsonData = JSON.parse(this.inputValue.trim())
        this.$emit('confirm', jsonData)
      } catch (e) {
        this.$emit('error', 'JSON 格式错误：' + e.message)
      }
    }
  },
  
  watch: {
    visible(newVal) {
      // 当弹窗关闭时，清空输入
      if (!newVal) {
        this.inputValue = ''
      }
    }
  }
}
</script>

<style scoped>
.close-btn.disabled,
.modal-btn.disabled {
  opacity: 0.5;
  pointer-events: none;
}

.json-input:disabled {
  opacity: 0.6;
}

.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.7);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  padding: 48rpx;
}

.modal-container {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-elevated);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  box-shadow: 0 16rpx 64rpx rgba(0, 0, 0, 0.5);
  overflow: hidden;
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx 40rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.close-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.close-btn:active {
  background-color: var(--bg-tertiary);
  transform: scale(0.95);
}

.close-icon {
  font-size: 64rpx;
  color: var(--text-secondary);
  line-height: 1;
}

.modal-body {
  padding: 32rpx 40rpx;
}

.modal-description {
  font-size: 24rpx;
  color: var(--text-secondary);
  margin-bottom: 24rpx;
  display: block;
}

.input-wrapper {
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 16rpx;
  padding: 24rpx;
  transition: var(--transition-base);
}

.input-wrapper:focus-within {
  border-color: var(--accent-primary);
  box-shadow: 0 0 0 4rpx rgba(0, 243, 255, 0.1);
}

.json-input {
  width: 100%;
  min-height: 300rpx;
  max-height: 600rpx;
  font-size: 24rpx;
  color: var(--text-primary);
  background-color: transparent;
  font-family: 'Courier New', monospace;
  line-height: 1.6;
}

.modal-footer {
  display: flex;
  gap: 16rpx;
  padding: 24rpx 40rpx 32rpx;
  border-top: 2rpx solid var(--border-primary);
}

.modal-btn {
  flex: 1;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  transition: var(--transition-base);
  border: 2rpx solid var(--border-primary);
}

.modal-btn:active {
  transform: scale(0.98);
}

.cancel-btn {
  background-color: var(--bg-tertiary);
}

.cancel-btn:active {
  background-color: var(--bg-secondary);
}

.confirm-btn {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
  box-shadow: 0 4rpx 16rpx rgba(0, 243, 255, 0.3);
}

.confirm-btn:active {
  box-shadow: 0 2rpx 8rpx rgba(0, 243, 255, 0.2);
}

.btn-text {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.confirm-btn .btn-text {
  color: #000000;
}
</style>

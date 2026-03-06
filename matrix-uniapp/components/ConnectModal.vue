<template>
  <view v-if="visible" class="modal-overlay" @tap="handleCancel">
    <view class="modal-content" @tap.stop>
      <view class="modal-header">
        <text class="modal-title">{{ title }}</text>
        <view class="close-btn" @tap="handleCancel">
          <Icon name="close" :size="32" />
        </view>
      </view>
      
      <view class="modal-body">
        <text class="modal-desc">{{ description }}</text>
        
        <view class="input-wrapper">
          <input 
            v-model="inputValue"
            type="text"
            class="custom-input"
            :placeholder="placeholder"
            placeholder-class="input-placeholder"
            @confirm="handleConfirm"
          />
        </view>
        
        <!-- 连接状态 -->
        <view v-if="connecting" class="status-box connecting">
          <view class="loading-spinner"></view>
          <text class="status-text">连接中...</text>
        </view>
        
        <view v-if="error" class="status-box error">
          <Icon name="close" :size="32" />
          <text class="status-text">{{ error }}</text>
        </view>
      </view>
      
      <view class="modal-footer">
        <view class="modal-btn cancel-btn" @tap="handleCancel">
          <text class="btn-text">取消</text>
        </view>
        <view 
          class="modal-btn confirm-btn" 
          :class="{ 'disabled': !inputValue || connecting }"
          @tap="handleConfirm"
        >
          <text class="btn-text">{{ connecting ? '连接中' : '连接' }}</text>
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
    visible: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: '连接设备'
    },
    description: {
      type: String,
      default: '请输入 LED 矩阵板的 IP 地址'
    },
    placeholder: {
      type: String,
      default: '192.168.31.84'
    },
    defaultValue: {
      type: String,
      default: ''
    }
  },
  
  data() {
    return {
      inputValue: '',
      connecting: false,
      error: '',
      timeoutTimer: null
    }
  },
  
  watch: {
    visible(val) {
      if (val) {
        this.inputValue = this.defaultValue
        this.connecting = false
        this.error = ''
        if (this.timeoutTimer) {
          clearTimeout(this.timeoutTimer)
          this.timeoutTimer = null
        }
      }
    },
    
    inputValue() {
      // 清除错误提示
      if (this.error) {
        this.error = ''
      }
    }
  },
  
  methods: {
    handleConfirm() {
      if (!this.inputValue || this.connecting) return
      
      this.connecting = true
      this.error = ''
      
      // 设置10秒超时
      this.timeoutTimer = setTimeout(() => {
        if (this.connecting) {
          this.connecting = false
          this.error = '连接超时，请重试'
          this.$emit('timeout')
        }
      }, 10000)
      
      this.$emit('confirm', this.inputValue)
    },
    
    handleCancel() {
      if (this.connecting) return
      
      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer)
        this.timeoutTimer = null
      }
      
      this.$emit('cancel')
      this.$emit('update:visible', false)
      
      // 直接关闭弹窗
      this.$parent.showConnectModal = false
    },
    
    // 外部调用：连接成功
    onSuccess() {
      this.connecting = false
      this.error = ''
      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer)
        this.timeoutTimer = null
      }
      this.$emit('update:visible', false)
      
      // 直接关闭弹窗
      if (this.$parent && this.$parent.showConnectModal !== undefined) {
        this.$parent.showConnectModal = false
      }
    },
    
    // 外部调用：连接失败
    onError(message) {
      this.connecting = false
      this.error = message || '连接失败，请检查 IP 地址'
      if (this.timeoutTimer) {
        clearTimeout(this.timeoutTimer)
        this.timeoutTimer = null
      }
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
  max-width: 600rpx;
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
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.modal-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.close-btn {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  transition: var(--transition-base);
}

.close-btn:active {
  background-color: var(--bg-secondary);
  transform: scale(0.9);
}

.modal-body {
  padding: 32rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.modal-desc {
  font-size: 24rpx;
  color: var(--text-secondary);
  line-height: 1.6;
}

.input-wrapper {
  display: flex;
  flex-direction: column;
}

.custom-input {
  width: 100%;
  padding: 0 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 12rpx;
  font-size: 32rpx;
  color: var(--text-primary);
  font-family: monospace;
  transition: var(--transition-base);
  box-sizing: border-box;
  line-height: 1.2;
  height: 80rpx;
}

.custom-input:focus {
  border-color: var(--accent-primary);
  box-shadow: 0 0 0 4rpx rgba(0, 243, 255, 0.1);
}

.input-placeholder {
  color: var(--text-tertiary);
}

.status-box {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 20rpx;
  border-radius: 12rpx;
  animation: fadeIn 0.3s ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(-10rpx);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.status-box.connecting {
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
}

.status-box.error {
  background-color: rgba(255, 100, 100, 0.1);
  border: 2rpx solid rgba(255, 100, 100, 0.3);
}

.loading-spinner {
  width: 32rpx;
  height: 32rpx;
  border: 4rpx solid rgba(0, 243, 255, 0.2);
  border-top-color: var(--accent-primary);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.status-text {
  font-size: 24rpx;
  font-weight: 500;
}

.status-box.connecting .status-text {
  color: var(--accent-primary);
}

.status-box.error .status-text {
  color: #ff6464;
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

.modal-btn:active:not(.disabled) {
  opacity: 0.7;
}

.modal-btn.disabled {
  opacity: 0.5;
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
</style>

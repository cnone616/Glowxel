<template>
  <view v-if="visible" class="toast-overlay">
    <view :class="['toast-container', `toast-${type}`]">
      <view class="toast-icon-wrapper">
        <image :src="iconSrc" class="toast-icon-image" mode="aspectFit" />
      </view>
      <text class="toast-message">{{ message }}</text>
    </view>
  </view>
</template>

<script>
export default {
  data() {
    return {
      visible: false,
      message: '',
      type: 'success',
      timer: null,
      showTimer: null
    }
  },
  
  computed: {
    iconSrc() {
      // 使用base64编码的SVG图标
      const icons = {
        success: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHZpZXdCb3g9IjAgMCA2NCA2NCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNNTMuMzMzMyAxNkwyNi42NjY3IDQyLjY2NjdMMTYgMzIiIHN0cm9rZT0iIzAwZmY5ZCIgc3Ryb2tlLXdpZHRoPSI0IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz48L3N2Zz4=',
        error: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHZpZXdCb3g9IjAgMCA2NCA2NCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNNDggMTZMMTYgNDhNMTYgMTZMNDggNDgiIHN0cm9rZT0iI2ZmMzMzMyIgc3Ryb2tlLXdpZHRoPSI0IiBzdHJva2UtbGluZWNhcD0icm91bmQiLz48L3N2Zz4=',
        warning: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHZpZXdCb3g9IjAgMCA2NCA2NCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48Y2lyY2xlIGN4PSIzMiIgY3k9IjMyIiByPSIyOCIgc3Ryb2tlPSIjZmY5NzZhIiBzdHJva2Utd2lkdGg9IjQiLz48cGF0aCBkPSJNMzIgMjBWMzZNMzIgNDRWNDYiIHN0cm9rZT0iI2ZmOTc2YSIgc3Ryb2tlLXdpZHRoPSI0IiBzdHJva2UtbGluZWNhcD0icm91bmQiLz48L3N2Zz4=',
        info: 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iNjQiIGhlaWdodD0iNjQiIHZpZXdCb3g9IjAgMCA2NCA2NCIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48Y2lyY2xlIGN4PSIzMiIgY3k9IjMyIiByPSIyOCIgc3Ryb2tlPSIjMTBhZWZmIiBzdHJva2Utd2lkdGg9IjQiLz48cGF0aCBkPSJNMzIgMjhWNDRNMzIgMjBWMjIiIHN0cm9rZT0iIzEwYWVmZiIgc3Ryb2tlLXdpZHRoPSI0IiBzdHJva2UtbGluZWNhcD0icm91bmQiLz48L3N2Zz4='
      }
      return icons[this.type] || icons.success
    }
  },
  
  methods: {
    show(message, type = 'success', duration = 2000) {
      if (this.timer) {
        clearTimeout(this.timer)
      }
      if (this.showTimer) {
        clearTimeout(this.showTimer)
        this.showTimer = null
      }
      
      this.message = message
      this.type = type
      
      // 通知父组件显示toast（用于隐藏canvas）
      this.$emit('show')

      // 让页面先完成 canvas 隐藏/卸载，再显示 toast，避免被原生 canvas 遮挡
      this.showTimer = setTimeout(() => {
        this.visible = true
        this.showTimer = null

        if (duration > 0) {
          this.timer = setTimeout(() => {
            this.hide()
          }, duration)
        }
      }, 32)
    },
    
    hide() {
      if (this.showTimer) {
        clearTimeout(this.showTimer)
        this.showTimer = null
      }
      this.visible = false
      // 通知父组件隐藏toast（用于显示canvas）
      this.$emit('hide')
      
      if (this.timer) {
        clearTimeout(this.timer)
        this.timer = null
      }
    }
  },
  
  beforeDestroy() {
    if (this.timer) {
      clearTimeout(this.timer)
    }
    if (this.showTimer) {
      clearTimeout(this.showTimer)
    }
  }
}
</script>

<style scoped>
.toast-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 10000;
  background-color: rgba(0, 0, 0, 0.3);
}

.toast-container {
  min-width: 280rpx;
  max-width: 480rpx;
  padding: 48rpx 40rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
  border-radius: 32rpx;
  box-shadow: 0 0 60rpx rgba(0, 243, 255, 0.15);
  display: flex;
  flex-direction: column;
  align-items: center;
  animation: toastFadeIn 0.3s ease;
}

@keyframes toastFadeIn {
  from {
    opacity: 0;
    transform: scale(0.9);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}

.toast-icon-wrapper {
  width: 80rpx;
  height: 80rpx;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid rgba(0, 243, 255, 0.2);
  margin-bottom: 24rpx;
}

.toast-success .toast-icon-wrapper {
  background-color: rgba(0, 255, 157, 0.1);
  border-color: rgba(0, 255, 157, 0.3);
}

.toast-error .toast-icon-wrapper {
  background-color: rgba(255, 51, 51, 0.1);
  border-color: rgba(255, 51, 51, 0.3);
}

.toast-warning .toast-icon-wrapper {
  background-color: rgba(255, 151, 106, 0.1);
  border-color: rgba(255, 151, 106, 0.3);
}

.toast-icon-image {
  width: 64rpx;
  height: 64rpx;
}

.toast-message {
  font-size: 28rpx;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
  line-height: 1.4;
}
</style>

<template>
  <view v-if="visible" class="glx-toast-overlay">
    <view :class="['glx-toast-container', `glx-toast--${type}`]">
      <view class="glx-toast-main">
        <view class="glx-toast-icon-wrapper">
          <text class="glx-toast-icon-glyph">{{ iconGlyph }}</text>
        </view>
        <text class="glx-toast-message">{{ message }}</text>
      </view>
      <view class="glx-toast-close" @click="hide">
        <text class="glx-toast-close-glyph">×</text>
      </view>
    </view>
  </view>
</template>

<script>
import { bindToastInstance, unbindToastInstance } from '../composables/useToast.js'

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
    iconGlyph() {
      const icons = {
        success: '✓',
        error: '×',
        warning: '!',
        info: 'i'
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

  mounted() {
    bindToastInstance(this)
  },
  
  beforeDestroy() {
    unbindToastInstance(this)
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
.glx-toast-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  z-index: 10000;
  background-color: transparent;
  pointer-events: none;
  padding: 120rpx 24rpx 0;
  box-sizing: border-box;
}

.glx-toast-container {
  width: 100%;
  min-width: 0;
  max-width: 620rpx;
  min-height: 104rpx;
  padding: 0 18rpx 0 24rpx;
  border: 4rpx solid #000000;
  border-radius: 0;
  box-shadow: 7rpx 7rpx 0 #000000;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
  animation: toastFadeIn 0.18s ease-out;
  pointer-events: auto;
  box-sizing: border-box;
}

@keyframes toastFadeIn {
  from {
    opacity: 0;
    transform: translateY(16rpx);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.glx-toast-main {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 18rpx;
  padding: 22rpx 0;
}

.glx-toast--success {
  background: #b9dc96;
}

.glx-toast--error {
  background: #ff6b6b;
}

.glx-toast--info {
  background: #bcd6ff;
}

.glx-toast--warning {
  background: #f2cf4a;
}

.glx-toast-icon-wrapper {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid #000000;
  background: rgba(255, 255, 255, 0.34);
  box-sizing: border-box;
}

.glx-toast-icon-glyph {
  font-size: 44rpx;
  line-height: 1;
  font-weight: 900;
  color: #000000;
}

.glx-toast-message {
  flex: 1;
  min-width: 0;
  font-size: 26rpx;
  font-weight: 900;
  color: #000000;
  text-align: left;
  line-height: 1.4;
}

.glx-toast-close {
  width: 60rpx;
  height: 60rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 3rpx solid #000000;
  background: rgba(255, 255, 255, 0.34);
  flex-shrink: 0;
  box-sizing: border-box;
}

.glx-toast-close-glyph {
  font-size: 32rpx;
  line-height: 1;
  font-weight: 900;
  color: #000000;
}
</style>

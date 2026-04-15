<template>
  <view v-if="visible" class="modal-overlay" @click="handleOverlayClick">
    <view class="modal-container" :class="modalClasses" @click.stop>
      <!-- 头部 -->
      <view v-if="title || $slots.header" class="modal-header">
        <slot name="header">
          <text class="modal-title">{{ title }}</text>
          <view v-if="closable" class="close-btn" @click="handleClose">
            <Icon name="close" :size="32" color="var(--nb-ink)" />
          </view>
        </slot>
      </view>
      
      <!-- 内容 -->
      <view class="modal-body">
        <slot></slot>
      </view>
      
      <!-- 底部 -->
      <view v-if="$slots.footer" class="modal-footer">
        <slot name="footer"></slot>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'Modal',
  components: {
    Icon
  },
  
  props: {
    // 显示状态
    visible: {
      type: Boolean,
      default: false
    },
    
    // 标题
    title: {
      type: String,
      default: ''
    },
    
    // 尺寸
    size: {
      type: String,
      default: 'medium', // small, medium, large, full
      validator: (value) => ['small', 'medium', 'large', 'full'].includes(value)
    },
    
    // 是否显示关闭按钮
    closable: {
      type: Boolean,
      default: true
    },
    
    // 点击遮罩是否关闭
    maskClosable: {
      type: Boolean,
      default: true
    },
    
    // 自定义样式类
    customClass: {
      type: String,
      default: ''
    }
  },
  
  emits: ['update:visible', 'close', 'open'],
  
  computed: {
    modalClasses() {
      return [
        `modal-${this.size}`,
        this.customClass
      ].filter(Boolean)
    }
  },
  
  watch: {
    visible(newVal) {
      if (newVal) {
        this.$emit('open')
        // 防止页面滚动
        this.preventScroll()
      } else {
        this.$emit('close')
        // 恢复页面滚动
        this.restoreScroll()
      }
    }
  },
  
  methods: {
    handleClose() {
      this.$emit('update:visible', false)
    },
    
    handleOverlayClick() {
      if (this.maskClosable) {
        this.handleClose()
      }
    },
    
    preventScroll() {
      // 微信小程序中防止背景滚动
      // #ifdef MP-WEIXIN
      wx.pageScrollTo({
        scrollTop: 0,
        duration: 0
      })
      // #endif
    },
    
    restoreScroll() {
      // 恢复滚动（如果需要的话）
    }
  }
}
</script>

<style scoped>
.modal-container {
  background-color: var(--nb-surface);
  border: var(--nb-border-width-panel) solid var(--nb-ink);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  max-height: 80vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  animation: modalSlideIn 0.18s ease-out;
}

@keyframes modalSlideIn {
  from {
    opacity: 0;
    transform: scale(0.9) translateY(20rpx);
  }
  to {
    opacity: 1;
    transform: scale(1) translateY(0);
  }
}

/* 尺寸变体 */
.modal-small {
  width: 80%;
  max-width: 600rpx;
}

.modal-medium {
  width: 90%;
  max-width: 800rpx;
}

.modal-large {
  width: 95%;
  max-width: 1000rpx;
}

.modal-full {
  width: 100%;
  height: 100%;
  max-height: 100vh;
  border-radius: 0;
}

/* 头部 */
.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx;
  flex-shrink: 0;
  border-bottom: var(--nb-border-width-panel) solid var(--nb-ink);
  gap: 16rpx;
}

.modal-title {
  font-size: 36rpx;
  font-weight: 900;
  color: var(--nb-ink);
  flex: 1;
}

.close-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  background: var(--nb-surface);
  box-shadow: var(--nb-shadow-soft);
  border-radius: 0;
  margin-left: 16rpx;
  box-sizing: border-box;
}


/* 内容 */
.modal-body {
  padding: 24rpx;
  flex: 1;
  overflow-y: auto;
}

/* 底部 */
.modal-footer {
  padding: 18rpx 24rpx 24rpx;
  flex-shrink: 0;
  display: flex;
  gap: 16rpx;
  justify-content: flex-end;
  border-top: var(--nb-border-width-panel) solid var(--nb-ink);
}

/* 响应式适配 */
@media (max-width: 750rpx) {
  .modal-overlay {
    padding: 24rpx;
  }
  
  .modal-header {
    padding: 24rpx 24rpx 0;
  }
  
  .modal-body {
    padding: 24rpx;
  }
  
  .modal-footer {
    padding: 0 24rpx 24rpx;
  }
  
  .modal-title {
    font-size: 32rpx;
  }
}
</style>

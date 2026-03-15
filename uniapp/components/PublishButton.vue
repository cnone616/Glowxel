<template>
  <view 
    class="publish-button"
    :class="{ 'disabled': disabled, 'loading': loading }"
    @click="handleClick"
  >
    <view class="button-content">
      <view class="button-icon">
        <Icon v-if="!loading" name="upload" :size="32" />
        <view v-else class="loading-spinner">
          <Icon name="loading" :size="32" />
        </view>
      </view>
      <text class="button-text">{{ buttonText }}</text>
    </view>
    
    <!-- 发布提示 -->
    <view v-if="showTip && !disabled" class="publish-tip">
      <text class="tip-text">点击发布您的作品</text>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'PublishButton',
  components: {
    Icon
  },
  
  props: {
    // 是否禁用（画布为空时）
    disabled: {
      type: Boolean,
      default: false
    },
    
    // 是否显示加载状态
    loading: {
      type: Boolean,
      default: false
    },
    
    // 自定义按钮文字
    text: {
      type: String,
      default: '发布项目'
    },
    
    // 是否显示提示
    showTip: {
      type: Boolean,
      default: false
    },
    
    // 按钮大小
    size: {
      type: String,
      default: 'normal', // normal, large, small
      validator: (value) => ['normal', 'large', 'small'].includes(value)
    },
    
    // 按钮类型
    type: {
      type: String,
      default: 'primary', // primary, secondary
      validator: (value) => ['primary', 'secondary'].includes(value)
    }
  },
  
  computed: {
    buttonText() {
      if (this.loading) {
        return '发布中...'
      }
      if (this.disabled) {
        return '画布为空'
      }
      return this.text
    }
  },
  
  methods: {
    handleClick() {
      if (this.disabled || this.loading) {
        return
      }
      
      // 触觉反馈
      uni.vibrateShort({
        type: 'light'
      })
      
      // 发出点击事件
      this.$emit('click')
    }
  }
}
</script>

<style scoped>
.publish-button {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx 32rpx;
  background: linear-gradient(135deg, var(--color-brand-primary) 0%, var(--color-brand-accent) 100%);
  border-radius: 16rpx;
  box-shadow: 0 8rpx 24rpx rgba(0, 243, 255, 0.3);
  transition: all 0.3s ease;
  cursor: pointer;
  user-select: none;
}

.publish-button:active {
  transform: scale(0.95);
  box-shadow: 0 4rpx 12rpx rgba(0, 243, 255, 0.2);
}

.publish-button.disabled {
  background: var(--color-text-disabled);
  box-shadow: none;
  cursor: not-allowed;
  opacity: 0.6;
}

.publish-button.disabled:active {
  transform: none;
}

.publish-button.loading {
  cursor: wait;
}

.publish-button.loading:active {
  transform: none;
}

.button-content {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.button-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  color: #000000;
}

.publish-button.disabled .button-icon {
  color: var(--color-text-tertiary);
}

.loading-spinner {
  animation: spin 1s linear infinite;
}

@keyframes spin {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.button-text {
  font-size: 28rpx;
  font-weight: 600;
  color: #000000;
  letter-spacing: 1rpx;
}

.publish-button.disabled .button-text {
  color: var(--color-text-tertiary);
}

.publish-tip {
  position: absolute;
  top: -48rpx;
  left: 50%;
  transform: translateX(-50%);
  background-color: rgba(0, 0, 0, 0.8);
  padding: 8rpx 16rpx;
  border-radius: 8rpx;
  white-space: nowrap;
  opacity: 0;
  animation: tipFadeIn 0.3s ease forwards;
}

.publish-tip::after {
  content: '';
  position: absolute;
  top: 100%;
  left: 50%;
  transform: translateX(-50%);
  width: 0;
  height: 0;
  border-left: 8rpx solid transparent;
  border-right: 8rpx solid transparent;
  border-top: 8rpx solid rgba(0, 0, 0, 0.8);
}

.tip-text {
  font-size: 20rpx;
  color: #FFFFFF;
}

@keyframes tipFadeIn {
  from {
    opacity: 0;
    transform: translateX(-50%) translateY(-8rpx);
  }
  to {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
  }
}

/* 大小变体 */
.publish-button.large {
  padding: 32rpx 48rpx;
}

.publish-button.large .button-icon {
  width: 40rpx;
  height: 40rpx;
}

.publish-button.large .button-text {
  font-size: 32rpx;
}

.publish-button.small {
  padding: 16rpx 24rpx;
}

.publish-button.small .button-icon {
  width: 24rpx;
  height: 24rpx;
}

.publish-button.small .button-text {
  font-size: 24rpx;
}

/* 类型变体 */
.publish-button.secondary {
  background: var(--color-card-background);
  border: 2rpx solid var(--color-brand-primary);
  box-shadow: 0 4rpx 12rpx rgba(0, 243, 255, 0.1);
}

.publish-button.secondary .button-icon,
.publish-button.secondary .button-text {
  color: var(--color-brand-primary);
}

.publish-button.secondary:active {
  background-color: rgba(0, 243, 255, 0.1);
}

/* 响应式适配 */
@media (max-width: 750rpx) {
  .publish-button {
    padding: 20rpx 28rpx;
  }
  
  .button-text {
    font-size: 26rpx;
  }
}

/* 深色模式适配 */
@media (prefers-color-scheme: dark) {
  .publish-tip {
    background-color: rgba(255, 255, 255, 0.9);
  }
  
  .publish-tip::after {
    border-top-color: rgba(255, 255, 255, 0.9);
  }
  
  .tip-text {
    color: #000000;
  }
}

/* 无障碍支持 */
.publish-button:focus {
  outline: 4rpx solid rgba(0, 243, 255, 0.5);
  outline-offset: 4rpx;
}

.publish-button.disabled:focus {
  outline: 4rpx solid rgba(136, 136, 136, 0.5);
}
</style>
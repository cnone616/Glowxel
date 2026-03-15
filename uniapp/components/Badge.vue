<template>
  <view class="badge" :class="[`badge-${variant}`, `badge-${size}`]">
    <text class="badge-text">{{ text }}</text>
  </view>
</template>

<script>
export default {
  name: 'Badge',
  props: {
    // 徽章文本
    text: {
      type: [String, Number],
      required: true
    },
    // 变体：primary, secondary, success, warning, error
    variant: {
      type: String,
      default: 'primary',
      validator: (value) => ['primary', 'secondary', 'success', 'warning', 'error'].includes(value)
    },
    // 尺寸：small, medium, large
    size: {
      type: String,
      default: 'medium',
      validator: (value) => ['small', 'medium', 'large'].includes(value)
    }
  }
}
</script>

<style scoped>
.badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-small);
  font-weight: 500;
  white-space: nowrap;
}

/* 尺寸变体 */
.badge-small {
  padding: 4rpx 12rpx;
  font-size: 20rpx;
  line-height: 1.2;
}

.badge-medium {
  padding: 8rpx 16rpx;
  font-size: 24rpx;
  line-height: 1.3;
}

.badge-large {
  padding: 12rpx 20rpx;
  font-size: 28rpx;
  line-height: 1.4;
}

.badge-text {
  color: inherit;
}

/* 颜色变体 */
.badge-primary {
  background-color: var(--color-brand-primary);
  color: #FFFFFF;
}

.badge-secondary {
  background-color: var(--color-app-background);
  color: var(--color-text-secondary);
  border: 1rpx solid var(--border-primary);
}

.badge-success {
  background-color: var(--color-success);
  color: #FFFFFF;
}

.badge-warning {
  background-color: var(--color-warning);
  color: #FFFFFF;
}

.badge-error {
  background-color: var(--color-error);
  color: #FFFFFF;
}

/* 数字徽章特殊样式 */
.badge[data-number="true"] {
  min-width: 40rpx;
  border-radius: 20rpx;
}

.badge-small[data-number="true"] {
  min-width: 32rpx;
  border-radius: 16rpx;
}

.badge-large[data-number="true"] {
  min-width: 48rpx;
  border-radius: 24rpx;
}
</style>
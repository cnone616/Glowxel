<template>
  <view class="avatar" :class="[`avatar-${size}`, { 'avatar-with-badge': showBadge }]">
    <!-- 用户头像 -->
    <image 
      v-if="src" 
      :src="src" 
      class="avatar-image"
      mode="aspectFill"
      @error="handleImageError"
    />
    <!-- 默认头像 -->
    <view v-else class="avatar-placeholder">
      <Icon name="user" :size="iconSize" color="var(--text-tertiary)" />
    </view>
    
    <!-- 状态徽章 -->
    <view v-if="showBadge" class="avatar-badge" :class="`badge-${status}`">
      <view class="badge-dot"></view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'Avatar',
  components: {
    Icon
  },
  props: {
    // 头像图片地址
    src: {
      type: String,
      default: ''
    },
    // 尺寸：small, medium, large
    size: {
      type: String,
      default: 'medium',
      validator: (value) => ['small', 'medium', 'large'].includes(value)
    },
    // 是否显示状态徽章
    showBadge: {
      type: Boolean,
      default: false
    },
    // 状态：online, offline, busy
    status: {
      type: String,
      default: 'offline',
      validator: (value) => ['online', 'offline', 'busy'].includes(value)
    }
  },
  
  computed: {
    iconSize() {
      const sizeMap = {
        small: 32,
        medium: 48,
        large: 64
      }
      return sizeMap[this.size]
    }
  },
  
  methods: {
    handleImageError() {
      // 图片加载失败时的处理
      this.$emit('error')
    }
  }
}
</script>

<style scoped>
.avatar {
  position: relative;
  display: inline-block;
  border-radius: 50%;
  overflow: hidden;
  background-color: var(--nb-surface);
  border: 2rpx solid var(--nb-ink);
}

/* 尺寸变体 */
.avatar-small {
  width: 64rpx;
  height: 64rpx;
}

.avatar-medium {
  width: 96rpx;
  height: 96rpx;
}

.avatar-large {
  width: 128rpx;
  height: 128rpx;
}

.avatar-image {
  width: 100%;
  height: 100%;
}

.avatar-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-paper);
}

/* 状态徽章 */
.avatar-badge {
  position: absolute;
  bottom: 0;
  right: 0;
  width: 20rpx;
  height: 20rpx;
  border-radius: 50%;
  border: 2rpx solid var(--nb-surface);
  display: flex;
  align-items: center;
  justify-content: center;
}

.avatar-small .avatar-badge {
  width: 16rpx;
  height: 16rpx;
}

.avatar-large .avatar-badge {
  width: 24rpx;
  height: 24rpx;
}

.badge-dot {
  width: 12rpx;
  height: 12rpx;
  border-radius: 50%;
}

.avatar-small .badge-dot {
  width: 8rpx;
  height: 8rpx;
}

.avatar-large .badge-dot {
  width: 16rpx;
  height: 16rpx;
}

/* 状态颜色 */
.badge-online .badge-dot {
  background-color: var(--nb-green);
}

.badge-offline .badge-dot {
  background-color: #777777;
}

.badge-busy .badge-dot {
  background-color: var(--nb-orange);
}
</style>
<template>
  <view class="project-card">
    <!-- 缩略图区域 -->
    <view class="thumbnail-area" @click="handleClick">
      <!-- 像素网格背景 -->
      <view class="grid-pattern"></view>
      
      <!-- 缩略图 -->
      <view class="thumbnail-wrapper">
        <image 
          v-if="project.thumbnail && project.thumbnail.startsWith('data:image')"
          :src="project.thumbnail"
          class="thumbnail-image"
          mode="aspectFit"
          @error="handleImageError"
        />
        <view v-else class="thumbnail-placeholder">
          <Icon name="picture" :size="48" color="rgba(255,255,255,0.3)" />
        </view>
      </view>
      
      <!-- 重新生成按钮 
      <view class="regenerate-btn" @click.stop="handleRegenerate">
        <Icon name="refresh" :size="28" />
      </view>
      -->
      
      <!-- 进度条 -->
      <view class="progress-bar">
        <view 
          class="progress-fill" 
          :style="{ width: project.progress + '%' }"
        ></view>
      </view>
    </view>
    
    <!-- 信息区域 -->
    <view class="info-area" @click="handleClick">
      <text class="project-name">{{ project.name }}</text>
      <view class="project-meta">
        <view class="meta-item">
          <Icon name="column-4" :size="20" />
          <text class="meta-text">{{ project.width }}×{{ project.height }}</text>
        </view>
        <view v-if="project.paddedWidth && (project.paddedWidth !== project.width || project.paddedHeight !== project.height)" class="meta-item">
          <text class="meta-text hint">({{ project.paddedWidth }}×{{ project.paddedHeight }})</text>
        </view>
        <view class="meta-item">
          <text class="meta-text">{{ formatDate(project.lastEdited) }}</text>
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
    project: {
      type: Object,
      required: true
    }
  },
  
  methods: {
    handleClick() {
      uni.navigateTo({
        url: `/pages/overview/overview?id=${this.project.id}`
      })
    },
    
    handleImageError(e) {
      console.log('缩略图加载失败:', e)
      // 图片加载失败时，清除无效的缩略图
      this.$emit('image-error', this.project.id)
    },
    
    handleRegenerate() {
      // 重新生成缩略图功能暂时移除
      console.log('重新生成缩略图功能暂时不可用')
    },
    
    formatDate(date) {
      const d = new Date(date)
      return `${d.getMonth() + 1}/${d.getDate()}`
    }
  }
}
</script>

<style scoped>
.project-card {
  display: flex;
  flex-direction: column;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  overflow: hidden;
  border: 2rpx solid var(--border-primary);
  box-shadow: var(--shadow-md);
  transition: var(--transition-base);
  height: 100%;
}

.project-card:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
  box-shadow: 0 0 30rpx rgba(0, 243, 255, 0.2);
}

/* 缩略图区域 */
.thumbnail-area {
  position: relative;
  width: 100%;
  padding-bottom: 100%;
  background-color: var(--bg-primary);
  overflow: hidden;
}

.grid-pattern {
  position: absolute;
  inset: 0;
  opacity: 0.1;
  background-image: 
    linear-gradient(var(--canvas-grid) 2rpx, transparent 2rpx),
    linear-gradient(90deg, var(--canvas-grid) 2rpx, transparent 2rpx);
  background-size: 16rpx 16rpx;
}

.thumbnail-wrapper {
  position: absolute;
  inset: 0;
  padding: 16rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.thumbnail-image {
  width: 100%;
  height: 100%;
  display: block;
  object-fit: contain;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}

.thumbnail-placeholder {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(0, 0, 0, 0.1);
}

.regenerate-btn {
  position: absolute;
  top: 16rpx;
  right: 16rpx;
  width: 56rpx;
  height: 56rpx;
  background-color: rgba(0, 0, 0, 0.6);
  backdrop-filter: blur(20rpx);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ffffff;
  opacity: 0.8;
  transition: var(--transition-base);
  z-index: 10;
}

.regenerate-btn:active {
  opacity: 1;
  transform: scale(0.95);
  background-color: var(--accent-primary);
}

.progress-bar {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  height: 6rpx;
  background-color: rgba(0, 0, 0, 0.3);
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, var(--accent-primary), var(--accent-secondary));
  transition: width 0.3s ease;
}

/* 信息区域 */
.info-area {
  padding: 24rpx;
  background-color: var(--bg-tertiary);
}

.project-name {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  margin-bottom: 12rpx;
}

.project-meta {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
  color: var(--text-secondary);
}

.meta-text {
  font-size: 22rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.meta-text.hint {
  opacity: 0.6;
  font-size: 20rpx;
}
</style>

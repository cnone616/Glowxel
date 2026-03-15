<template>
  <view class="template-card" @click="handleClick" v-if="template && typeof template === 'object'">
    <!-- 模板预览图 -->
    <view class="template-image-container">
      <image 
        :src="template.image" 
        class="template-image"
        mode="aspectFill"
        @error="handleImageError"
      />
      <!-- 图片加载失败占位 -->
      <view v-if="imageError" class="image-placeholder">
        <Icon name="picture" :size="48" color="#AAAAAA" />
      </view>
      
      <!-- 难度标签 -->
      <view class="difficulty-badge" :class="getDifficultyClass(template.difficulty)">
        <text class="difficulty-text">{{ template.difficulty }}</text>
      </view>
    </view>
    
    <!-- 模板信息 -->
    <view class="template-info">
      <!-- 模板名称 -->
      <text class="template-name">{{ template.name }}</text>
      
      <!-- 模板详情 -->
      <view class="template-details">
        <view class="detail-item">
          <Icon name="adjust" :size="24" color="#666666" />
          <text class="detail-text">{{ template.size }}</text>
        </view>
        
        <view class="detail-item">
          <Icon name="picture" :size="24" color="#666666" />
          <text class="detail-text">{{ template.colorCount }}色</text>
        </view>
      </view>
      
      <!-- 使用统计 -->
      <view class="template-stats">
        <Icon name="user" :size="24" color="#4F7FFF" />
        <text class="usage-count">{{ formatUsageCount(template.usageCount) }}人使用</text>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'

export default {
  name: 'TemplateCard',
  components: {
    Icon
  },
  props: {
    // 模板数据
    template: {
      type: Object,
      required: true
    }
  },
  
  data() {
    return {
      imageError: false
    }
  },
  
  methods: {
    handleClick() {
      this.$emit('click', this.template)
    },
    
    handleImageError() {
      this.imageError = true
    },
    
    getDifficultyClass(difficulty) {
      const difficultyMap = {
        '简单': 'difficulty-easy',
        '中等': 'difficulty-medium', 
        '困难': 'difficulty-hard'
      }
      return difficultyMap[difficulty] || 'difficulty-easy'
    },
    
    formatUsageCount(count) {
      if (count === undefined || count === null) return '0'
      if (count >= 10000) {
        return (count / 10000).toFixed(1) + '万'
      } else if (count >= 1000) {
        return (count / 1000).toFixed(1) + 'k'
      }
      return count.toString()
    }
  }
}
</script>

<style scoped>
.template-card {
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  box-shadow: var(--shadow-card);
  overflow: hidden;
  transition: all 0.2s ease;
}

.template-card:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.template-image-container {
  position: relative;
  width: 100%;
  height: 240rpx;
  background-color: var(--color-app-background);
}

.template-image {
  width: 100%;
  height: 100%;
}

.image-placeholder {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
}

.difficulty-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  padding: 6rpx 12rpx;
  border-radius: var(--radius-small);
  backdrop-filter: blur(10rpx);
}

.difficulty-easy {
  background-color: rgba(46, 204, 113, 0.9);
}

.difficulty-medium {
  background-color: rgba(241, 156, 18, 0.9);
}

.difficulty-hard {
  background-color: rgba(231, 76, 60, 0.9);
}

.difficulty-text {
  font-size: 20rpx;
  color: #FFFFFF;
  font-weight: 500;
}

.template-info {
  padding: 20rpx;
}

.template-name {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  line-height: 1.4;
  margin-bottom: 12rpx;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 1;
  overflow: hidden;
}

.template-details {
  display: flex;
  align-items: center;
  gap: 24rpx;
  margin-bottom: 12rpx;
}

.detail-item {
  display: flex;
  align-items: center;
  gap: 6rpx;
}

.detail-text {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.template-stats {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.usage-count {
  font-size: 22rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}
</style>
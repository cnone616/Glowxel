<template>
  <view class="template-card glx-panel-card" @click="handleClick" v-if="template && typeof template === 'object'">
    <!-- 模板预览图 -->
    <view class="template-image-container">
      <image 
        :src="template.image_url" 
        class="template-image"
        mode="aspectFill"
        @error="handleImageError"
      />
      <!-- 图片加载失败占位 -->
      <view v-if="imageError" class="image-placeholder">
        <Icon name="picture" :size="48" color="var(--text-tertiary)" />
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
          <Icon name="adjust" :size="24" color="currentColor" />
          <text class="detail-text">{{ template.size }}</text>
        </view>
        
        <view class="detail-item">
          <Icon name="picture" :size="24" color="currentColor" />
          <text class="detail-text">{{ template.color_count }}色</text>
        </view>
      </view>
      
      <!-- 使用统计 -->
      <view class="template-stats">
        <Icon name="user" :size="24" color="currentColor" />
        <text class="usage-count">{{ formatUsageCount(template.usage_count) }}人使用</text>
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
      if (typeof count !== 'number') return '0'
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
  background-color: var(--nb-surface);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
  transition: all 0.2s ease;
}


.template-image-container {
  position: relative;
  width: 100%;
  height: 240rpx;
  background-color: var(--nb-paper);
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
  background-color: var(--nb-paper);
}

.difficulty-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  padding: 6rpx 12rpx;
  border: var(--nb-border-width-control) solid var(--nb-ink);
  border-radius: 0;
  box-shadow: var(--nb-shadow-soft);
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
  color: var(--nb-ink);
  font-weight: 900;
}

.template-info {
  padding: 20rpx;
}

.template-name {
  font-size: 26rpx;
  font-weight: 900;
  color: var(--nb-ink);
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
  color: #4a4a4a;
}

.detail-text {
  font-size: 22rpx;
  color: currentColor;
}

.template-stats {
  display: flex;
  align-items: center;
  gap: 8rpx;
  color: var(--nb-ink);
}

.usage-count {
  font-size: 22rpx;
  color: currentColor;
  font-weight: 800;
}
</style>

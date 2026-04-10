<template>
  <view class="artwork-card glx-panel-card" @click="handleClick" v-if="artwork && typeof artwork === 'object'">
    <!-- 作品图片 -->
    <view class="artwork-image-container">
      <image 
        :src="artwork.cover_url" 
        class="artwork-image"
        mode="aspectFill"
        @error="handleImageError"
      />
      <!-- 图片加载失败占位 -->
      <view v-if="imageError" class="image-placeholder">
        <Icon name="picture" :size="64" color="#AAAAAA" />
      </view>
    </view>
    
    <!-- 作品信息 -->
    <view class="artwork-info">
      <!-- 作品标题 -->
      <text class="artwork-title">{{ artwork.title }}</text>
      
      <!-- 创作者信息 -->
      <view class="author-info">
        <Avatar 
          :src="artwork.author_avatar" 
          size="small"
          @error="handleAvatarError"
        />
        <text class="author-name">{{ artwork.author_name }}</text>
      </view>
      
      <!-- 互动数据 -->
      <view class="artwork-stats">
        <view class="stat-item" @click.stop="handleLike">
          <Icon 
            :name="isLiked ? 'favorite-filling' : 'favorite'" 
            :size="32" 
            :color="isLiked ? '#E74C3C' : '#666666'" 
          />
          <text class="stat-text">{{ formatNumber(artwork.likes) }}</text>
        </view>
        
        <view class="stat-item" @click.stop="handleComment">
          <Icon name="comment" :size="32" color="#666666" />
          <text class="stat-text">{{ formatNumber(artwork.comments_count) }}</text>
        </view>
        
        <view class="stat-item">
          <Icon name="browse" :size="32" color="#666666" />
          <text class="stat-text">{{ formatNumber(artwork.views) }}</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'
import Avatar from './Avatar.vue'

export default {
  name: 'ArtworkCard',
  components: {
    Icon,
    Avatar
  },
  props: {
    // 作品数据
    artwork: {
      type: Object,
      required: true
    },
    // 是否已点赞
    liked: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      imageError: false,
      isLiked: this.liked
    }
  },
  
  methods: {
    handleClick() {
      this.$emit('click', this.artwork)
    },
    
    handleLike() {
      this.isLiked = !this.isLiked
      this.$emit('like', {
        artwork: this.artwork,
        liked: this.isLiked
      })
    },
    
    handleComment() {
      this.$emit('comment', this.artwork)
    },
    
    handleImageError() {
      this.imageError = true
    },
    
    handleAvatarError() {
      // 头像加载失败处理
    },
    
    formatNumber(num) {
      if (typeof num !== 'number') {
        return '0'
      }
      if (num >= 1000) {
        return (num / 1000).toFixed(1) + 'k'
      }
      return num.toString()
    }
  }
}
</script>

<style scoped>
.artwork-card {
  background-color: var(--nb-surface);
  border-radius: 0;
  box-shadow: var(--nb-shadow-strong);
  overflow: hidden;
  transition: all 0.2s ease;
}

.artwork-card:active {
  transform: translate(4rpx, 4rpx);
  box-shadow: 2rpx 2rpx 0 var(--nb-ink);
}

.artwork-image-container {
  position: relative;
  width: 100%;
  height: 320rpx;
  background-color: var(--nb-paper);
}

.artwork-image {
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

.artwork-info {
  padding: 24rpx;
}

.artwork-title {
  font-size: 28rpx;
  font-weight: 900;
  color: var(--nb-ink);
  line-height: 1.4;
  margin-bottom: 16rpx;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 2;
  overflow: hidden;
}

.author-info {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-bottom: 20rpx;
}

.author-name {
  font-size: 24rpx;
  color: #4a4a4a;
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.artwork-stats {
  display: flex;
  align-items: center;
  gap: 32rpx;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
  transition: all 0.2s ease;
}

.stat-item:active {
  transform: scale(0.95);
}

.stat-text {
  font-size: 22rpx;
  color: #4a4a4a;
  font-weight: 800;
}

/* 点赞动画 */
.stat-item:first-child:active {
  transform: translate(2rpx, 2rpx);
}
</style>

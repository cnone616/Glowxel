<template>
  <view class="comment">
    <!-- 用户头像 -->
    <view class="comment-avatar">
      <Avatar 
        :src="comment.author.avatar" 
        :size="60"
      />
    </view>
    
    <!-- 评论内容 -->
    <view class="comment-content">
      <!-- 用户信息 -->
      <view class="comment-header">
        <text class="username">{{ comment.author.name }}</text>
        <Badge 
          v-if="comment.author.level" 
          :text="`Lv.${comment.author.level}`" 
          variant="secondary" 
          size="small"
        />
        <text class="comment-time">{{ formatTime(comment.createdAt) }}</text>
      </view>
      
      <!-- 评论文本 -->
      <text class="comment-text">{{ comment.content }}</text>
      
      <!-- 评论图片 -->
      <view v-if="comment.images && comment.images.length > 0" class="comment-images">
        <image 
          v-for="(image, index) in comment.images" 
          :key="index"
          :src="image" 
          class="comment-image"
          mode="aspectFill"
          @click="handleImageClick(image, index)"
        />
      </view>
      
      <!-- 回复内容 -->
      <view v-if="comment.replyTo" class="reply-content">
        <text class="reply-prefix">回复 </text>
        <text class="reply-username">@{{ comment.replyTo.author.name }}</text>
        <text class="reply-text">: {{ comment.replyTo.content }}</text>
      </view>
      
      <!-- 评论操作 -->
      <view class="comment-actions">
        <view class="action-item" @click="handleLike">
          <Icon 
            :name="isLiked ? 'favorite-filling' : 'favorite'" 
            :size="28" 
            :color="isLiked ? '#E74C3C' : '#666666'" 
          />
          <text class="action-text" :class="{ 'action-active': isLiked }">
            {{ (comment.likes || 0) > 0 ? comment.likes : '点赞' }}
          </text>
        </view>
        
        <view class="action-item" @click="handleReply">
          <Icon name="comment" :size="28" color="#666666" />
          <text class="action-text">回复</text>
        </view>
        
        <view v-if="showMore" class="action-item" @click="handleMore">
          <Icon name="more" :size="28" color="#666666" />
        </view>
      </view>
      
      <!-- 子评论 -->
      <view v-if="comment.replies && comment.replies.length > 0" class="sub-comments">
        <Comment 
          v-for="reply in visibleReplies" 
          :key="reply.id"
          :comment="reply"
          :isSubComment="true"
          @like="handleSubLike"
          @reply="handleSubReply"
          @imageClick="handleSubImageClick"
        />
        
        <!-- 展开更多回复 -->
        <view 
          v-if="comment.replies.length > visibleRepliesCount" 
          class="expand-replies"
          @click="expandReplies"
        >
          <text class="expand-text">
            展开更多回复 ({{ comment.replies.length - visibleRepliesCount }})
          </text>
          <Icon name="arrow-down" :size="24" color="#4F7FFF" />
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import Icon from './Icon.vue'
import Avatar from './Avatar.vue'
import Badge from './Badge.vue'

export default {
  name: 'Comment',
  components: {
    Icon,
    Avatar,
    Badge
  },
  props: {
    // 评论数据
    comment: {
      type: Object,
      required: true
    },
    // 是否为子评论
    isSubComment: {
      type: Boolean,
      default: false
    },
    // 是否显示更多操作
    showMore: {
      type: Boolean,
      default: true
    },
    // 是否已点赞
    liked: {
      type: Boolean,
      default: false
    }
  },
  
  data() {
    return {
      isLiked: this.liked,
      visibleRepliesCount: 3,
      showAllReplies: false
    }
  },
  
  computed: {
    visibleReplies() {
      if (!this.comment.replies) return []
      
      if (this.showAllReplies) {
        return this.comment.replies
      }
      
      return this.comment.replies.slice(0, this.visibleRepliesCount)
    }
  },
  
  methods: {
    handleLike() {
      this.isLiked = !this.isLiked
      this.$emit('like', {
        comment: this.comment,
        liked: this.isLiked
      })
    },
    
    handleReply() {
      this.$emit('reply', this.comment)
    },
    
    handleMore() {
      this.$emit('more', this.comment)
    },
    
    handleImageClick(image, index) {
      this.$emit('imageClick', {
        image,
        index,
        images: this.comment.images
      })
    },
    
    handleSubLike(data) {
      this.$emit('like', data)
    },
    
    handleSubReply(comment) {
      this.$emit('reply', comment)
    },
    
    handleSubImageClick(data) {
      this.$emit('imageClick', data)
    },
    
    expandReplies() {
      this.showAllReplies = true
    },
    
    formatTime(timestamp) {
      const now = new Date()
      const time = new Date(timestamp)
      const diff = now - time
      
      const minutes = Math.floor(diff / (1000 * 60))
      const hours = Math.floor(diff / (1000 * 60 * 60))
      const days = Math.floor(diff / (1000 * 60 * 60 * 24))
      
      if (minutes < 1) return '刚刚'
      if (minutes < 60) return `${minutes}分钟前`
      if (hours < 24) return `${hours}小时前`
      if (days < 7) return `${days}天前`
      
      return time.toLocaleDateString('zh-CN', {
        month: 'short',
        day: 'numeric'
      })
    }
  }
}
</script>

<style scoped>
.comment {
  display: flex;
  gap: 16rpx;
  padding: 20rpx 0;
}

.comment:not(:last-child) {
  border-bottom: 1rpx solid var(--border-primary);
}

.comment-avatar {
  flex-shrink: 0;
}

.comment-content {
  flex: 1;
  min-width: 0;
}

.comment-header {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-bottom: 8rpx;
  flex-wrap: wrap;
}

.username {
  font-size: 26rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.comment-time {
  font-size: 22rpx;
  color: var(--color-text-disabled);
  margin-left: auto;
}

.comment-text {
  font-size: 28rpx;
  color: var(--color-text-primary);
  line-height: 1.6;
  margin-bottom: 12rpx;
  word-wrap: break-word;
}

.comment-images {
  display: flex;
  gap: 12rpx;
  margin-bottom: 12rpx;
  flex-wrap: wrap;
}

.comment-image {
  width: 120rpx;
  height: 120rpx;
  border-radius: var(--radius-small);
  background-color: var(--color-app-background);
}

.reply-content {
  background-color: var(--color-app-background);
  padding: 12rpx 16rpx;
  border-radius: var(--radius-small);
  margin-bottom: 12rpx;
  border-left: 4rpx solid var(--color-brand-primary);
}

.reply-prefix {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.reply-username {
  font-size: 24rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

.reply-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
}

.comment-actions {
  display: flex;
  align-items: center;
  gap: 32rpx;
  margin-top: 12rpx;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 8rpx 12rpx;
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
}

.action-item:active {
  background-color: var(--color-app-background);
  transform: scale(0.95);
}

.action-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

.action-active {
  color: var(--color-error);
}

.sub-comments {
  margin-top: 20rpx;
  padding-left: 20rpx;
  border-left: 2rpx solid var(--border-primary);
}

.expand-replies {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 0;
  cursor: pointer;
}

.expand-replies:active {
  opacity: 0.7;
}

.expand-text {
  font-size: 24rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

/* 子评论样式调整 */
.comment .comment {
  padding: 16rpx 0;
}

.comment .comment .comment-avatar {
  transform: scale(0.9);
}

.comment .comment .comment-text {
  font-size: 26rpx;
}

.comment .comment .comment-actions {
  margin-top: 8rpx;
}

/* 点赞动画 */
.action-item:first-child:active {
  animation: likeAnimation 0.3s ease;
}

@keyframes likeAnimation {
  0% { transform: scale(0.95); }
  50% { transform: scale(1.1); }
  100% { transform: scale(0.95); }
}
</style>
<template>
  <view class="artwork-detail-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="40" color="#FFFFFF" />
        </view>
        <text class="header-title">作品详情</text>
        <view class="more-btn" @click="showMoreActions">
          <Icon name="more-horizontal" :size="40" color="#FFFFFF" />
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view scroll-y class="main-content">
      <!-- 作品展示区域 -->
      <view class="artwork-section">
        <view class="artwork-container">
          <image 
            :src="artwork.coverUrl" 
            class="artwork-image"
            mode="aspectFit"
            @click="previewImage"
          />
          <view class="zoom-hint">
            <Icon name="zoom-in" :size="32" color="#FFFFFF" />
            <text class="zoom-text">点击查看大图</text>
          </view>
        </view>
      </view>
      
      <!-- 作品信息 -->
      <view class="info-section">
        <view class="artwork-info">
          <text class="artwork-title">{{ artwork.title }}</text>
          <view class="artwork-meta">
            <view class="meta-item">
              <Icon name="modular" :size="28" color="#666666" />
              <text class="meta-text">{{ artwork.width }}×{{ artwork.height }}</text>
            </view>
            <view class="meta-item">
              <Icon name="picture" :size="28" color="#666666" />
              <text class="meta-text">{{ artwork.colorCount }}色</text>
            </view>
            <view class="meta-item">
              <Icon name="calendar" :size="28" color="#666666" />
              <text class="meta-text">{{ formatDate(artwork.createdAt) }}</text>
            </view>
          </view>
          
          <!-- 标签 -->
          <view v-if="artwork.tags && artwork.tags.length" class="tags-container">
            <view 
              v-for="tag in artwork.tags" 
              :key="tag" 
              class="tag-item"
            >
              <text class="tag-text">#{{ tag }}</text>
            </view>
          </view>
        </view>
        
        <!-- 作者信息 -->
        <view class="author-section">
          <view class="author-info" @click="goToUserProfile">
            <Avatar :src="artwork.author.avatar" :size="80" />
            <view class="author-details">
              <text class="author-name">{{ artwork.author.name }}</text>
              <text class="author-stats">{{ artwork.author.worksCount }}个作品</text>
            </view>
          </view>
          <view 
            class="follow-btn"
            :class="{ 'following': artwork.author.isFollowing }"
            @click="toggleFollow"
          >
            <text class="follow-text">
              {{ artwork.author.isFollowing ? '已关注' : '关注' }}
            </text>
          </view>
        </view>
      </view>
      
      <!-- 交互按钮 -->
      <view class="action-section">
        <view class="action-buttons">
          <view 
            class="action-btn"
            :class="{ 'active': isLiked }"
            @click="toggleLike"
          >
            <Icon :name="isLiked ? 'favorite-filling' : 'favorite'" :size="40" :color="isLiked ? '#FF4757' : '#666666'" />
            <text class="action-text">{{ artwork.likes || 0 }}</text>
          </view>
          
          <view 
            class="action-btn"
            :class="{ 'active': isCollected }"
            @click="toggleCollect"
          >
            <Icon :name="isCollected ? 'file-common-filling' : 'file-common'" :size="40" :color="isCollected ? '#4F7FFF' : '#666666'" />
            <text class="action-text">{{ artwork.collects || 0 }}</text>
          </view>
          
          <view class="action-btn" @click="showCommentInput">
            <Icon name="comment" :size="40" color="#666666" />
            <text class="action-text">{{ artwork.comments || 0 }}</text>
          </view>
          
          <view class="action-btn" @click="shareArtwork">
            <Icon name="share" :size="40" color="#666666" />
            <text class="action-text">分享</text>
          </view>
        </view>
      </view>
      
      <!-- 评论区域 -->
      <view class="comment-section">
        <view class="section-header">
          <text class="section-title">评论 ({{ comments.length }})</text>
        </view>
        
        <!-- 评论列表 -->
        <view v-if="comments.length > 0" class="comment-list">
          <Comment 
            v-for="comment in comments"
            :key="comment.id"
            :comment="comment"
            @reply="handleReply"
            @like="handleCommentLike"
            @delete="handleCommentDelete"
          />
        </view>
        
        <!-- 空状态 -->
        <view v-else class="empty-comments">
          <Icon name="comment" :size="80" color="#CCCCCC" />
          <text class="empty-text">还没有评论，快来抢沙发吧！</text>
        </view>
      </view>
      
      <!-- 相关推荐 -->
      <view class="related-section">
        <view class="section-header">
          <text class="section-title">相关推荐</text>
        </view>
        <view class="related-grid">
          <ArtworkCard 
            v-for="item in relatedArtworks"
            :key="item.id"
            :artwork="item"
            @click="goToArtwork"
          />
        </view>
      </view>
    </scroll-view>
    
    <!-- 底部评论输入 -->
    <view v-if="showCommentBar" class="comment-input-bar">
      <view class="comment-input-container">
        <Input
          v-model="commentText"
          type="text"
          placeholder="写下你的评论..."
          :focus="commentInputFocus"
          clearable
          @blur="handleCommentBlur"
        />
        <view 
          class="send-btn"
          :class="{ 'active': commentText.trim() }"
          @click="sendComment"
        >
          <text class="send-text">发送</text>
        </view>
      </view>
    </view>
    
    <!-- 更多操作弹窗 -->
    <Modal v-model:visible="showMoreModal" title="更多操作">
      <view class="more-actions">
        <view class="action-item" @click="downloadArtwork">
          <Icon name="download" :size="40" color="#4F7FFF" />
          <text class="action-label">下载作品</text>
        </view>
        <view class="action-item" @click="useAsTemplate">
          <Icon name="copy" :size="40" color="#4F7FFF" />
          <text class="action-label">用作模板</text>
        </view>
        <view class="action-item" @click="reportArtwork">
          <Icon name="warning" :size="40" color="#FF4757" />
          <text class="action-label">举报作品</text>
        </view>
      </view>
    </Modal>
  </view>
</template>

<script>
import { MockAPI } from '../../data/mock/index.js'
import { useProjectStore } from '../../store/project.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Avatar from '../../components/Avatar.vue'
import Comment from '../../components/Comment.vue'
import ArtworkCard from '../../components/ArtworkCard.vue'
import Modal from '../../components/Modal.vue'
import Input from '../../components/Input.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    Comment,
    ArtworkCard,
    Modal,
    Input
  },
  
  data() {
    return {
      artworkId: '',
      artwork: {},
      comments: [],
      relatedArtworks: [],
      
      // 交互状态
      isLiked: false,
      isCollected: false,
      
      // 评论相关
      showCommentBar: false,
      commentText: '',
      commentInputFocus: false,
      
      // 弹窗状态
      showMoreModal: false,
      
      // 加载状态
      isLoading: true
    }
  },
  
  onLoad(options) {
    this.artworkId = options.id
    if (this.artworkId) {
      this.loadArtworkDetail()
    }
  },
  
  methods: {
    async loadArtworkDetail() {
      try {
        this.isLoading = true
        
        // 获取作品详情
        this.artwork = await MockAPI.artworks.getById(this.artworkId)
        
        // 获取评论列表
        this.comments = await MockAPI.comments.getByArtworkId(this.artworkId)
        
        // 获取相关推荐
        this.relatedArtworks = await MockAPI.artworks.getRelated(this.artworkId)
        
        // 获取用户交互状态
        this.isLiked = await MockAPI.likes.isLiked(this.artworkId)
        this.isCollected = await MockAPI.collections.isCollected(this.artworkId)
        
      } catch (error) {
        console.error('加载作品详情失败:', error)
        uni.showToast({
          title: '加载失败',
          icon: 'error'
        })
      } finally {
        this.isLoading = false
      }
    },
    
    handleBack() {
      uni.navigateBack()
    },
    
    previewImage() {
      uni.previewImage({
        urls: [this.artwork.coverUrl],
        current: this.artwork.coverUrl
      })
    },
    
    goToUserProfile() {
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${this.artwork.author.id}`
      })
    },
    
    async toggleFollow() {
      try {
        const isFollowing = !this.artwork.author.isFollowing
        
        // 调用关注API
        if (isFollowing) {
          await MockAPI.follows.follow(this.artwork.author.id)
        } else {
          await MockAPI.follows.unfollow(this.artwork.author.id)
        }
        
        this.artwork.author.isFollowing = isFollowing
        
        uni.showToast({
          title: isFollowing ? '关注成功' : '取消关注',
          icon: 'success'
        })
      } catch (error) {
        console.error('关注操作失败:', error)
        uni.showToast({
          title: '操作失败',
          icon: 'error'
        })
      }
    },
    
    async toggleLike() {
      try {
        const liked = !this.isLiked
        
        // 调用点赞API
        if (liked) {
          await MockAPI.likes.like(this.artworkId)
          this.artwork.likes = (this.artwork.likes || 0) + 1
        } else {
          await MockAPI.likes.unlike(this.artworkId)
          this.artwork.likes = Math.max(0, (this.artwork.likes || 0) - 1)
        }
        
        this.isLiked = liked
        
      } catch (error) {
        console.error('点赞操作失败:', error)
        uni.showToast({
          title: '操作失败',
          icon: 'error'
        })
      }
    },
    
    async toggleCollect() {
      try {
        const collected = !this.isCollected

        // 调用收藏API
        if (collected) {
          await MockAPI.collections.collect(this.artworkId)
          this.artwork.collects = (this.artwork.collects || 0) + 1

          // 收藏时创建只读本地项目
          this._addCollectedProject()
        } else {
          await MockAPI.collections.uncollect(this.artworkId)
          this.artwork.collects = Math.max(0, (this.artwork.collects || 0) - 1)

          // 取消收藏时删除本地项目
          this._removeCollectedProject()
        }

        this.isCollected = collected

        uni.showToast({
          title: collected ? '已收藏并添加到我的项目' : '取消收藏',
          icon: 'success'
        })
      } catch (error) {
        console.error('收藏操作失败:', error)
        uni.showToast({
          title: '操作失败',
          icon: 'error'
        })
      }
    },

    // 收藏时添加只读项目到本地
    _addCollectedProject() {
      if (!this.artwork) return
      const projectStore = useProjectStore()
      projectStore.addCollectedProject({
        name: this.artwork.title,
        width: this.artwork.width || 52,
        height: this.artwork.height || 52,
        palette: [],
        thumbnail: this.artwork.coverUrl || '',
        artworkId: this.artwork.id,
        originalAuthor: this.artwork.author ? {
          id: this.artwork.author.id,
          name: this.artwork.author.name,
          avatar: this.artwork.author.avatar
        } : null
      })
    },

    // 取消收藏时删除对应的本地项目
    _removeCollectedProject() {
      if (!this.artwork) return
      const projectStore = useProjectStore()
      const project = projectStore.projects.find(
        p => p.source === 'collected' && p.artworkId === this.artwork.id
      )
      if (project) {
        projectStore.deleteProject(project.id)
      }
    },
    
    showCommentInput() {
      this.showCommentBar = true
      this.commentInputFocus = true
    },
    
    handleCommentBlur() {
      if (!this.commentText.trim()) {
        this.showCommentBar = false
      }
    },
    
    async sendComment() {
      if (!this.commentText.trim()) return
      
      try {
        const newComment = await MockAPI.comments.add({
          artworkId: this.artworkId,
          content: this.commentText.trim()
        })
        
        this.comments.unshift(newComment)
        this.artwork.comments = (this.artwork.comments || 0) + 1
        this.commentText = ''
        this.showCommentBar = false
        
        uni.showToast({
          title: '评论成功',
          icon: 'success'
        })
      } catch (error) {
        console.error('发表评论失败:', error)
        uni.showToast({
          title: '评论失败',
          icon: 'error'
        })
      }
    },
    
    handleReply(comment) {
      this.commentText = `@${comment.author.name} `
      this.showCommentInput()
    },
    
    async handleCommentLike(comment) {
      try {
        const liked = !comment.isLiked
        
        if (liked) {
          await MockAPI.comments.like(comment.id)
          comment.likes = (comment.likes || 0) + 1
        } else {
          await MockAPI.comments.unlike(comment.id)
          comment.likes = Math.max(0, (comment.likes || 0) - 1)
        }
        
        comment.isLiked = liked
      } catch (error) {
        console.error('评论点赞失败:', error)
      }
    },
    
    async handleCommentDelete(comment) {
      try {
        await MockAPI.comments.delete(comment.id)
        
        const index = this.comments.findIndex(c => c.id === comment.id)
        if (index > -1) {
          this.comments.splice(index, 1)
          this.artwork.comments = Math.max(0, (this.artwork.comments || 0) - 1)
        }
        
        uni.showToast({
          title: '删除成功',
          icon: 'success'
        })
      } catch (error) {
        console.error('删除评论失败:', error)
        uni.showToast({
          title: '删除失败',
          icon: 'error'
        })
      }
    },
    
    shareArtwork() {
      // 微信小程序分享
      uni.showShareMenu({
        withShareTicket: true
      })
    },
    
    showMoreActions() {
      this.showMoreModal = true
    },
    
    downloadArtwork() {
      this.showMoreModal = false
      uni.showToast({
        title: '下载功能开发中',
        icon: 'none'
      })
    },
    
    useAsTemplate() {
      this.showMoreModal = false
      uni.navigateTo({
        url: `/pages/create/create?templateId=${this.artworkId}`
      })
    },
    
    reportArtwork() {
      this.showMoreModal = false
      uni.showToast({
        title: '举报功能开发中',
        icon: 'none'
      })
    },
    
    goToArtwork(artwork) {
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`
      })
    },
    
    formatDate(dateString) {
      const date = new Date(dateString)
      const now = new Date()
      const diff = now - date
      
      const minute = 60 * 1000
      const hour = 60 * minute
      const day = 24 * hour
      
      if (diff < hour) {
        return Math.floor(diff / minute) + '分钟前'
      } else if (diff < day) {
        return Math.floor(diff / hour) + '小时前'
      } else if (diff < 7 * day) {
        return Math.floor(diff / day) + '天前'
      } else {
        return date.toLocaleDateString()
      }
    }
  }
}
</script>

<style scoped>
.artwork-detail-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
}

/* 状态栏占位 */
.status-bar {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  flex-shrink: 0;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
}

.back-btn, .more-btn {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.back-btn:active, .more-btn:active {
  transform: scale(0.95);
  background-color: rgba(255, 255, 255, 0.1);
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: #FFFFFF;
  flex: 1;
  text-align: center;
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
}

/* 作品展示区域 */
.artwork-section {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 0 32rpx 48rpx;
}

.artwork-container {
  position: relative;
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-large);
  overflow: hidden;
}

.artwork-image {
  width: 100%;
  height: 600rpx;
  display: block;
}

.zoom-hint {
  position: absolute;
  bottom: 24rpx;
  right: 24rpx;
  display: flex;
  align-items: center;
  gap: 8rpx;
  background-color: rgba(0, 0, 0, 0.6);
  padding: 12rpx 16rpx;
  border-radius: var(--radius-small);
}

.zoom-text {
  font-size: 24rpx;
  color: #FFFFFF;
}

/* 信息区域 */
.info-section {
  background-color: var(--color-card-background);
  margin: -24rpx 32rpx 32rpx;
  border-radius: var(--radius-large);
  padding: 32rpx;
  box-shadow: var(--shadow-card);
}

.artwork-info {
  margin-bottom: 32rpx;
}

.artwork-title {
  font-size: 40rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  margin-bottom: 16rpx;
  display: block;
}

.artwork-meta {
  display: flex;
  gap: 24rpx;
  margin-bottom: 24rpx;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.meta-text {
  font-size: 26rpx;
  color: var(--color-text-secondary);
}

.tags-container {
  display: flex;
  flex-wrap: wrap;
  gap: 12rpx;
}

.tag-item {
  background-color: var(--color-app-background);
  padding: 8rpx 16rpx;
  border-radius: var(--radius-small);
  border: 2rpx solid var(--border-primary);
}

.tag-text {
  font-size: 24rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

/* 作者信息 */
.author-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding-top: 32rpx;
  border-top: 2rpx solid var(--border-primary);
}

.author-info {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.author-details {
  flex: 1;
}

.author-name {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  display: block;
  margin-bottom: 4rpx;
}

.author-stats {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.follow-btn {
  padding: 16rpx 32rpx;
  background-color: var(--color-brand-primary);
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.follow-btn.following {
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
}

.follow-btn:active {
  transform: scale(0.95);
}

.follow-text {
  font-size: 28rpx;
  font-weight: 500;
  color: #FFFFFF;
}

.follow-btn.following .follow-text {
  color: var(--color-text-secondary);
}

/* 交互按钮 */
.action-section {
  margin: 0 32rpx 32rpx;
}

.action-buttons {
  display: flex;
  background-color: var(--color-card-background);
  border-radius: var(--radius-large);
  padding: 24rpx;
  box-shadow: var(--shadow-card);
}

.action-btn {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx;
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.action-btn:active {
  transform: scale(0.95);
  background-color: var(--color-app-background);
}

.action-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-weight: 500;
}

.action-btn.active .action-text {
  color: var(--color-brand-primary);
  font-weight: 600;
}

/* 评论区域 */
.comment-section {
  margin: 0 32rpx 32rpx;
  background-color: var(--color-card-background);
  border-radius: var(--radius-large);
  padding: 32rpx;
  box-shadow: var(--shadow-card);
}

.section-header {
  margin-bottom: 24rpx;
}

.section-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.comment-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.empty-comments {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  text-align: center;
}

.empty-text {
  font-size: 28rpx;
  color: var(--color-text-disabled);
  margin-top: 16rpx;
}

/* 相关推荐 */
.related-section {
  margin: 0 32rpx 32rpx;
  background-color: var(--color-card-background);
  border-radius: var(--radius-large);
  padding: 32rpx;
  box-shadow: var(--shadow-card);
}

.related-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

/* 评论输入栏 */
.comment-input-bar {
  background-color: var(--color-card-background);
  border-top: 2rpx solid var(--border-primary);
  padding: 24rpx 32rpx;
  padding-bottom: calc(24rpx + env(safe-area-inset-bottom));
}

.comment-input-container {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.send-btn {
  padding: 12rpx 24rpx;
  background-color: var(--color-text-disabled);
  border-radius: var(--radius-small);
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.send-btn.active {
  background-color: var(--color-brand-primary);
}

.send-btn:active {
  transform: scale(0.95);
}

.send-text {
  font-size: 26rpx;
  color: #FFFFFF;
  font-weight: 500;
}

/* 更多操作弹窗 */
.more-actions {
  padding: 24rpx 0;
}

.action-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  padding: 24rpx 32rpx;
  transition: all 0.2s ease;
}

.action-item:active {
  background-color: var(--color-app-background);
}

.action-label {
  font-size: 32rpx;
  color: var(--color-text-primary);
}
</style>
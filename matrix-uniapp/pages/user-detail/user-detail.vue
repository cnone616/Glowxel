<template>
  <view class="user-detail-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="fanhui" :size="40" color="#FFFFFF" />
        </view>
        <text class="header-title">用户详情</text>
        <view class="more-btn" @click="showMoreActions">
          <Icon name="gengduo" :size="40" color="#FFFFFF" />
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view 
      scroll-y 
      class="main-content"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 用户信息卡片 -->
      <view class="user-info-section">
        <view class="user-card">
          <!-- 用户头像和基本信息 -->
          <view class="user-basic">
            <Avatar :src="userInfo.avatar" :size="120" />
            <view class="user-details">
              <text class="user-name">{{ userInfo.name }}</text>
              <text class="user-bio">{{ userInfo.bio || '这个人很懒，什么都没留下' }}</text>
              <view class="user-meta">
                <text class="join-time">{{ formatJoinTime(userInfo.joinTime) }}加入</text>
              </view>
            </view>
          </view>
          
          <!-- 统计数据 -->
          <view class="user-stats">
            <view class="stat-item" @click="goToUserWorks">
              <text class="stat-number">{{ userInfo.worksCount || 0 }}</text>
              <text class="stat-label">作品</text>
            </view>
            <view class="stat-item" @click="goToFollowers">
              <text class="stat-number">{{ userInfo.followersCount || 0 }}</text>
              <text class="stat-label">粉丝</text>
            </view>
            <view class="stat-item" @click="goToFollowing">
              <text class="stat-number">{{ userInfo.followingCount || 0 }}</text>
              <text class="stat-label">关注</text>
            </view>
            <view class="stat-item">
              <text class="stat-number">{{ userInfo.totalLikes || 0 }}</text>
              <text class="stat-label">获赞</text>
            </view>
          </view>
          
          <!-- 操作按钮 -->
          <view class="user-actions">
            <view 
              v-if="!isCurrentUser"
              class="follow-btn"
              :class="{ 'following': userInfo.isFollowing }"
              @click="toggleFollow"
            >
              <Icon :name="userInfo.isFollowing ? 'user-filling' : 'user'" :size="32" :color="userInfo.isFollowing ? '#FFFFFF' : '#4F7FFF'" />
              <text class="follow-text">
                {{ userInfo.isFollowing ? '已关注' : '关注' }}
              </text>
            </view>
            
            <view class="message-btn" @click="sendMessage">
              <Icon name="comment" :size="32" color="#666666" />
              <text class="message-text">私信</text>
            </view>
            
            <view class="share-btn" @click="shareUser">
              <Icon name="share" :size="32" color="#666666" />
              <text class="share-text">分享</text>
            </view>
          </view>
        </view>
      </view>
      
      <!-- 作品分类标签 -->
      <view class="category-section">
        <view class="category-tabs">
          <view 
            v-for="(category, index) in categories" 
            :key="category.key"
            class="tab-item"
            :class="{ 'active': activeCategory === category.key }"
            @click="switchCategory(category.key)"
          >
            <text class="tab-text">{{ category.name }}</text>
            <text v-if="category.count !== undefined" class="tab-count">({{ category.count }})</text>
          </view>
        </view>
      </view>
      
      <!-- 作品列表 -->
      <view class="works-section">
        <view v-if="filteredWorks.length === 0 && !isLoading" class="empty-state">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="#AAAAAA" />
          </view>
          <text class="empty-title">{{ getEmptyTitle() }}</text>
          <text class="empty-subtitle">{{ getEmptySubtitle() }}</text>
        </view>
        
        <view v-else class="works-grid">
          <ArtworkCard 
            v-for="artwork in filteredWorks" 
            :key="artwork.id"
            :artwork="artwork"
            @click="goToArtwork"
          />
        </view>
        
        <!-- 加载更多 -->
        <view v-if="isLoading" class="loading-more">
          <Icon name="loading" :size="48" color="#4F7FFF" class="loading-icon" />
          <text class="loading-text">加载中...</text>
        </view>
        
        <view v-if="hasMore && !isLoading && filteredWorks.length > 0" class="load-more-btn" @click="loadMore">
          <text class="load-more-text">加载更多</text>
        </view>
        
        <view v-if="!hasMore && filteredWorks.length > 0" class="no-more">
          <text class="no-more-text">没有更多了</text>
        </view>
      </view>
    </scroll-view>
    
    <!-- 更多操作弹窗 -->
    <Modal v-model:visible="showMoreModal" title="更多操作">
      <view class="more-actions">
        <view v-if="!isCurrentUser" class="action-item" @click="reportUser">
          <Icon name="warning" :size="40" color="#FF4757" />
          <text class="action-label">举报用户</text>
        </view>
        <view v-if="!isCurrentUser" class="action-item" @click="blockUser">
          <Icon name="close" :size="40" color="#FF4757" />
          <text class="action-label">拉黑用户</text>
        </view>
        <view class="action-item" @click="copyUserLink">
          <Icon name="link" :size="40" color="#4F7FFF" />
          <text class="action-label">复制链接</text>
        </view>
      </view>
    </Modal>
  </view>
</template>

<script>
import { MockAPI } from '../../data/mock/index.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Avatar from '../../components/Avatar.vue'
import ArtworkCard from '../../components/ArtworkCard.vue'
import Modal from '../../components/Modal.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    ArtworkCard,
    Modal
  },
  
  data() {
    return {
      userId: '',
      userInfo: {},
      userWorks: [],
      isCurrentUser: false,
      
      // 分类相关
      activeCategory: 'all',
      categories: [
        { key: 'all', name: '全部', count: 0 },
        { key: 'latest', name: '最新', count: 0 },
        { key: 'popular', name: '最热', count: 0 },
        { key: 'collected', name: '收藏', count: 0 }
      ],
      
      // 数据相关
      isLoading: false,
      isRefreshing: false,
      hasMore: true,
      currentPage: 1,
      pageSize: 20,
      
      // 弹窗状态
      showMoreModal: false
    }
  },
  
  computed: {
    filteredWorks() {
      if (!this.userWorks.length) return []
      
      switch (this.activeCategory) {
        case 'latest':
          return this.userWorks.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt))
        case 'popular':
          return this.userWorks.sort((a, b) => (b.likes || 0) - (a.likes || 0))
        case 'collected':
          // TODO: 获取用户收藏的作品
          return []
        default:
          return this.userWorks
      }
    }
  },
  
  onLoad(options) {
    this.userId = options.id
    if (this.userId) {
      this.loadUserDetail()
    }
  },
  
  methods: {
    async loadUserDetail() {
      try {
        this.isLoading = true
        
        // 获取用户详情
        this.userInfo = await MockAPI.users.getById(this.userId)
        
        // 检查是否是当前用户
        const currentUser = await MockAPI.users.getCurrent()
        this.isCurrentUser = currentUser && currentUser.id == this.userId
        
        // 获取用户作品
        await this.loadUserWorks()
        
        // 更新分类计数
        this.updateCategoryCounts()
        
      } catch (error) {
        console.error('加载用户详情失败:', error)
        uni.showToast({
          title: '加载失败',
          icon: 'error'
        })
      } finally {
        this.isLoading = false
      }
    },
    
    async loadUserWorks(reset = false) {
      try {
        if (reset) {
          this.currentPage = 1
          this.userWorks = []
          this.hasMore = true
        }
        
        // 获取用户作品列表
        const works = await MockAPI.artworks.getByUserId(this.userId, this.currentPage, this.pageSize)
        
        if (reset) {
          this.userWorks = works
        } else {
          this.userWorks = [...this.userWorks, ...works]
        }
        
        this.hasMore = works.length === this.pageSize
        this.currentPage++
        
      } catch (error) {
        console.error('加载用户作品失败:', error)
        throw error
      }
    },
    
    updateCategoryCounts() {
      this.categories = this.categories.map(category => {
        switch (category.key) {
          case 'all':
            return { ...category, count: this.userWorks.length }
          case 'latest':
            return { ...category, count: this.userWorks.length }
          case 'popular':
            return { ...category, count: this.userWorks.filter(w => (w.likes || 0) > 50).length }
          case 'collected':
            return { ...category, count: 0 } // TODO: 实际收藏数
          default:
            return category
        }
      })
    },
    
    async handleRefresh() {
      this.isRefreshing = true
      
      try {
        await this.loadUserDetail()
      } catch (error) {
        console.error('刷新失败:', error)
      } finally {
        this.isRefreshing = false
      }
    },
    
    async loadMore() {
      if (this.isLoading || !this.hasMore) return
      
      this.isLoading = true
      
      try {
        await this.loadUserWorks(false)
      } catch (error) {
        console.error('加载更多失败:', error)
      } finally {
        this.isLoading = false
      }
    },
    
    async switchCategory(category) {
      if (this.activeCategory === category) return
      
      this.activeCategory = category
      // 分类切换不需要重新请求数据，只是重新排序
    },
    
    handleBack() {
      uni.navigateBack()
    },
    
    async toggleFollow() {
      if (this.isCurrentUser) return
      
      try {
        const isFollowing = !this.userInfo.isFollowing
        
        // 调用关注API
        if (isFollowing) {
          await MockAPI.follows.follow(this.userId)
          this.userInfo.followersCount = (this.userInfo.followersCount || 0) + 1
        } else {
          await MockAPI.follows.unfollow(this.userId)
          this.userInfo.followersCount = Math.max(0, (this.userInfo.followersCount || 0) - 1)
        }
        
        this.userInfo.isFollowing = isFollowing
        
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
    
    sendMessage() {
      uni.showToast({
        title: '私信功能开发中',
        icon: 'none'
      })
    },
    
    shareUser() {
      // 微信小程序分享
      uni.showShareMenu({
        withShareTicket: true
      })
    },
    
    showMoreActions() {
      this.showMoreModal = true
    },
    
    reportUser() {
      this.showMoreModal = false
      uni.showToast({
        title: '举报功能开发中',
        icon: 'none'
      })
    },
    
    blockUser() {
      this.showMoreModal = false
      uni.showToast({
        title: '拉黑功能开发中',
        icon: 'none'
      })
    },
    
    copyUserLink() {
      this.showMoreModal = false
      uni.setClipboardData({
        data: `用户：${this.userInfo.name}`,
        success: () => {
          uni.showToast({
            title: '链接已复制',
            icon: 'success'
          })
        }
      })
    },
    
    goToUserWorks() {
      // 切换到全部作品
      this.activeCategory = 'all'
    },
    
    goToFollowers() {
      uni.navigateTo({
        url: `/pages/user-list/user-list?type=followers&userId=${this.userId}`
      })
    },
    
    goToFollowing() {
      uni.navigateTo({
        url: `/pages/user-list/user-list?type=following&userId=${this.userId}`
      })
    },
    
    goToArtwork(artwork) {
      uni.navigateTo({
        url: `/pages/artwork-detail/artwork-detail?id=${artwork.id}`
      })
    },
    
    getEmptyTitle() {
      const titleMap = {
        all: '暂无作品',
        latest: '暂无最新作品',
        popular: '暂无热门作品',
        collected: '暂无收藏'
      }
      return titleMap[this.activeCategory] || '暂无内容'
    },
    
    getEmptySubtitle() {
      const subtitleMap = {
        all: '快来发布第一个作品吧！',
        latest: '还没有最新作品',
        popular: '还没有热门作品',
        collected: '还没有收藏任何作品'
      }
      return subtitleMap[this.activeCategory] || '敬请期待！'
    },
    
    formatJoinTime(dateString) {
      if (!dateString) return ''
      
      const date = new Date(dateString)
      const year = date.getFullYear()
      const month = date.getMonth() + 1
      
      return `${year}年${month}月`
    }
  }
}
</script>

<style scoped>
.user-detail-page {
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

/* 用户信息区域 */
.user-info-section {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 0 32rpx 48rpx;
}

.user-card {
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-large);
  padding: 32rpx;
  backdrop-filter: blur(10rpx);
}

.user-basic {
  display: flex;
  align-items: flex-start;
  gap: 24rpx;
  margin-bottom: 32rpx;
}

.user-details {
  flex: 1;
}

.user-name {
  font-size: 40rpx;
  font-weight: 700;
  color: #FFFFFF;
  display: block;
  margin-bottom: 8rpx;
}

.user-bio {
  font-size: 28rpx;
  color: rgba(255, 255, 255, 0.8);
  line-height: 1.5;
  display: block;
  margin-bottom: 12rpx;
}

.user-meta {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.join-time {
  font-size: 24rpx;
  color: rgba(255, 255, 255, 0.6);
}

/* 统计数据 */
.user-stats {
  display: flex;
  justify-content: space-around;
  margin-bottom: 32rpx;
  padding: 24rpx 0;
  border-top: 1rpx solid rgba(255, 255, 255, 0.2);
  border-bottom: 1rpx solid rgba(255, 255, 255, 0.2);
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  transition: all 0.2s ease;
}

.stat-item:active {
  transform: scale(0.95);
}

.stat-number {
  font-size: 36rpx;
  font-weight: 700;
  color: #FFFFFF;
}

.stat-label {
  font-size: 24rpx;
  color: rgba(255, 255, 255, 0.7);
}

/* 操作按钮 */
.user-actions {
  display: flex;
  gap: 16rpx;
}

.follow-btn, .message-btn, .share-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 20rpx;
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.follow-btn {
  background-color: var(--color-brand-primary);
}

.follow-btn.following {
  background-color: rgba(255, 255, 255, 0.2);
}

.message-btn, .share-btn {
  background-color: rgba(255, 255, 255, 0.1);
}

.follow-btn:active, .message-btn:active, .share-btn:active {
  transform: scale(0.95);
}

.follow-text, .message-text, .share-text {
  font-size: 26rpx;
  font-weight: 500;
  color: #FFFFFF;
}

/* 分类标签 */
.category-section {
  background-color: var(--color-card-background);
  margin: -24rpx 32rpx 32rpx;
  border-radius: var(--radius-large);
  padding: 32rpx;
  box-shadow: var(--shadow-card);
}

.category-tabs {
  display: flex;
  gap: 16rpx;
}

.tab-item {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  border-radius: var(--radius-medium);
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
}

.tab-item:active {
  transform: scale(0.95);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--color-text-secondary);
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #FFFFFF;
  font-weight: 600;
}

.tab-count {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.tab-item.active .tab-count {
  color: rgba(255, 255, 255, 0.8);
}

/* 作品区域 */
.works-section {
  margin: 0 32rpx 32rpx;
}

.works-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
  margin-bottom: 48rpx;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 96rpx 32rpx;
  text-align: center;
}

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--color-app-background);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.5;
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
}

.loading-icon {
  animation: rotate 1s linear infinite;
}

@keyframes rotate {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.loading-text {
  font-size: 26rpx;
  color: var(--color-text-secondary);
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  border: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  transform: scale(0.98);
  background-color: var(--color-app-background);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--color-brand-primary);
  font-weight: 500;
}

.no-more {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
}

.no-more-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
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
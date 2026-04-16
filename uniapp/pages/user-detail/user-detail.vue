<template>
  <view class="user-detail-page glx-page-shell">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header glx-topbar glx-page-shell__fixed">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="32" color="var(--nb-ink)" />
        </view>
        <text class="header-title glx-topbar__title">用户详情</text>
        <view class="more-btn glx-cta-button" @click="showMoreActions">
          <Icon name="more-horizontal" :size="40" color="var(--nb-ink)" />
        </view>
      </view>
    </view>
    
    <!-- 主要内容 -->
    <scroll-view 
      scroll-y 
      class="main-content glx-scroll-region glx-page-shell__content"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 用户信息卡片 -->
      <view class="user-info-section">
        <view class="user-card glx-panel-card">
          <!-- 用户头像和基本信息 -->
          <view class="user-basic">
            <Avatar :src="userInfo.avatar" :size="120" />
            <view class="user-details">
              <text class="user-name">{{ userInfo.name }}</text>
              <text class="user-bio">{{ userInfo.bio || '这个人很懒，什么都没留下' }}</text>
              <view class="user-meta">
                <text class="join-time">{{ formatJoinTime(userInfo.created_at) }}加入</text>
              </view>
            </view>
          </view>
          
          <!-- 统计数据 -->
          <view class="user-stats">
            <view class="stat-item" @click="goToUserWorks">
              <text class="stat-number">{{ userInfo.works_count || 0 }}</text>
              <text class="stat-label">作品</text>
            </view>
            <view class="stat-item" @click="goToFollowers">
              <text class="stat-number">{{ userInfo.followers_count || 0 }}</text>
              <text class="stat-label">粉丝</text>
            </view>
            <view class="stat-item" @click="goToFollowing">
              <text class="stat-number">{{ userInfo.following_count || 0 }}</text>
              <text class="stat-label">关注</text>
            </view>
            <view class="stat-item">
              <text class="stat-number">{{ userInfo.total_likes || 0 }}</text>
              <text class="stat-label">获赞</text>
            </view>
          </view>
          
          <!-- 操作按钮 -->
          <view class="user-actions">
            <view 
              v-if="!isCurrentUser"
              class="follow-btn glx-cta-button"
              :class="{ 'following': isFollowing }"
              @click="toggleFollow"
            >
              <Icon :name="isFollowing ? 'user-filling' : 'user'" :size="32" color="var(--nb-ink)" />
              <text class="follow-text">
                {{ isFollowing ? '已关注' : '关注' }}
              </text>
            </view>
            
            <view class="message-btn glx-action-tile" @click="sendMessage">
              <Icon name="comment" :size="32" color="currentColor" />
              <text class="message-text">私信</text>
            </view>
            
            <view class="share-btn glx-action-tile" @click="shareUser">
              <Icon name="share" :size="32" color="currentColor" />
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
            class="tab-item glx-choice-chip"
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
        <view v-if="filteredWorks.length === 0 && !isLoading" class="empty-state glx-panel-card">
          <view class="empty-icon">
            <Icon name="picture" :size="80" color="var(--text-tertiary)" />
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
          <Icon name="loading" :size="48" color="currentColor" class="loading-icon" />
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
        <view v-if="!isCurrentUser" class="action-item danger" @click="reportUser">
          <Icon name="warning" :size="40" color="currentColor" />
          <text class="action-label">举报用户</text>
        </view>
        <view v-if="!isCurrentUser" class="action-item danger" @click="blockUser">
          <Icon name="close" :size="40" color="currentColor" />
          <text class="action-label">拉黑用户</text>
        </view>
        <view class="action-item link" @click="copyUserLink">
          <Icon name="link" :size="40" color="currentColor" />
          <text class="action-label">复制链接</text>
        </view>
      </view>
    </Modal>

    <ConfirmDialogHost />
    <Toast />
  </view>
</template>

<script>
import { userAPI, followAPI } from '../../api/index.js'
import { useDialog } from '../../composables/useDialog.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Avatar from '../../components/Avatar.vue'
import ArtworkCard from '../../components/ArtworkCard.vue'
import ConfirmDialogHost from '../../components/ConfirmDialogHost.vue'
import Modal from '../../components/Modal.vue'
import Toast from '../../components/Toast.vue'

const dialog = useDialog()
const toast = useToast()

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    ArtworkCard,
    ConfirmDialogHost,
    Modal,
    Toast
  },
  
  data() {
    return {
      userId: '',
      userInfo: {},
      userWorks: [],
      isCurrentUser: false,
      isFollowing: false,
      
      // 分类相关
      activeCategory: 'all',
      categories: [
        { key: 'all', name: '全部', count: 0 },
        { key: 'latest', name: '最新', count: 0 },
        { key: 'popular', name: '最热', count: 0 }
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
          return [...this.userWorks].sort((a, b) => new Date(b.created_at) - new Date(a.created_at))
        case 'popular':
          return [...this.userWorks].sort((a, b) => b.likes - a.likes)
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
        const uRes = await userAPI.getUserById(this.userId)
        if (uRes.success && uRes.data && uRes.data.user) {
          this.userInfo = uRes.data.user
        }
        
        // 检查是否是当前用户
        const pRes = await userAPI.getProfile()
        if (pRes.success && pRes.data && pRes.data.user) {
          this.isCurrentUser = pRes.data.user.id == this.userId
          await this.loadFollowState(pRes.data.user.id)
        } else {
          this.isCurrentUser = false
          this.isFollowing = false
        }

        // 获取用户作品
        await this.loadUserWorks()
        
        // 更新分类计数
        this.updateCategoryCounts()
        
      } catch (error) {
        console.error('加载用户详情失败:', error)
        toast.showError('加载失败')
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
        const wRes = await userAPI.getUserArtworks(this.userId, this.currentPage, this.pageSize)
        const works = wRes.success && wRes.data ? wRes.data.list : []
        
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
            return { ...category, count: this.userWorks.filter(w => w.likes > 0).length }
          default:
            return category
        }
      })
    },

    async loadFollowState(currentUserId) {
      if (!currentUserId) {
        this.isFollowing = false
        return
      }

      const res = await followAPI.getFollowing(currentUserId, 1, 100)
      if (res.success && res.data && res.data.list) {
        this.isFollowing = res.data.list.some((user) => user.id == this.userId)
      } else {
        this.isFollowing = false
      }
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
        const res = await followAPI.toggleFollow(this.userId)
        if (!(res.success && res.data)) {
          throw new Error('follow failed')
        }

        this.isFollowing = res.data.followed
        if (res.data.followed) {
          this.userInfo.followers_count += 1
        } else {
          this.userInfo.followers_count -= 1
        }
        
        toast.showSuccess(res.data.followed ? '关注成功' : '取消关注')
      } catch (error) {
        console.error('关注操作失败:', error)
        toast.showError('操作失败')
      }
    },
    
    async sendMessage() {
      await dialog.alert({
        title: '私信说明',
        content: '当前版本暂未开放站内私信，你可以先通过分享主页或关注对方保持联系。'
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
      uni.setClipboardData({
        data: `举报用户：${this.userInfo.name}（ID: ${this.userInfo.id}）`,
        success: () => {
          toast.showSuccess('举报信息已复制')
        },
        fail: () => {
          toast.showError('复制失败')
        }
      })
    },
    
    async blockUser() {
      this.showMoreModal = false
      await dialog.alert({
        title: '拉黑说明',
        content: '当前版本暂未提供拉黑能力，如遇恶意内容，可先复制举报信息并联系平台处理。'
      })
    },
    
    copyUserLink() {
      this.showMoreModal = false
      uni.setClipboardData({
        data: `用户：${this.userInfo.name}`,
        success: () => {
          toast.showSuccess('链接已复制')
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
        popular: '暂无热门作品'
      }
      return titleMap[this.activeCategory] || '暂无内容'
    },
    
    getEmptySubtitle() {
      const subtitleMap = {
        all: '快来发布第一个作品吧！',
        latest: '还没有最新作品',
        popular: '还没有热门作品'
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
  background-color: var(--nb-paper);
}

/* 状态栏占位 */
.status-bar {
  background: var(--nb-paper);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background: var(--nb-paper);
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
  border-radius: 0;
  transition: all 0.2s ease;
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
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
  background: var(--tone-blue-soft);
  padding: 0 32rpx 48rpx;
}

.user-card {
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 32rpx;
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-strong);
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
  color: var(--nb-ink);
  display: block;
  margin-bottom: 8rpx;
}

.user-bio {
  font-size: 28rpx;
  color: var(--text-secondary);
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
  color: var(--text-tertiary);
}

/* 统计数据 */
.user-stats {
  display: flex;
  justify-content: space-around;
  margin-bottom: 32rpx;
  padding: 24rpx 0;
  border-top: 2rpx solid rgba(0, 0, 0, 0.12);
  border-bottom: 2rpx solid rgba(0, 0, 0, 0.12);
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  transition: all 0.2s ease;
}

.stat-number {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
}

.stat-label {
  font-size: 24rpx;
  color: var(--text-secondary);
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
  border-radius: 0;
  transition: all 0.2s ease;
}

.follow-btn {
  background-color: var(--nb-yellow);
  border: 2rpx solid var(--nb-ink);
  color: var(--nb-ink);
}

.follow-btn.following {
  background-color: var(--tone-green-soft);
}

.message-btn, .share-btn {
  background-color: var(--tone-paper-soft);
  border: 2rpx solid var(--nb-ink);
  color: var(--text-secondary);
}

.follow-text, .message-text, .share-text {
  font-size: 26rpx;
  font-weight: 500;
  color: currentColor;
}

.follow-btn.following .follow-text {
  color: var(--nb-ink);
}

/* 分类标签 */
.category-section {
  background-color: var(--tone-paper-soft);
  margin: -24rpx 32rpx 32rpx;
  border-radius: 0;
  padding: 32rpx;
  box-shadow: var(--nb-shadow-strong);
  border: 2rpx solid var(--nb-ink);
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
  border-radius: 0;
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.tab-item.active {
  background-color: var(--nb-yellow);
  border-color: var(--nb-ink);
}

.tab-text {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--text-secondary);
  white-space: nowrap;
}

.tab-item.active .tab-text {
  color: #000000;
  font-weight: 700;
}

.tab-count {
  font-size: 22rpx;
  color: var(--text-tertiary);
}

.tab-item.active .tab-count {
  color: rgba(0, 0, 0, 0.7);
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

.empty-icon {
  width: 152rpx;
  height: 152rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--tone-blue-soft);
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
  border-radius: 0;
  margin-bottom: 32rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
  margin-bottom: 12rpx;
}

.empty-subtitle {
  font-size: 26rpx;
  color: var(--text-secondary);
  line-height: 1.5;
}

/* 加载状态 */
.loading-more {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 48rpx;
  gap: 16rpx;
  color: var(--nb-blue);
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
  color: currentColor;
}

.load-more-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32rpx;
  margin: 32rpx 0;
  background-color: var(--tone-paper-soft);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  transition: background-color 0.2s ease;
  box-shadow: var(--nb-shadow-soft);
}

.load-more-btn:active {
  background-color: var(--nb-paper);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--nb-ink);
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
  color: var(--text-tertiary);
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
  color: var(--nb-ink);
}

.action-item:active {
  background-color: var(--tone-paper-soft);
}

.action-item.danger {
  color: var(--nb-coral);
}

.action-item.link {
  color: var(--nb-blue);
}

.action-label {
  font-size: 32rpx;
  color: currentColor;
}
</style>

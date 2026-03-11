<template>
  <view class="following-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="arrow-left" :size="32" />
      </view>
      <text class="nav-title">关注</text>
      <view class="nav-right" @click="showSearchBar = !showSearchBar">
        <Icon name="search" :size="32" />
      </view>
    </view>
    
    <!-- 搜索栏 -->
    <view v-if="showSearchBar" class="search-section">
      <view class="search-bar">
        <Icon name="search" :size="28" color="var(--color-text-disabled)" />
        <input 
          v-model="searchQuery"
          class="search-input"
          placeholder="搜索关注的用户..."
          focus
        />
        <view v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">
          <Icon name="close" :size="24" />
        </view>
      </view>
    </view>
    
    <!-- 分类标签 -->
    <view class="category-tabs">
      <view 
        v-for="category in categories"
        :key="category.value"
        class="category-tab"
        :class="{ active: currentCategory === category.value }"
        @click="currentCategory = category.value"
      >
        <text class="category-text">{{ category.label }}</text>
        <text v-if="category.count > 0" class="category-count">{{ category.count }}</text>
      </view>
    </view>
    
    <!-- 关注列表 -->
    <scroll-view scroll-y class="content" @scrolltolower="loadMore">
      <view v-if="filteredFollowing.length === 0" class="empty-state">
        <Icon name="user-plus" :size="120" color="var(--color-text-disabled)" />
        <text class="empty-title">{{ searchQuery ? '未找到相关用户' : getEmptyTitle() }}</text>
        <text class="empty-desc">{{ searchQuery ? '试试其他关键词' : getEmptyDesc() }}</text>
        <button v-if="!searchQuery && currentCategory === 'all'" class="explore-btn" @click="goToExplore">
          <Icon name="compass" :size="32" />
          <text>去发现</text>
        </button>
      </view>
      
      <view v-else class="following-list">
        <view 
          v-for="user in filteredFollowing"
          :key="user.id"
          class="following-item"
          @click="goToUserProfile(user)"
        >
          <!-- 头像 -->
          <view class="user-avatar">
            <image 
              v-if="user.avatar"
              :src="user.avatar"
              class="avatar-image"
              mode="aspectFill"
            />
            <view v-else class="avatar-placeholder">
              <Icon name="user" :size="60" color="var(--color-text-disabled)" />
            </view>
            
            <!-- 在线状态 -->
            <view v-if="user.isOnline" class="online-indicator"></view>
            
            <!-- 特殊标识 -->
            <view v-if="user.isVerified" class="verified-badge">
              <Icon name="check" :size="20" color="#FFFFFF" />
            </view>
          </view>
          
          <!-- 用户信息 -->
          <view class="user-info">
            <view class="user-name-row">
              <text class="user-name">{{ user.nickname }}</text>
              <view v-if="user.isMutual" class="mutual-badge">
                <Icon name="heart" :size="20" />
                <text class="mutual-text">互关</text>
              </view>
            </view>
            
            <text class="user-bio">{{ user.bio || '这个人很懒，什么都没写' }}</text>
            
            <view class="user-meta">
              <text class="follow-time">{{ formatFollowTime(user.followTime) }}</text>
              <view class="user-stats">
                <text class="stat-item">{{ user.worksCount }} 作品</text>
                <text class="stat-item">{{ user.followersCount }} 粉丝</text>
              </view>
            </view>
            
            <!-- 最新作品预览 -->
            <view v-if="user.latestWork" class="latest-work">
              <text class="work-label">最新:</text>
              <text class="work-title">{{ user.latestWork.title }}</text>
              <text class="work-time">{{ formatWorkTime(user.latestWork.createdAt) }}</text>
            </view>
          </view>
          
          <!-- 操作按钮 -->
          <view class="user-actions">
            <button class="unfollow-btn" @click.stop="unfollowUser(user)">
              <text>取消关注</text>
            </button>
            
            <button class="message-btn" @click.stop="sendMessage(user)">
              <Icon name="message" :size="24" />
            </button>
          </view>
        </view>
      </view>
      
      <!-- 加载更多 -->
      <view v-if="hasMore && !isLoading" class="load-more" @click="loadMore">
        <text class="load-text">点击加载更多</text>
      </view>
      
      <view v-if="isLoading" class="loading">
        <text class="loading-text">加载中...</text>
      </view>
    </scroll-view>
    
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast
  },
  
  data() {
    return {
      toast: null,
      showSearchBar: false,
      searchQuery: '',
      currentCategory: 'all',
      isLoading: false,
      hasMore: true,
      categories: [
        { value: 'all', label: '全部', count: 0 },
        { value: 'mutual', label: '互关', count: 0 },
        { value: 'active', label: '活跃', count: 0 },
        { value: 'recent', label: '最近', count: 0 }
      ],
      following: [
        // 模拟数据
        {
          id: '1',
          nickname: '像素大师',
          avatar: '',
          bio: '专业像素艺术家，10年创作经验',
          followTime: Date.now() - 86400000,
          worksCount: 156,
          followersCount: 2340,
          isMutual: true,
          isOnline: true,
          isVerified: true,
          isActive: true,
          latestWork: {
            title: '赛博朋克城市',
            createdAt: Date.now() - 3600000
          }
        },
        {
          id: '2',
          nickname: '创意工坊',
          avatar: '',
          bio: '分享创意设计和教程',
          followTime: Date.now() - 172800000,
          worksCount: 89,
          followersCount: 1250,
          isMutual: false,
          isOnline: false,
          isVerified: false,
          isActive: true,
          latestWork: {
            title: '像素动物系列',
            createdAt: Date.now() - 86400000
          }
        },
        {
          id: '3',
          nickname: '拼豆达人',
          avatar: '',
          bio: '用拼豆还原经典游戏角色',
          followTime: Date.now() - 259200000,
          worksCount: 67,
          followersCount: 890,
          isMutual: true,
          isOnline: true,
          isVerified: false,
          isActive: false,
          latestWork: {
            title: '马里奥兄弟',
            createdAt: Date.now() - 172800000
          }
        },
        {
          id: '4',
          nickname: '艺术学院',
          avatar: '',
          bio: '官方账号，分享教学资源',
          followTime: Date.now() - 345600000,
          worksCount: 234,
          followersCount: 5670,
          isMutual: false,
          isOnline: false,
          isVerified: true,
          isActive: true,
          latestWork: {
            title: '色彩搭配指南',
            createdAt: Date.now() - 259200000
          }
        },
        {
          id: '5',
          nickname: '游戏怀旧',
          avatar: '',
          bio: '重现经典游戏的像素艺术',
          followTime: Date.now() - 432000000,
          worksCount: 45,
          followersCount: 567,
          isMutual: false,
          isOnline: false,
          isVerified: false,
          isActive: false,
          latestWork: {
            title: '塞尔达传说',
            createdAt: Date.now() - 345600000
          }
        }
      ]
    }
  },
  
  computed: {
    filteredFollowing() {
      let users = [...this.following]
      
      // 分类过滤
      switch (this.currentCategory) {
        case 'mutual':
          users = users.filter(u => u.isMutual)
          break
        case 'active':
          users = users.filter(u => u.isActive)
          break
        case 'recent':
          const weekAgo = Date.now() - 7 * 24 * 60 * 60 * 1000
          users = users.filter(u => u.followTime > weekAgo)
          break
      }
      
      // 搜索过滤
      if (this.searchQuery.trim()) {
        const query = this.searchQuery.toLowerCase()
        users = users.filter(user => 
          user.nickname.toLowerCase().includes(query) ||
          (user.bio && user.bio.toLowerCase().includes(query))
        )
      }
      
      return users
    }
  },
  
  onLoad() {
    this.toast = useToast()
    this.updateCategoryCounts()
    
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
  },
  
  methods: {
    goBack() {
      uni.navigateBack()
    },
    
    goToExplore() {
      uni.switchTab({
        url: '/pages/community/community'
      })
    },
    
    goToUserProfile(user) {
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${user.id}`
      })
    },
    
    updateCategoryCounts() {
      this.categories[0].count = this.following.length
      this.categories[1].count = this.following.filter(u => u.isMutual).length
      this.categories[2].count = this.following.filter(u => u.isActive).length
      
      const weekAgo = Date.now() - 7 * 24 * 60 * 60 * 1000
      this.categories[3].count = this.following.filter(u => u.followTime > weekAgo).length
    },
    
    getEmptyTitle() {
      switch (this.currentCategory) {
        case 'mutual': return '暂无互关用户'
        case 'active': return '暂无活跃用户'
        case 'recent': return '本周无新关注'
        default: return '暂未关注任何人'
      }
    },
    
    getEmptyDesc() {
      switch (this.currentCategory) {
        case 'mutual': return '关注更多用户来建立互关关系'
        case 'active': return '你关注的用户最近都不太活跃'
        case 'recent': return '去发现更多有趣的用户吧'
        default: return '去社区发现有趣的创作者吧'
      }
    },
    
    async unfollowUser(user) {
      uni.showModal({
        title: '取消关注',
        content: `确定要取消关注 ${user.nickname} 吗？`,
        success: async (res) => {
          if (res.confirm) {
            try {
              // 模拟取消关注API调用
              await new Promise(resolve => setTimeout(resolve, 500))
              
              const index = this.following.findIndex(u => u.id === user.id)
              if (index > -1) {
                this.following.splice(index, 1)
                this.updateCategoryCounts()
                this.toast.showInfo(`已取消关注 ${user.nickname}`)
              }
            } catch (error) {
              this.toast.showError('操作失败，请重试')
            }
          }
        }
      })
    },
    
    sendMessage(user) {
      this.toast.showInfo('私信功能开发中')
    },
    
    async loadMore() {
      if (this.isLoading || !this.hasMore) return
      
      this.isLoading = true
      
      try {
        // 模拟加载更多数据
        await new Promise(resolve => setTimeout(resolve, 1000))
        
        // 模拟没有更多数据
        this.hasMore = false
        this.toast.showInfo('已加载全部关注')
      } catch (error) {
        this.toast.showError('加载失败，请重试')
      } finally {
        this.isLoading = false
      }
    },
    
    formatFollowTime(timestamp) {
      const date = new Date(timestamp)
      const now = new Date()
      const diff = now - date
      
      if (diff < 86400000) return '今天关注'
      if (diff < 172800000) return '昨天关注'
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前关注`
      
      return date.toLocaleDateString() + ' 关注'
    },
    
    formatWorkTime(timestamp) {
      const date = new Date(timestamp)
      const now = new Date()
      const diff = now - date
      
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前`
      
      return date.toLocaleDateString()
    }
  }
}
</script>

<style scoped>
.following-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
}

.nav-left, .nav-right {
  width: 80rpx;
  display: flex;
  align-items: center;
}

.nav-right {
  justify-content: flex-end;
}

.nav-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.search-section {
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.search-bar {
  display: flex;
  align-items: center;
  gap: 16rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  padding: 16rpx 24rpx;
}

.search-input {
  flex: 1;
  font-size: 28rpx;
  color: var(--color-text-primary);
}

.clear-btn {
  padding: 8rpx;
  border-radius: 50%;
  background-color: var(--color-text-disabled);
  color: #FFFFFF;
}

.category-tabs {
  display: flex;
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  gap: 16rpx;
}

.category-tab {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.category-tab.active {
  background-color: var(--color-brand-primary);
  border-color: var(--color-brand-primary);
  color: #FFFFFF;
}

.category-text {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.category-tab.active .category-text {
  color: #FFFFFF;
}

.category-count {
  font-size: 20rpx;
  background-color: rgba(255, 255, 255, 0.2);
  padding: 4rpx 8rpx;
  border-radius: 10rpx;
  color: inherit;
}

.content {
  flex: 1;
  padding: 32rpx;
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 60vh;
  gap: 24rpx;
}

.empty-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-secondary);
}

.empty-desc {
  font-size: 24rpx;
  color: var(--color-text-disabled);
  text-align: center;
}

.explore-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 48rpx;
  background-color: var(--color-brand-primary);
  color: #FFFFFF;
  border: none;
  border-radius: 16rpx;
  font-size: 28rpx;
  font-weight: 600;
}

.explore-btn::after {
  border: none;
}

.following-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.following-item {
  display: flex;
  align-items: flex-start;
  gap: 24rpx;
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.following-item:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.user-avatar {
  position: relative;
  width: 100rpx;
  height: 100rpx;
  border-radius: 50rpx;
  overflow: hidden;
  background-color: var(--color-app-background);
  flex-shrink: 0;
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
}

.online-indicator {
  position: absolute;
  bottom: 4rpx;
  right: 4rpx;
  width: 24rpx;
  height: 24rpx;
  background-color: var(--color-success);
  border: 4rpx solid var(--color-card-background);
  border-radius: 50%;
}

.verified-badge {
  position: absolute;
  top: -4rpx;
  right: -4rpx;
  width: 32rpx;
  height: 32rpx;
  background-color: var(--color-brand-primary);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 4rpx solid var(--color-card-background);
}

.user-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.user-name-row {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.user-name {
  font-size: 28rpx;
  font-weight: 600;
  color: var(--color-text-primary);
}

.mutual-badge {
  display: flex;
  align-items: center;
  gap: 4rpx;
  background-color: rgba(255, 122, 69, 0.1);
  color: var(--color-brand-accent);
  padding: 4rpx 8rpx;
  border-radius: 12rpx;
}

.mutual-text {
  font-size: 20rpx;
  font-weight: 500;
}

.user-bio {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.user-meta {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.follow-time {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.user-stats {
  display: flex;
  gap: 16rpx;
}

.stat-item {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.latest-work {
  display: flex;
  align-items: center;
  gap: 8rpx;
  background-color: var(--color-app-background);
  padding: 12rpx 16rpx;
  border-radius: 12rpx;
  margin-top: 8rpx;
}

.work-label {
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

.work-title {
  font-size: 22rpx;
  color: var(--color-text-primary);
  font-weight: 500;
  flex: 1;
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.work-time {
  font-size: 20rpx;
  color: var(--color-text-disabled);
}

.user-actions {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  align-items: center;
}

.unfollow-btn, .message-btn {
  display: flex;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx 24rpx;
  border: none;
  border-radius: 12rpx;
  font-size: 24rpx;
  font-weight: 500;
  transition: all 0.2s ease;
}

.unfollow-btn {
  background-color: var(--color-app-background);
  color: var(--color-text-secondary);
  border: 2rpx solid var(--border-primary);
}

.message-btn {
  background-color: var(--color-app-background);
  color: var(--color-text-secondary);
  border: 2rpx solid var(--border-primary);
  padding: 12rpx;
  min-width: 64rpx;
  justify-content: center;
}

.unfollow-btn::after, .message-btn::after {
  border: none;
}

.unfollow-btn:active, .message-btn:active {
  background-color: var(--border-primary);
  transform: scale(0.95);
}

.load-more, .loading {
  text-align: center;
  padding: 32rpx;
}

.load-text, .loading-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.load-more:active {
  opacity: 0.7;
}
</style>
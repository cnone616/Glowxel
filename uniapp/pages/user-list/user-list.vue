<template>
  <view class="user-list-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 顶部导航 -->
    <view class="header">
      <view class="header-content">
        <view class="back-btn" @click="handleBack">
          <Icon name="direction-left" :size="40" color="#1F1F1F" />
        </view>
        <text class="header-title">{{ pageTitle }}</text>
        <view class="search-btn" @click="toggleSearch">
          <Icon name="search" :size="40" color="#4F7FFF" />
        </view>
      </view>
    </view>
    
    <!-- 搜索栏 -->
    <view v-if="showSearch" class="search-section">
      <Input
        v-model="searchTerm"
        type="search"
        placeholder="搜索用户..."
        prefix-icon="search"
        clearable
        :focus="showSearch"
        @blur="handleSearchBlur"
      />
    </view>
    
    <!-- 主要内容 -->
    <scroll-view 
      scroll-y 
      class="main-content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 用户列表 -->
      <view class="user-section">
        <view v-if="filteredUsers.length === 0 && !isLoading" class="empty-state">
          <view class="empty-icon">
            <Icon name="user" :size="80" color="#AAAAAA" />
          </view>
          <text class="empty-title">{{ getEmptyTitle() }}</text>
          <text class="empty-subtitle">{{ getEmptySubtitle() }}</text>
        </view>
        
        <view v-else class="user-list">
          <view 
            v-for="user in filteredUsers" 
            :key="user.id"
            class="user-item"
            @click="goToUserProfile(user)"
          >
            <Avatar :src="user.avatar" :size="80" />
            <view class="user-info">
              <text class="user-name">{{ user.name }}</text>
              <text class="user-bio">{{ user.bio || '这个人很懒，什么都没留下' }}</text>
              <view class="user-stats">
                <text class="stat-text">{{ user.worksCount || 0 }}作品</text>
                <text class="stat-text">{{ user.followersCount || 0 }}粉丝</text>
              </view>
            </view>
            <view 
              class="follow-btn"
              :class="{ 'following': user.isFollowing }"
              @click.stop="toggleFollow(user)"
            >
              <text class="follow-text">
                {{ user.isFollowing ? '已关注' : '关注' }}
              </text>
            </view>
          </view>
        </view>
        
        <!-- 加载更多 -->
        <view v-if="isLoading" class="loading-more">
          <Icon name="loading" :size="48" color="#4F7FFF" class="loading-icon" />
          <text class="loading-text">加载中...</text>
        </view>
        
        <view v-if="hasMore && !isLoading && filteredUsers.length > 0" class="load-more-btn" @click="loadMore">
          <text class="load-more-text">加载更多</text>
        </view>
        
        <view v-if="!hasMore && filteredUsers.length > 0" class="no-more">
          <text class="no-more-text">没有更多了</text>
        </view>
      </view>
    </scroll-view>
  </view>
</template>

<script>
import { userAPI, followAPI } from '../../api/index.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Avatar from '../../components/Avatar.vue'
import Input from '../../components/Input.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    Input
  },
  
  data() {
    return {
      // 页面参数
      listType: 'followers', // followers, following, recommended
      userId: '',
      pageTitle: '用户列表',
      
      // 搜索相关
      searchTerm: '',
      showSearch: false,
      
      // 数据相关
      users: [],
      isLoading: false,
      isRefreshing: false,
      hasMore: true,
      currentPage: 1,
      pageSize: 20
    }
  },
  
  computed: {
    filteredUsers() {
      if (!this.searchTerm) {
        return this.users
      }
      
      const term = this.searchTerm.toLowerCase()
      return this.users.filter(user => 
        user.name.toLowerCase().includes(term) ||
        (user.bio && user.bio.toLowerCase().includes(term))
      )
    }
  },
  
  onLoad(options) {
    this.listType = options.type || 'followers'
    this.userId = options.userId || ''
    this.pageTitle = this.getPageTitle()
    
    this.loadInitialData()
  },
  
  methods: {
    getPageTitle() {
      const titleMap = {
        followers: '粉丝列表',
        following: '关注列表',
        recommended: '推荐用户'
      }
      return titleMap[this.listType] || '用户列表'
    },
    
    async loadInitialData() {
      this.isLoading = true
      
      try {
        await this.loadUsers(true)
      } catch (error) {
        console.error('加载数据失败:', error)
        uni.showToast({
          title: '加载失败',
          icon: 'error'
        })
      } finally {
        this.isLoading = false
      }
    },
    
    async loadUsers(reset = false) {
      if (reset) {
        this.currentPage = 1
        this.users = []
        this.hasMore = true
      }
      
      try {
        let newUsers = []
        let res

        switch (this.listType) {
          case 'followers':
            res = await followAPI.getFollowers(this.userId, this.currentPage, this.pageSize)
            newUsers = res.success ? (res.data?.list || []) : []
            break
          case 'following':
            res = await followAPI.getFollowing(this.userId, this.currentPage, this.pageSize)
            newUsers = res.success ? (res.data?.list || []) : []
            break
          case 'recommended':
            res = await userAPI.getRecommendedUsers(this.pageSize)
            newUsers = res.success ? (res.data || []) : []
            break
          default:
            res = await userAPI.getRecommendedUsers(this.pageSize)
            newUsers = res.success ? (res.data || []) : []
            break
        }
        
        if (reset) {
          this.users = newUsers
        } else {
          this.users = [...this.users, ...newUsers]
        }
        
        this.hasMore = newUsers.length === this.pageSize
        this.currentPage++
      } catch (error) {
        console.error('加载用户失败:', error)
        throw error
      }
    },
    
    async handleRefresh() {
      this.isRefreshing = true
      
      try {
        await this.loadUsers(true)
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
        await this.loadUsers(false)
      } catch (error) {
        console.error('加载更多失败:', error)
      } finally {
        this.isLoading = false
      }
    },
    
    handleBack() {
      uni.navigateBack()
    },
    
    toggleSearch() {
      this.showSearch = !this.showSearch
      if (!this.showSearch) {
        this.searchTerm = ''
      }
    },
    
    handleSearchBlur() {
      if (!this.searchTerm) {
        this.showSearch = false
      }
    },
    
    goToUserProfile(user) {
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${user.id}`
      })
    },
    
    async toggleFollow(user) {
      try {
        const result = await followAPI.toggleFollow(user.id)
        
        if (result.success) {
          user.isFollowing = result.isFollowing
          
          // 更新粉丝数
          if (result.isFollowing) {
            user.followersCount = (user.followersCount || 0) + 1
          } else {
            user.followersCount = Math.max(0, (user.followersCount || 0) - 1)
          }
          
          uni.showToast({
            title: result.message,
            icon: 'success'
          })
        }
      } catch (error) {
        console.error('关注操作失败:', error)
        uni.showToast({
          title: '操作失败',
          icon: 'error'
        })
      }
    },
    
    getEmptyTitle() {
      const titleMap = {
        followers: '暂无粉丝',
        following: '暂无关注',
        recommended: '暂无推荐用户'
      }
      
      if (this.searchTerm) {
        return '未找到相关用户'
      }
      
      return titleMap[this.listType] || '暂无用户'
    },
    
    getEmptySubtitle() {
      const subtitleMap = {
        followers: '还没有人关注你',
        following: '还没有关注任何人',
        recommended: '暂时没有推荐用户'
      }
      
      if (this.searchTerm) {
        return '尝试其他关键词'
      }
      
      return subtitleMap[this.listType] || '敬请期待！'
    }
  }
}
</script>

<style scoped>
.user-list-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--color-app-background);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--color-card-background);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--color-card-background);
  border-bottom: 1rpx solid var(--border-primary);
  flex-shrink: 0;
}

.header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24rpx 32rpx;
}

.back-btn, .search-btn {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
}

.back-btn:active {
  transform: scale(0.95);
  background-color: var(--color-app-background);
}

.search-btn {
  background-color: var(--color-app-background);
}

.search-btn:active {
  transform: scale(0.95);
  background-color: rgba(79, 127, 255, 0.1);
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
  flex: 1;
  text-align: center;
}

/* 搜索栏 */
.search-section {
  background-color: var(--color-card-background);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--border-primary);
}

/* 主要内容 */
.main-content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 32rpx;
}

/* 用户区域 */
.user-section {
  margin: 32rpx;
}

.user-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
  margin-bottom: 48rpx;
}

.user-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  background-color: var(--color-card-background);
  border-radius: var(--radius-medium);
  padding: 24rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.user-item:active {
  transform: scale(0.98);
  background-color: var(--color-app-background);
}

.user-info {
  flex: 1;
  min-width: 0;
}

.user-name {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary);
  display: block;
  margin-bottom: 8rpx;
}

.user-bio {
  font-size: 26rpx;
  color: var(--color-text-secondary);
  line-height: 1.4;
  display: block;
  margin-bottom: 12rpx;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.user-stats {
  display: flex;
  gap: 16rpx;
}

.stat-text {
  font-size: 24rpx;
  color: var(--color-text-disabled);
}

.follow-btn {
  padding: 16rpx 32rpx;
  background-color: var(--color-brand-primary);
  border-radius: var(--radius-medium);
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.follow-btn.following {
  background-color: var(--color-app-background);
  border: 2rpx solid var(--border-primary);
}

.follow-btn:active {
  transform: scale(0.95);
}

.follow-text {
  font-size: 26rpx;
  font-weight: 500;
  color: #FFFFFF;
}

.follow-btn.following .follow-text {
  color: var(--color-text-secondary);
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
</style>
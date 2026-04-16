<template>
  <view class="user-list-page glx-page-shell">
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
        <text class="header-title glx-topbar__title">{{ pageTitle }}</text>
        <view class="search-btn" @click="toggleSearch">
          <Icon name="search" :size="40" color="var(--nb-blue)" />
        </view>
      </view>
    </view>
    
    <!-- 搜索栏 -->
    <view v-if="showSearch" class="search-section glx-page-shell__fixed">
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
      class="main-content glx-scroll-region glx-page-shell__content"
      @scrolltolower="loadMore"
      :refresher-enabled="true"
      :refresher-triggered="isRefreshing"
      @refresherrefresh="handleRefresh"
    >
      <!-- 用户列表 -->
      <view class="user-section">
        <view v-if="filteredUsers.length === 0 && !isLoading" class="empty-state glx-panel-card">
          <view class="empty-icon">
            <Icon name="user" :size="80" color="var(--text-tertiary)" />
          </view>
          <text class="empty-title">{{ getEmptyTitle() }}</text>
          <text class="empty-subtitle">{{ getEmptySubtitle() }}</text>
        </view>
        
        <view v-else class="user-list">
          <view 
            v-for="user in filteredUsers" 
            :key="user.id"
            class="user-item glx-list-card"
            @click="goToUserProfile(user)"
          >
            <Avatar :src="user.avatar" :size="80" />
            <view class="user-info">
              <text class="user-name">{{ user.name }}</text>
              <text class="user-bio">{{ user.bio || '这个人很懒，什么都没留下' }}</text>
              <view class="user-stats">
                <text class="stat-text">{{ user.works_count || 0 }}作品</text>
                <text class="stat-text">{{ user.followers_count || 0 }}粉丝</text>
              </view>
            </view>
            <view 
              class="follow-btn glx-cta-button"
              :class="{ 'following': user.is_following }"
              @click.stop="toggleFollow(user)"
            >
              <text class="follow-text">
                {{ user.is_following ? '已关注' : '关注' }}
              </text>
            </view>
          </view>
        </view>
        
        <!-- 加载更多 -->
        <view v-if="isLoading" class="loading-more">
          <Icon name="loading" :size="48" color="currentColor" class="loading-icon" />
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

    <Toast />
  </view>
</template>

<script>
import { userAPI, followAPI } from '../../api/index.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Avatar from '../../components/Avatar.vue'
import Input from '../../components/Input.vue'
import Toast from '../../components/Toast.vue'

const toast = useToast()

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Avatar,
    Input,
    Toast
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
        toast.showError('加载失败')
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
            newUsers = res.success && res.data ? res.data.list : []
            break
          case 'following':
            res = await followAPI.getFollowing(this.userId, this.currentPage, this.pageSize)
            newUsers = res.success && res.data ? res.data.list : []
            break
          case 'recommended':
            res = await userAPI.getRecommendedUsers(this.pageSize)
            newUsers = res.success && res.data ? res.data.list : []
            break
          default:
            res = await userAPI.getRecommendedUsers(this.pageSize)
            newUsers = res.success && res.data ? res.data.list : []
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
        
        if (result.success && result.data) {
          user.is_following = result.data.followed
          
          if (result.data.followed) {
            user.followers_count += 1
          } else {
            user.followers_count -= 1
          }
          
          toast.showSuccess(result.data.followed ? '关注成功' : '取消关注')
        }
      } catch (error) {
        console.error('关注操作失败:', error)
        toast.showError('操作失败')
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
  background-color: var(--nb-paper);
  overflow: hidden;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--nb-surface);
  flex-shrink: 0;
}

/* 顶部导航 */
.header {
  background-color: var(--nb-surface);
  border-bottom: 1rpx solid var(--nb-ink);
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
  border-radius: 0;
  transition: all 0.2s ease;
}

.search-btn {
  background-color: var(--nb-paper);
}

.search-btn:active {
  background-color: var(--nb-yellow);
}

.header-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--nb-ink);
  flex: 1;
  text-align: center;
}

/* 搜索栏 */
.search-section {
  background-color: var(--nb-surface);
  padding: 0 32rpx 24rpx;
  border-bottom: 1rpx solid var(--nb-ink);
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
  background-color: var(--nb-surface);
  border-radius: 0;
  padding: 24rpx;
  box-shadow: var(--nb-shadow-strong);
  transition: all 0.2s ease;
}

.user-item:active {
  background-color: var(--nb-paper);
}

.user-info {
  flex: 1;
  min-width: 0;
}

.user-name {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--nb-ink);
  display: block;
  margin-bottom: 8rpx;
}

.user-bio {
  font-size: 26rpx;
  color: #4a4a4a;
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
  color: #777777;
}

.follow-btn {
  padding: 16rpx 32rpx;
  background-color: var(--nb-yellow);
  border-radius: 0;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.follow-btn.following {
  background-color: var(--nb-paper);
  border: 2rpx solid var(--nb-ink);
}

.follow-text {
  font-size: 26rpx;
  font-weight: 500;
  color: #FFFFFF;
}

.follow-btn.following .follow-text {
  color: #4a4a4a;
}

/* 空状态 */

.empty-icon {
  width: 160rpx;
  height: 160rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--nb-paper);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  box-shadow: var(--nb-shadow-soft);
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
  color: #4a4a4a;
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
  background-color: var(--nb-surface);
  border-radius: 0;
  border: 2rpx solid var(--nb-ink);
  transition: all 0.2s ease;
}

.load-more-btn:active {
  background-color: var(--nb-paper);
}

.load-more-text {
  font-size: 28rpx;
  color: var(--nb-yellow);
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
  color: #777777;
}
</style>

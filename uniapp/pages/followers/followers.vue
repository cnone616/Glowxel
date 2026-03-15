<template>
  <view class="followers-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 导航栏 -->
    <view class="navbar">
      <view class="nav-left" @click="goBack">
        <Icon name="direction-left" :size="32" color="var(--color-text-primary)" />
      </view>
      <text class="nav-title">粉丝</text>
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
          placeholder="搜索粉丝昵称..."
          focus
        />
        <view v-if="searchQuery" class="clear-btn" @click="searchQuery = ''">
          <Icon name="close" :size="24" />
        </view>
      </view>
    </view>
    
    <!-- 统计信息 -->
    <view class="stats-section">
      <view class="stat-item">
        <text class="stat-number">{{ totalFollowers }}</text>
        <text class="stat-label">粉丝</text>
      </view>
      <view class="stat-divider"></view>
      <view class="stat-item">
        <text class="stat-number">{{ mutualFollows }}</text>
        <text class="stat-label">互相关注</text>
      </view>
      <view class="stat-divider"></view>
      <view class="stat-item">
        <text class="stat-number">{{ recentFollows }}</text>
        <text class="stat-label">本周新增</text>
      </view>
    </view>
    
    <!-- 粉丝列表 -->
    <scroll-view scroll-y class="content" @scrolltolower="loadMore">
      <view v-if="filteredFollowers.length === 0" class="empty-state">
        <Icon name="users" :size="120" color="var(--color-text-disabled)" />
        <text class="empty-title">{{ searchQuery ? '未找到相关粉丝' : '暂无粉丝' }}</text>
        <text class="empty-desc">{{ searchQuery ? '试试其他关键词' : '分享你的作品来获得更多粉丝吧' }}</text>
      </view>
      
      <view v-else class="followers-list">
        <view 
          v-for="follower in filteredFollowers"
          :key="follower.id"
          class="follower-item"
          @click="goToUserProfile(follower)"
        >
          <!-- 头像 -->
          <view class="follower-avatar">
            <image 
              v-if="follower.avatar"
              :src="follower.avatar"
              class="avatar-image"
              mode="aspectFill"
            />
            <view v-else class="avatar-placeholder">
              <Icon name="user" :size="60" color="var(--color-text-disabled)" />
            </view>
            
            <!-- 在线状态 -->
            <view v-if="follower.isOnline" class="online-indicator"></view>
          </view>
          
          <!-- 用户信息 -->
          <view class="follower-info">
            <view class="user-name-row">
              <text class="follower-name">{{ follower.nickname }}</text>
              <view v-if="follower.isMutual" class="mutual-badge">
                <Icon name="heart" :size="20" />
                <text class="mutual-text">互关</text>
              </view>
            </view>
            
            <text class="follower-bio">{{ follower.bio || '这个人很懒，什么都没写' }}</text>
            
            <view class="follower-meta">
              <text class="follow-time">{{ formatFollowTime(follower.followTime) }}</text>
              <text class="works-count">{{ follower.worksCount }} 作品</text>
            </view>
          </view>
          
          <!-- 操作按钮 -->
          <view class="follower-actions">
            <button 
              v-if="!follower.isFollowing"
              class="follow-btn"
              @click.stop="followUser(follower)"
            >
              <Icon name="plus" :size="24" />
              <text>关注</text>
            </button>
            
            <button 
              v-else
              class="following-btn"
              @click.stop="unfollowUser(follower)"
            >
              <Icon name="check" :size="24" />
              <text>已关注</text>
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
      isLoading: false,
      hasMore: true,
      currentPage: 1,
      pageSize: 20,
      followers: [
        // 模拟数据
        {
          id: '1',
          nickname: '像素艺术家',
          avatar: '',
          bio: '专注像素艺术创作，分享美好作品',
          followTime: Date.now() - 86400000,
          worksCount: 25,
          isFollowing: false,
          isMutual: false,
          isOnline: true
        },
        {
          id: '2',
          nickname: '创意设计师',
          avatar: '',
          bio: '设计改变世界，创意点亮生活',
          followTime: Date.now() - 172800000,
          worksCount: 42,
          isFollowing: true,
          isMutual: true,
          isOnline: false
        },
        {
          id: '3',
          nickname: '拼豆爱好者',
          avatar: '',
          bio: '用拼豆记录生活的美好瞬间',
          followTime: Date.now() - 259200000,
          worksCount: 18,
          isFollowing: false,
          isMutual: false,
          isOnline: true
        },
        {
          id: '4',
          nickname: '游戏玩家',
          avatar: '',
          bio: '',
          followTime: Date.now() - 345600000,
          worksCount: 8,
          isFollowing: true,
          isMutual: false,
          isOnline: false
        },
        {
          id: '5',
          nickname: '艺术学生',
          avatar: '',
          bio: '正在学习数字艺术，希望能创作出更好的作品',
          followTime: Date.now() - 432000000,
          worksCount: 12,
          isFollowing: false,
          isMutual: false,
          isOnline: true
        }
      ]
    }
  },
  
  computed: {
    filteredFollowers() {
      if (!this.searchQuery.trim()) {
        return this.followers
      }
      
      const query = this.searchQuery.toLowerCase()
      return this.followers.filter(follower => 
        follower.nickname.toLowerCase().includes(query) ||
        (follower.bio && follower.bio.toLowerCase().includes(query))
      )
    },
    
    totalFollowers() {
      return this.followers.length
    },
    
    mutualFollows() {
      return this.followers.filter(f => f.isMutual).length
    },
    
    recentFollows() {
      const weekAgo = Date.now() - 7 * 24 * 60 * 60 * 1000
      return this.followers.filter(f => f.followTime > weekAgo).length
    }
  },
  
  onLoad() {
    this.toast = useToast()
    
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
    
    goToUserProfile(user) {
      uni.navigateTo({
        url: `/pages/user-detail/user-detail?id=${user.id}`
      })
    },
    
    async followUser(user) {
      try {
        // 模拟关注API调用
        await new Promise(resolve => setTimeout(resolve, 500))
        
        user.isFollowing = true
        this.toast.showSuccess(`已关注 ${user.nickname}`)
        
        // 触觉反馈
        uni.vibrateShort()
      } catch (error) {
        this.toast.showError('关注失败，请重试')
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
              
              user.isFollowing = false
              user.isMutual = false
              this.toast.showInfo(`已取消关注 ${user.nickname}`)
            } catch (error) {
              this.toast.showError('操作失败，请重试')
            }
          }
        }
      })
    },
    

    
    async loadMore() {
      if (this.isLoading || !this.hasMore) return
      
      this.isLoading = true
      
      try {
        // 模拟加载更多数据
        await new Promise(resolve => setTimeout(resolve, 1000))
        
        // 这里应该是实际的API调用
        // const newFollowers = await api.getFollowers(this.currentPage + 1)
        
        // 模拟没有更多数据
        this.hasMore = false
        this.toast.showInfo('已加载全部粉丝')
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
      
      if (diff < 60000) return '刚刚关注'
      if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前关注`
      if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前关注`
      if (diff < 604800000) return `${Math.floor(diff / 86400000)}天前关注`
      
      return date.toLocaleDateString() + ' 关注'
    }
  }
}
</script>

<style scoped>
.followers-page {
  height: 100vh;
  background-color: var(--color-app-background);
  display: flex;
  flex-direction: column;
}

.navbar {
  height: 88rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 32rpx;
  background-color: var(--color-card-background);
  border-bottom: 2rpx solid var(--border-primary);
  position: relative;
}

.nav-left {
  position: absolute;
  left: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
  justify-content: flex-start;
}

.nav-right {
  position: absolute;
  right: 32rpx;
  width: 80rpx;
  display: flex;
  align-items: center;
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

.stats-section {
  display: flex;
  align-items: center;
  background-color: var(--color-card-background);
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.stat-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.stat-number {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.stat-label {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.stat-divider {
  width: 2rpx;
  height: 60rpx;
  background-color: var(--border-primary);
  margin: 0 32rpx;
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

.followers-list {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.follower-item {
  display: flex;
  align-items: center;
  gap: 24rpx;
  background-color: var(--color-card-background);
  border-radius: 16rpx;
  padding: 32rpx;
  box-shadow: var(--shadow-card);
  transition: all 0.2s ease;
}

.follower-item:active {
  transform: scale(0.98);
  box-shadow: var(--shadow-floating);
}

.follower-avatar {
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

.follower-info {
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

.follower-name {
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

.follower-bio {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.follower-meta {
  display: flex;
  gap: 16rpx;
}

.follow-time, .works-count {
  font-size: 22rpx;
  color: var(--color-text-disabled);
}

.follower-actions {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
  align-items: center;
}

.follow-btn, .following-btn {
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

.follow-btn {
  background-color: var(--color-brand-primary);
  color: #FFFFFF;
}

.following-btn {
  background-color: var(--color-app-background);
  color: var(--color-text-secondary);
  border: 2rpx solid var(--border-primary);
}



.follow-btn::after, .following-btn::after {
  border: none;
}

.follow-btn:active {
  background-color: var(--accent-hover);
  transform: scale(0.95);
}

.following-btn:active {
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
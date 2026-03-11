<template>
  <view class="profile-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    <scroll-view scroll-y class="content">
      <!-- 用户信息卡片 -->
      <view class="user-card">
        <view v-if="!userStore.hasLogin" class="login-prompt">
          <view class="avatar-placeholder">
            <Icon name="user" :size="80" />
          </view>
          <text class="prompt-text">登录后可同步数据到云端</text>
          <button class="login-btn" @click="handleLogin">
            <Icon name="user" :size="32" />
            <text>微信登录</text>
          </button>
        </view>
        
        <view v-else class="user-info">
          <view class="user-avatar">
            <image 
              v-if="userStore.avatar" 
              :src="userStore.avatar" 
              class="avatar"
              mode="aspectFill"
            />
            <view v-else class="avatar-placeholder">
              <Icon name="user" :size="80" />
            </view>
          </view>
          
          <view class="user-details">
            <view class="user-name-row">
              <text class="nickname">{{ userStore.nickname || '陈晓明' }}</text>
              <button class="edit-btn" @click="handleEditProfile">
                <text>编辑</text>
              </button>
            </view>
            <text class="user-id">@{{ userStore.openid ? userStore.openid.slice(0, 8) : 'alexchen' }}</text>
          </view>
        </view>
        
        <!-- 统计数据 -->
        <view v-if="userStore.hasLogin" class="user-stats">
          <view class="stat-item" @click="goToMyWorks">
            <text class="stat-number">{{ projectStore.projects.length }}</text>
            <text class="stat-label">作品</text>
          </view>
          <view class="stat-item" @click="goToFollowers">
            <text class="stat-number">1.2K</text>
            <text class="stat-label">粉丝</text>
          </view>
          <view class="stat-item" @click="goToFollowing">
            <text class="stat-number">342</text>
            <text class="stat-label">关注</text>
          </view>
        </view>
      </view>

      <!-- 功能菜单 -->
      <view class="menu-section">
        <!-- 我的作品 -->
        <view class="menu-item" @click="goToMyWorks">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="picture" :size="40" color="#4F7FFF" />
            </view>
            <text class="menu-label">我的作品</text>
          </view>
          <view class="menu-right">
            <text class="menu-count">{{ projectStore.projects.length }}</text>
            <Icon name="arrow-right" :size="32" color="#AAAAAA" />
          </view>
        </view>

        <!-- 我的收藏 -->
        <view class="menu-item" @click="goToMyFavorites">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="bookmark" :size="40" color="#4F7FFF" />
            </view>
            <text class="menu-label">我的收藏</text>
          </view>
          <view class="menu-right">
            <text class="menu-count">12</text>
            <Icon name="arrow-right" :size="32" color="#AAAAAA" />
          </view>
        </view>

        <!-- 我的设备 -->
        <view class="menu-item" @click="goToMyDevices">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="link" :size="40" color="#4F7FFF" />
            </view>
            <text class="menu-label">我的设备</text>
          </view>
          <view class="menu-right">
            <text class="menu-count">3</text>
            <Icon name="arrow-right" :size="32" color="#AAAAAA" />
          </view>
        </view>

        <!-- 成就 -->
        <view class="menu-item" @click="goToAchievements">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="award" :size="40" color="#4F7FFF" />
            </view>
            <text class="menu-label">成就</text>
          </view>
          <view class="menu-right">
            <text class="menu-count">8</text>
            <Icon name="arrow-right" :size="32" color="#AAAAAA" />
          </view>
        </view>

        <!-- 云同步设置 -->
        <view v-if="userStore.hasLogin" class="menu-item" @click="goToCloudSync">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="cloud" :size="40" color="#4F7FFF" />
            </view>
            <view class="menu-info">
              <text class="menu-label">自动同步</text>
              <text class="menu-desc">自动同步项目到云端</text>
            </view>
          </view>
          <view class="menu-right">
            <switch 
              :checked="userStore.syncEnabled" 
              @change="handleSyncChange"
              color="#4F7FFF"
            />
          </view>
        </view>

        <!-- 设置 -->
        <view class="menu-item" @click="goToSettings">
          <view class="menu-left">
            <view class="menu-icon">
              <Icon name="settings" :size="40" color="#4F7FFF" />
            </view>
            <text class="menu-label">设置</text>
          </view>
          <view class="menu-right">
            <Icon name="arrow-right" :size="32" color="#AAAAAA" />
          </view>
        </view>
      </view>

      <!-- 退出登录 -->
      <view v-if="userStore.hasLogin" class="logout-section">
        <button class="logout-btn" @click="handleLogout">
          <Icon name="sign-out" :size="32" />
          <text>退出登录</text>
        </button>
      </view>
    </scroll-view>
    
    <!-- Toast -->
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useUserStore } from '../../store/user.js'
import { useProjectStore } from '../../store/project.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'
import { MockAPI } from '../../data/mock/index.js'
import { 
  getCurrentUser, 
  getProfileStats, 
  getMyFavorites, 
  getAchievementStats 
} from '../../data/mock/profile.js'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast
  },
  
  data() {
    // 直接初始化 store
    const userStore = useUserStore()
    const projectStore = useProjectStore()
    
    return {
      userStore,
      projectStore,
      toast: null
    }
  },

  onLoad() {
    this.toast = useToast()
    
    // 注册 Toast
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
  },

  methods: {
    // 登录
    async handleLogin() {
      uni.showLoading({ title: '登录中...' })
      
      const res = await this.userStore.login()
      
      uni.hideLoading()
      
      if (res.success) {
        this.toast.showSuccess('登录成功')
      } else {
        this.toast.showError(res.error || '登录失败')
      }
    },

    // 编辑个人资料
    handleEditProfile() {
      uni.navigateTo({
        url: '/pages/edit-profile/edit-profile'
      })
    },

    // 跳转到我的作品
    goToMyWorks() {
      uni.navigateTo({
        url: '/pages/my-works/my-works'
      })
    },

    // 跳转到我的收藏
    goToMyFavorites() {
      uni.navigateTo({
        url: '/pages/my-favorites/my-favorites'
      })
    },

    // 跳转到我的设备
    goToMyDevices() {
      uni.navigateTo({
        url: '/pages/control/control'
      })
    },

    // 跳转到成就页面
    goToAchievements() {
      uni.navigateTo({
        url: '/pages/achievements/achievements'
      })
    },

    // 跳转到粉丝页面
    goToFollowers() {
      uni.navigateTo({
        url: '/pages/followers/followers'
      })
    },

    // 跳转到关注页面
    goToFollowing() {
      uni.navigateTo({
        url: '/pages/following/following'
      })
    },

    // 跳转到云同步设置
    goToCloudSync() {
      uni.navigateTo({
        url: '/pages/cloud-sync/cloud-sync'
      })
    },

    // 跳转到设置页面
    goToSettings() {
      uni.navigateTo({
        url: '/pages/settings/settings'
      })
    },

    // 切换同步状态
    handleSyncChange(e) {
      if (e.detail.value) {
        this.userStore.enableSync()
        this.toast.showSuccess('已开启云同步')
      } else {
        this.userStore.disableSync()
        this.toast.showInfo('已关闭云同步')
      }
    },

    // 退出登录
    handleLogout() {
      uni.showModal({
        title: '确认退出',
        content: '退出后将无法同步数据到云端',
        success: (res) => {
          if (res.confirm) {
            this.userStore.logout()
            this.toast.showSuccess('已退出登录')
          }
        }
      })
    }
  }
}
</script>

<style scoped>
.profile-page {
  height: 100vh;
  background-color: var(--color-app-background);
  overflow: hidden;
}

/* 页面标题 */
.page-header {
  background-color: var(--color-card-background);
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.page-title {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.content {
  flex: 1;
  overflow-y: auto;
  padding-bottom: 80rpx;
  height: 100%;
}

/* 用户卡片 */
.user-card {
  background-color: var(--color-card-background);
  border-radius: 24rpx;
  padding: 32rpx;
  margin: 32rpx 20rpx;
  box-shadow: var(--shadow-md);
}

.login-prompt {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
}

.avatar-placeholder {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
  background-color: var(--color-app-background);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-secondary);
}

.prompt-text {
  font-size: 28rpx;
  color: var(--color-text-secondary);
  text-align: center;
}

.login-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 48rpx;
  background: linear-gradient(135deg, #4F7FFF 0%, #00f3ff 100%);
  color: #FFFFFF;
  border: none;
  border-radius: 16rpx;
  font-size: 28rpx;
  font-weight: 600;
  box-shadow: var(--shadow-md);
}

.login-btn::after {
  border: none;
}

.user-info {
  display: flex;
  align-items: flex-start;
  gap: 24rpx;
  margin-bottom: 32rpx;
}

.user-avatar .avatar {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
}

.user-details {
  flex: 1;
}

.user-name-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 8rpx;
}

.nickname {
  font-size: 36rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.edit-btn {
  padding: 12rpx 24rpx;
  background-color: transparent;
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

.edit-btn::after {
  border: none;
}

.user-id {
  font-size: 24rpx;
  color: var(--color-text-secondary);
  font-family: monospace;
}

/* 统计数据 */
.user-stats {
  display: flex;
  justify-content: space-around;
  padding-top: 24rpx;
  border-top: 2rpx solid var(--border-primary);
}

.stat-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  cursor: pointer;
  transition: all 0.2s ease;
}

.stat-item:active {
  transform: scale(0.95);
}

.stat-number {
  font-size: 32rpx;
  font-weight: 700;
  color: var(--color-text-primary);
}

.stat-label {
  font-size: 24rpx;
  color: var(--color-text-secondary);
}

/* 功能菜单 */
.menu-section {
  background-color: var(--color-card-background);
  border-radius: 24rpx;
  margin: 32rpx 20rpx;
  overflow: hidden;
  box-shadow: var(--shadow-md);
}

.menu-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  transition: all 0.2s ease;
}

.menu-item:last-child {
  border-bottom: none;
}

.menu-item:active {
  background-color: var(--color-app-background);
}

.menu-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
}

.menu-icon {
  width: 80rpx;
  height: 80rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(79, 127, 255, 0.1);
  border-radius: 16rpx;
}

.menu-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.menu-label {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--color-text-primary);
}

.menu-desc {
  font-size: 22rpx;
  color: var(--color-text-secondary);
}

.menu-right {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.menu-count {
  font-size: 24rpx;
  font-weight: 600;
  color: var(--color-brand-primary);
  background-color: rgba(79, 127, 255, 0.1);
  padding: 8rpx 16rpx;
  border-radius: 20rpx;
  min-width: 40rpx;
  text-align: center;
}

/* 退出登录 */
.logout-section {
  margin: 32rpx 20rpx;
}

.logout-btn {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  padding: 32rpx;
  background-color: var(--color-card-background);
  border: 2rpx solid rgba(239, 68, 68, 0.3);
  border-radius: 24rpx;
  color: #EF4444;
  font-size: 28rpx;
  font-weight: 600;
  transition: all 0.2s ease;
}

.logout-btn::after {
  border: none;
}

.logout-btn:active {
  background-color: rgba(239, 68, 68, 0.05);
  transform: scale(0.98);
}
</style>

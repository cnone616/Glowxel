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
          <image 
            v-if="userStore.avatar" 
            :src="userStore.avatar" 
            class="avatar"
            mode="aspectFill"
          />
          <view v-else class="avatar-placeholder">
            <Icon name="user" :size="80" />
          </view>
          
          <view class="info-content">
            <text class="nickname">{{ userStore.nickname }}</text>
            <text class="openid">ID: {{ userStore.openid.slice(0, 8) }}...</text>
          </view>
          
          <button 
            v-if="!userStore.userInfo" 
            class="get-info-btn" 
            @click="handleGetUserInfo"
          >
            完善资料
          </button>
        </view>
      </view>

      <!-- 云同步设置 -->
      <view v-if="userStore.hasLogin" class="section">
        <text class="section-title">云同步</text>
        
        <view class="setting-card">
          <view class="setting-row">
            <view class="setting-left">
              <view class="setting-icon">
                <Icon name="upload" :size="40" />
              </view>
              <view class="setting-info">
                <text class="setting-label">自动同步</text>
                <text class="setting-desc">自动同步项目到云端</text>
              </view>
            </view>
            <switch 
              :checked="userStore.syncEnabled" 
              @change="handleSyncChange"
              color="#4F7FFF"
            />
          </view>
          
          <view v-if="userStore.syncEnabled" class="sync-actions">
            <button class="action-btn" @click="handleSyncNow">
              <Icon name="refresh" :size="28" />
              <text>立即同步</text>
            </button>
            <button class="action-btn" @click="handlePullFromCloud">
              <Icon name="download" :size="28" />
              <text>从云端拉取</text>
            </button>
          </view>
        </view>
      </view>

      <!-- 数据统计 -->
      <view class="section">
        <text class="section-title">数据统计</text>
        
        <view class="stats-grid">
          <view class="stat-item">
            <text class="stat-value">{{ projectStore.projects.length }}</text>
            <text class="stat-label">本地项目</text>
          </view>
          <view class="stat-item">
            <text class="stat-value">{{ cloudProjectCount }}</text>
            <text class="stat-label">云端项目</text>
          </view>
          <view class="stat-item">
            <text class="stat-value">{{ storageSize }}</text>
            <text class="stat-label">存储空间</text>
          </view>
        </view>
      </view>

      <!-- 账号操作 -->
      <view v-if="userStore.hasLogin" class="section">
        <text class="section-title">账号</text>
        
        <view class="setting-card">
          <button class="logout-btn" @click="handleLogout">
            <Icon name="sign-out" :size="32" />
            <text>退出登录</text>
          </button>
        </view>
      </view>
    </scroll-view>
    
    <!-- 自定义底部导航栏 -->
    <CustomTabBar />
    
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
import CustomTabBar from '../../components/CustomTabBar.vue'
import Toast from '../../components/Toast.vue'
import projectAPI from '../../api/project.js'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    CustomTabBar,
    Toast
  },
  
  data() {
    // 直接初始化 store
    const userStore = useUserStore()
    const projectStore = useProjectStore()
    
    return {
      userStore,
      projectStore,
      toast: null,
      cloudProjectCount: 0,
      storageSize: '0 KB'
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
    
    // 加载统计数据
    this.loadStats()
  },

  methods: {
    // 登录
    async handleLogin() {
      uni.showLoading({ title: '登录中...' })
      
      const res = await this.userStore.login()
      
      uni.hideLoading()
      
      if (res.success) {
        this.toast.showSuccess('登录成功')
        this.loadStats()
      } else {
        this.toast.showError(res.error || '登录失败')
      }
    },

    // 获取用户信息
    async handleGetUserInfo() {
      const res = await this.userStore.getUserProfile()
      
      if (res.success) {
        this.toast.showSuccess('资料已完善')
      } else {
        this.toast.showError(res.error || '获取失败')
      }
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

    // 立即同步
    async handleSyncNow() {
      if (!this.userStore.hasLogin) {
        this.toast.showError('请先登录')
        return
      }
      
      uni.showLoading({ title: '同步中...' })
      
      try {
        const projects = this.projectStore.projects
        
        for (const project of projects) {
          await projectAPI.createProject({
            ...project,
            userId: this.userStore.openid
          })
        }
        
        uni.hideLoading()
        this.toast.showSuccess('同步完成')
        this.loadStats()
      } catch (error) {
        uni.hideLoading()
        this.toast.showError('同步失败')
      }
    },

    // 从云端拉取
    async handlePullFromCloud() {
      if (!this.userStore.hasLogin) {
        this.toast.showError('请先登录')
        return
      }
      
      uni.showModal({
        title: '确认拉取',
        content: '从云端拉取数据会覆盖本地数据，是否继续？',
        success: async (res) => {
          if (res.confirm) {
            uni.showLoading({ title: '拉取中...' })
            
            const result = await projectAPI.getUserProjects()
            
            uni.hideLoading()
            
            if (result.success) {
              // 覆盖本地数据
              this.projectStore.projects = result.data
              this.projectStore.saveToStorage()
              this.toast.showSuccess('拉取完成')
            } else {
              this.toast.showError('拉取失败')
            }
          }
        }
      })
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
            this.cloudProjectCount = 0
          }
        }
      })
    },

    // 加载统计数据
    async loadStats() {
      // 计算本地存储大小
      try {
        const info = uni.getStorageInfoSync()
        const sizeKB = info.currentSize
        this.storageSize = sizeKB < 1024 
          ? `${sizeKB} KB` 
          : `${(sizeKB / 1024).toFixed(2)} MB`
      } catch (e) {
        this.storageSize = '0 KB'
      }
      
      // 获取云端项目数量
      if (this.userStore.hasLogin) {
        const res = await projectAPI.getUserProjects()
        if (res.success) {
          this.cloudProjectCount = res.data.length
        }
      }
    }
  }
}
</script>

<style scoped>
.profile-page {
  height: 100vh;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.content {
  height: 100%;
  padding: 32rpx;
  padding-bottom: calc(160rpx + env(safe-area-inset-bottom));
}

/* 用户卡片 */
.user-card {
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  padding: 32rpx;
  border: 2rpx solid var(--border-primary);
  margin-bottom: 32rpx;
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
  background-color: var(--bg-secondary);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
}

.prompt-text {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.login-btn {
  display: flex;
  align-items: center;
  gap: 12rpx;
  padding: 20rpx 40rpx;
  background: linear-gradient(135deg, #00f3ff 0%, #00a8ff 100%);
  color: #000;
  border: none;
  border-radius: 12rpx;
  font-size: 28rpx;
  font-weight: 500;
}

.login-btn::after {
  border: none;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 24rpx;
}

.avatar {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
}

.info-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.nickname {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.openid {
  font-size: 22rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.get-info-btn {
  padding: 16rpx 24rpx;
  background-color: var(--accent-primary);
  color: #000;
  border: none;
  border-radius: 8rpx;
  font-size: 24rpx;
}

.get-info-btn::after {
  border: none;
}

/* 区块 */
.section {
  margin-bottom: 32rpx;
}

.section-title {
  font-size: 22rpx;
  font-weight: 600;
  color: var(--accent-primary);
  text-transform: uppercase;
  letter-spacing: 2rpx;
  margin-bottom: 16rpx;
  display: block;
}

/* 设置卡片 */
.setting-card {
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  padding: 24rpx;
  border: 2rpx solid var(--border-primary);
}

.setting-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.setting-left {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.setting-icon {
  color: var(--accent-primary);
}

.setting-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.setting-label {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.setting-desc {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.sync-actions {
  display: flex;
  gap: 16rpx;
  margin-top: 24rpx;
  padding-top: 24rpx;
  border-top: 2rpx solid var(--border-primary);
}

.action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 20rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 12rpx;
  font-size: 24rpx;
  color: var(--text-primary);
}

.action-btn::after {
  border: none;
}

/* 统计网格 */
.stats-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16rpx;
}

.stat-item {
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
  padding: 24rpx;
  border: 2rpx solid var(--border-primary);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
}

.stat-value {
  font-size: 36rpx;
  font-weight: bold;
  color: var(--accent-primary);
  font-family: monospace;
}

.stat-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

/* 退出按钮 */
.logout-btn {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 20rpx;
  background-color: rgba(239, 68, 68, 0.1);
  border: 2rpx solid rgba(239, 68, 68, 0.3);
  border-radius: 12rpx;
  color: var(--error-color);
  font-size: 26rpx;
  font-weight: 500;
}

.logout-btn::after {
  border: none;
}
</style>

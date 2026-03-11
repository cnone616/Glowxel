<template>
  <view class="settings-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="settings-container">
        <!-- 用户信息卡片 -->
        <view class="user-card">
          <view v-if="!userStore.hasLogin" class="login-prompt">
            <view class="avatar-placeholder">
              <Icon name="user" :size="80" />
            </view>
            <text class="prompt-text">登录后可同步数据到云端</text>
            <button class="login-btn" @click="showLoginModal = true">
              <Icon name="user" :size="32" />
              <text>微信登录</text>
            </button>
          </view>
          
          <view v-else class="user-info">
            <!-- #ifdef MP-WEIXIN -->
            <button 
              class="avatar-btn" 
              open-type="chooseAvatar" 
              @chooseavatar="handleChooseAvatar"
            >
              <image 
                v-if="userStore.avatar" 
                :src="userStore.avatar" 
                class="avatar"
                mode="aspectFill"
              />
              <view v-else class="avatar-placeholder">
                <Icon name="user" :size="80" />
              </view>
            </button>
            <!-- #endif -->
            <!-- #ifndef MP-WEIXIN -->
            <view class="avatar-placeholder">
              <Icon name="user" :size="80" />
            </view>
            <!-- #endif -->
            
            <view class="info-content">
              <view class="nickname-input-wrapper">
                <!-- #ifdef MP-WEIXIN -->
                <input 
                  type="nickname" 
                  class="nickname-input"
                  :value="userStore.nickname"
                  @blur="handleNicknameChange"
                  placeholder="点击设置昵称"
                />
                <!-- #endif -->
                <!-- #ifndef MP-WEIXIN -->
                <text class="nickname">{{ userStore.nickname }}</text>
                <!-- #endif -->
              </view>
              <text class="status-text">已登录</text>
            </view>
          </view>
        </view>

        <!-- 云同步设置 - 仅登录后显示 -->
        <view v-if="userStore.hasLogin" class="section">
          <text class="section-title">云同步</text>
          
          <view class="setting-card">
            <view class="setting-row">
              <view class="setting-left">
                <view class="setting-icon">
                  <Icon name="switch" :size="40" />
                </view>
                <view class="setting-info">
                  <text class="setting-label">自动同步</text>
                  <text class="setting-desc">自动同步项目到云端</text>
                </view>
              </view>
              <switch 
                :checked="userStore.syncEnabled" 
                @change="handleSyncChange"
                color="#00f3ff"
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

        <!-- 数据统计 - 仅登录后显示 -->
        <view v-if="userStore.hasLogin" class="section">
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

        <!-- 我的设备 -->
        <view class="section">
          <text class="section-title">我的设备</text>
          
          <view class="setting-card clickable" @click="goToDeviceControl">
            <view class="setting-row">
              <view class="setting-left">
                <view class="setting-icon">
                  <Icon name="link" :size="40" />
                </view>
                <view class="setting-info">
                  <text class="setting-label">LED 矩阵板</text>
                  <text class="setting-desc">连接和控制您的设备</text>
                </view>
              </view>
              <Icon name="arrow-right" :size="32" color="var(--text-tertiary)" />
            </view>
          </view>
        </view>

        <!-- 数据与存储 -->
        <view class="section">
          <text class="section-title">数据与存储</text>
          
          <view class="setting-card">
            <view class="setting-header">
              <view class="setting-icon">
                <Icon name="file-common" :size="40" />
              </view>
              <view class="setting-info">
                <text class="setting-label">本地存储</text>
                <text class="setting-desc">管理您的设备数据</text>
              </view>
            </view>
            
            <view class="danger-btn" @click="handleClearData">
              <Icon name="ashbin" :size="32" />
              <text class="danger-text">清除所有数据</text>
            </view>
          </view>
        </view>

        <!-- 账号操作 -->
        <view v-if="userStore.hasLogin" class="section">
          <text class="section-title">账号</text>
          
          <view class="setting-card">
            <button class="logout-btn" @click="handleLogout">
              <Icon name="exit" :size="32" />
              <text>退出登录</text>
            </button>
          </view>
        </view>

        <!-- 关于 -->
        <view class="section">
          <text class="section-title">关于</text>
          
          <view class="about-card">
            <view class="about-header">
              <view class="logo-wrapper">
                <Logo :size="64" :showText="true" />
              </view>
              <view class="version-badge">
                <text class="version-text">v{{ appVersion }}</text>
              </view>
            </view>
            <text class="about-desc">
              专为 52x52 LED 矩阵板设计的像素艺术布局工具。
              针对移动端优化，支持离线存储。
            </text>
          </view>
        </view>
      </view>
    </scroll-view>
    
    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />
    
    <!-- 确认弹窗 -->
    <ConfirmModal
      :visible.sync="showSyncModal"
      title="选择同步方式"
      content="完全同步会删除云端多余的项目（本地没有的），普通同步只上传本地项目"
      confirmText="完全同步"
      cancelText="普通同步"
      @confirm="performFullSync"
      @cancel="performNormalSync"
    />
    
    <ConfirmModal
      :visible.sync="showPullModal"
      title="确认拉取"
      content="从云端拉取数据会覆盖本地数据，是否继续？"
      confirmText="确认"
      cancelText="取消"
      @confirm="confirmPullFromCloud"
    />
    
    <ConfirmModal
      :visible.sync="showLogoutModal"
      title="确认退出"
      content="退出后将无法同步数据到云端"
      confirmText="确认退出"
      cancelText="取消"
      danger
      @confirm="confirmLogout"
    />
    
    <ConfirmModal
      :visible.sync="showClearModal"
      title="确认清除"
      content="您确定要清除所有数据吗？此操作无法撤销。"
      confirmText="清除"
      cancelText="取消"
      danger
      @confirm="confirmClearData"
    />
    
    <!-- 登录弹窗 -->
    <view v-if="showLoginModal" class="login-modal" @tap="closeLoginModal">
      <view class="modal-content" @tap.stop>
        <view class="modal-header">
          <text class="modal-title">完善个人信息</text>
          <view class="close-btn" @tap="closeLoginModal">
            <Icon name="close" :size="32" />
          </view>
        </view>
        
        <view class="modal-body">
          <!-- 头像选择 -->
          <view class="form-item">
            <text class="form-label">头像</text>
            <!-- #ifdef MP-WEIXIN -->
            <button 
              class="avatar-picker" 
              open-type="chooseAvatar" 
              @chooseavatar="handleChooseAvatar"
            >
              <image 
                v-if="tempUserInfo.avatarUrl" 
                :src="tempUserInfo.avatarUrl" 
                class="temp-avatar"
                mode="aspectFill"
              />
              <view v-else class="temp-avatar-placeholder">
                <Icon name="camera" :size="50" style="height: 90rpx"/>
                <text class="picker-hint">点击选择头像</text>
              </view>
            </button>
            <!-- #endif -->
          </view>
          
          <!-- 昵称输入 -->
          <view class="form-item">
            <text class="form-label">昵称</text>
            <!-- #ifdef MP-WEIXIN -->
            <input 
              type="nickname" 
              class="nickname-picker"
              v-model="tempUserInfo.nickName"
              placeholder="请输入昵称"
              placeholder-class="input-placeholder"
            />
            <!-- #endif -->
          </view>
          
          <view class="form-tips">
            <text class="tips-text">• 头像和昵称用于个人资料展示</text>
            <text class="tips-text">• 可以随时在个人中心修改</text>
          </view>
        </view>
        
        <view class="modal-footer">
          <button class="confirm-btn" @tap="handleConfirmLogin">
            <text>完成登录</text>
          </button>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
import { useUserStore } from '../../store/user.js'
import { useProjectStore } from '../../store/project.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Logo from '../../components/Logo.vue'
import Toast from '../../components/Toast.vue'
import ConfirmModal from '../../components/ConfirmModal.vue'
import projectAPI from '../../api/project.js'
import packageInfo from '../../package.json'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Logo,
    Toast,
    ConfirmModal
  },
  data() {
    const userStore = useUserStore()
    const projectStore = useProjectStore()
    
    return {
      userStore,
      projectStore,
      toast: null,
      cloudProjectCount: 0,
      storageSize: '0 KB',
      contentHeight: 'calc(100vh - 128rpx)',
      showLoginModal: false,
      tempUserInfo: {
        nickName: '',
        avatarUrl: ''
      },
      appVersion: packageInfo.version,
      // 确认弹窗状态
      showSyncModal: false,
      showPullModal: false,
      showLogoutModal: false,
      showClearModal: false
    }
  },
  
  computed: {
    allThemes() {
      return []
    }
  },

  onLoad() {
    this.toast = useToast()
    
    // 注册自定义 Toast 实例
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
    
    // 计算内容高度（减去状态栏和底部导航栏）
    const systemInfo = uni.getSystemInfoSync()
    const statusBarHeight = systemInfo.statusBarHeight || 0
    const safeAreaBottom = systemInfo.safeArea?.bottom || systemInfo.screenHeight
    const screenHeight = systemInfo.screenHeight
    const safeAreaInsetBottom = screenHeight - safeAreaBottom
    
    // 状态栏 + 底部导航栏(128rpx = 64px) + 底部安全区
    const tabBarHeight = 64 + safeAreaInsetBottom
    this.contentHeight = `${screenHeight - statusBarHeight - tabBarHeight}px`
    
    console.log('计算高度:', {
      screenHeight,
      statusBarHeight,
      tabBarHeight,
      contentHeight: this.contentHeight
    })
    
    // 加载统计数据
    this.loadStats()
  },

  onShow() {
    this.loadStats()
  },

  methods: {
    // 跳转到设备控制页面
    goToDeviceControl() {
      uni.navigateTo({
        url: '/pages/control/control'
      })
    },
    
    // 打开登录弹窗
    async handleQuickLogin() {
      uni.showLoading({ title: '登录中...' })
      
      try {
        // #ifdef MP-WEIXIN
        // 调用 wx.login 获取登录凭证
        const loginRes = await new Promise((resolve, reject) => {
          uni.login({
            provider: 'weixin',
            success: resolve,
            fail: reject
          })
        })
        
        console.log('登录成功，code:', loginRes.code)
        
        // 检查本地是否已有用户信息
        const savedUserInfo = uni.getStorageSync('userInfo')
        
        if (savedUserInfo && savedUserInfo.nickName && savedUserInfo.nickName !== '微信用户') {
          // 已有用户信息，直接登录
          this.userStore.isLoggedIn = true
          this.userStore.openid = 'auto'
          this.userStore.userInfo = savedUserInfo
          
          // 更新登录时间
          const loginTime = Date.now()
          uni.setStorageSync('isLoggedIn', true)
          uni.setStorageSync('loginTime', loginTime)
          
          uni.hideLoading()
          this.toast.showSuccess('登录成功')
          this.loadStats()
        } else {
          // 首次登录，显示完善信息弹窗
          uni.hideLoading()
          
          // 重置临时用户信息
          this.tempUserInfo = {
            nickName: '',
            avatarUrl: ''
          }
          
          // 显示登录弹窗
          this.showLoginModal = true
        }
        // #endif
      } catch (error) {
        console.error('登录失败:', error)
        uni.hideLoading()
        this.toast.showError('登录失败')
      }
    },
    
    // 关闭登录弹窗
    closeLoginModal() {
      this.showLoginModal = false
    },
    
    // 确认登录
    async handleConfirmLogin() {
      // 检查是否输入了昵称
      if (!this.tempUserInfo.nickName || !this.tempUserInfo.nickName.trim()) {
        this.toast.showError('请输入昵称')
        return
      }
      
      // 保存用户信息
      this.userStore.isLoggedIn = true
      this.userStore.openid = 'auto'
      this.userStore.userInfo = {
        nickName: this.tempUserInfo.nickName.trim(),
        avatarUrl: this.tempUserInfo.avatarUrl
      }
      
      const loginTime = Date.now()
      uni.setStorageSync('isLoggedIn', true)
      uni.setStorageSync('userInfo', this.userStore.userInfo)
      uni.setStorageSync('loginTime', loginTime)
      
      // 同步到云端
      await this.userStore.syncUserInfo()
      
      // 关闭弹窗
      this.showLoginModal = false
      
      this.toast.showSuccess('登录成功')
      this.loadStats()
    },
    
    // 选择头像（登录弹窗中）
    handleChooseAvatar(e) {
      console.log('选择头像:', e)
      const avatarUrl = e.detail.avatarUrl
      
      if (avatarUrl) {
        if (this.showLoginModal) {
          // 登录流程中
          this.tempUserInfo.avatarUrl = avatarUrl
        } else {
          // 已登录，直接更新
          this.updateAvatar(avatarUrl)
        }
      }
    },
    
    // 更新头像（已登录用户）
    async updateAvatar(avatarUrl) {
      this.userStore.userInfo = {
        ...this.userStore.userInfo,
        avatarUrl: avatarUrl
      }
      
      uni.setStorageSync('userInfo', this.userStore.userInfo)
      await this.userStore.syncUserInfo()
      
      this.toast.showSuccess('头像已更新')
    },
    
    // 修改昵称（已登录用户）
    async handleNicknameChange(e) {
      const nickname = e.detail.value
      
      if (nickname && nickname.trim()) {
        this.userStore.userInfo = {
          ...this.userStore.userInfo,
          nickName: nickname.trim()
        }
        
        uni.setStorageSync('userInfo', this.userStore.userInfo)
        await this.userStore.syncUserInfo()
        
        this.toast.showSuccess('昵称已更新')
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
      
      // 显示同步方式选择弹窗
      this.showSyncModal = true
    },
    
    // 普通同步（只上传，不删除云端）
    async performNormalSync() {
      uni.showLoading({ title: '同步中...' })
      
      try {
        const projects = this.projectStore.projects
        
        // 先获取云端已有的项目
        const cloudRes = await projectAPI.getUserProjects()
        const cloudProjects = cloudRes.success ? cloudRes.data : []
        
        // 创建云端项目 ID 映射
        const cloudProjectMap = {}
        cloudProjects.forEach(p => {
          if (p.id) {
            cloudProjectMap[p.id] = p._id
          }
        })
        
        // 同步本地项目
        for (const project of projects) {
          const cloudId = cloudProjectMap[project.id]
          
          if (cloudId) {
            // 更新已存在的项目
            await projectAPI.updateProject(cloudId, project)
          } else {
            // 创建新项目
            await projectAPI.createProject(project)
          }
        }
        
        uni.hideLoading()
        this.toast.showSuccess('同步完成')
        this.loadStats()
      } catch (error) {
        uni.hideLoading()
        this.toast.showError('同步失败')
      }
    },
    
    // 完全同步（上传本地，删除云端多余的）
    async performFullSync() {
      uni.showLoading({ title: '完全同步中...' })
      
      try {
        const projects = this.projectStore.projects
        
        // 先获取云端已有的项目
        const cloudRes = await projectAPI.getUserProjects()
        const cloudProjects = cloudRes.success ? cloudRes.data : []
        
        // 创建本地项目 ID 集合
        const localProjectIds = new Set(projects.map(p => p.id))
        
        // 创建云端项目 ID 映射
        const cloudProjectMap = {}
        cloudProjects.forEach(p => {
          if (p.id) {
            cloudProjectMap[p.id] = p._id
          }
        })
        
        // 同步本地项目到云端
        for (const project of projects) {
          const cloudId = cloudProjectMap[project.id]
          
          if (cloudId) {
            // 更新已存在的项目
            await projectAPI.updateProject(cloudId, project)
          } else {
            // 创建新项目
            await projectAPI.createProject(project)
          }
        }
        
        // 删除云端多余的项目（本地没有的）
        for (const cloudProject of cloudProjects) {
          if (!localProjectIds.has(cloudProject.id)) {
            // 本地没有这个项目，删除云端的
            await projectAPI.deleteProject(cloudProject._id)
          }
        }
        
        uni.hideLoading()
        this.toast.showSuccess('完全同步完成')
        this.loadStats()
      } catch (error) {
        uni.hideLoading()
        console.error('完全同步失败:', error)
        this.toast.showError('同步失败')
      }
    },

    // 从云端拉取
    async handlePullFromCloud() {
      if (!this.userStore.hasLogin) {
        this.toast.showError('请先登录')
        return
      }
      
      this.showPullModal = true
    },
    
    // 确认从云端拉取
    async confirmPullFromCloud() {
      uni.showLoading({ title: '拉取中...' })
      
      const result = await projectAPI.getUserProjects()
      
      uni.hideLoading()
      
      if (result.success) {
        this.projectStore.projects = result.data
        this.projectStore.saveProjects()
        this.toast.showSuccess('拉取完成')
      } else {
        this.toast.showError('拉取失败')
      }
    },

    // 退出登录
    handleLogout() {
      this.showLogoutModal = true
    },
    
    // 确认退出登录
    confirmLogout() {
      this.userStore.logout()
      this.toast.showSuccess('已退出登录')
      this.cloudProjectCount = 0
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
    },

    goToProfile() {
      uni.navigateTo({
        url: '/pages/profile/profile'
      })
    },

    handleThemeChange(themeId) {
      // 主题切换已禁用，统一使用浅色主题
      console.log('主题切换已禁用')
    },

    handleClearData() {
      this.showClearModal = true
    },
    
    // 确认清除数据
    confirmClearData() {
      // 清除所有存储
      uni.clearStorageSync()
      
      this.toast.showSuccess('所有数据已清除')
      
      // 延迟重启应用
      setTimeout(() => {
        uni.reLaunch({
          url: '/pages/library/library'
        })
      }, 1000)
    }
  }
}
</script>

<style scoped>
.settings-page {
  height: 100vh;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.content {
  width: calc(100% - 96rpx); /* 减去左右内边距 48rpx * 2 = 96rpx */
  padding: 0 48rpx;
}

/* 头部 */
.header {
  padding: 32rpx 32rpx 16rpx;
}

.title {
  font-size: 48rpx;
  font-weight: bold;
  background: linear-gradient(135deg, var(--text-primary) 0%, var(--text-secondary) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

/* 设置容器 */
.settings-container {
  padding: 100rpx 0 32rpx; /* 移除左右内边距，只保留上下内边距 */
}

/* 区块 */
.section {
  margin-bottom: 48rpx;
}

.section-title {
  font-size: 22rpx;
  font-weight: 600;
  color: var(--accent-primary);
  text-transform: uppercase;
  letter-spacing: 2rpx;
  margin-bottom: 24rpx;
  display: block;
}

/* 主题网格 */
.theme-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16rpx;
}

.theme-card {
  position: relative;
  padding: 32rpx;
  border-radius: 16rpx;
  border: 4rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  transition: var(--transition-base);
}

.theme-card:active {
  transform: scale(0.98);
}

.theme-card.active {
  border-color: var(--accent-primary);
  background-color: rgba(0, 243, 255, 0.1);
  box-shadow: 0 0 20rpx rgba(0, 243, 255, 0.2);
}

.theme-name {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.theme-card.active .theme-name {
  color: var(--accent-primary);
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
  padding: 0rpx 40rpx;
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

.avatar-btn {
  padding: 0;
  margin: 0;
  background: none;
  border: none;
  line-height: 1;
}

.avatar-btn::after {
  border: none;
}

.avatar {
  width: 120rpx;
  height: 120rpx;
  border-radius: 60rpx;
  display: block;
}

.info-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.nickname-input-wrapper {
  width: 100%;
}

.nickname-input {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
  background: transparent;
  border: none;
  padding: 0;
  width: 100%;
}

.nickname {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.status-text {
  font-size: 22rpx;
  color: var(--accent-primary);
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

/* 同步操作按钮 */
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

/* 设置卡片 */
.setting-card {
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  padding: 24rpx;
  border: 2rpx solid var(--border-primary);
}

.setting-card.clickable {
  transition: var(--transition-base);
}

.setting-card.clickable:active {
  transform: scale(0.98);
  background-color: var(--bg-secondary);
}

.setting-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.setting-header {
  display: flex;
  align-items: center;
  gap: 16rpx;
  margin-bottom: 24rpx;
}

.setting-left {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.setting-icon {
  display: flex;
  align-items: center;
  justify-content: center;
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

/* 危险按钮 */
.danger-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12rpx;
  padding: 20rpx;
  background-color: rgba(239, 68, 68, 0.1);
  border: 2rpx solid rgba(239, 68, 68, 0.3);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.danger-btn:active {
  background-color: rgba(239, 68, 68, 0.2);
  transform: scale(0.98);
}

.danger-text {
  font-size: 26rpx;
  color: var(--error-color);
  font-weight: 500;
}

/* 关于卡片 */
.about-card {
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  padding: 24rpx;
  border: 2rpx solid var(--border-primary);
}

.about-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24rpx;
}

.logo-wrapper {
  flex: 1;
  display: flex;
  align-items: center;
}

.version-badge {
  padding: 8rpx 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 8rpx;
}

.version-text {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--text-secondary);
}

.about-desc {
  font-size: 22rpx;
  color: var(--text-secondary);
  line-height: 1.6;
}

/* 浅色主题适配 */
.light-theme .theme-card.active {
  background-color: rgba(0, 153, 204, 0.08);
  box-shadow: 0 0 15rpx rgba(0, 153, 204, 0.15);
}

/* 登录弹窗 */
.login-modal {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.7);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  padding: 64rpx;
}

.modal-content {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  border: 2rpx solid var(--border-primary);
  overflow: hidden;
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--text-primary);
}

.close-btn {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.close-btn:active {
  transform: scale(0.9);
  color: var(--text-primary);
}

.modal-body {
  padding: 32rpx;
}

.form-item {
  margin-bottom: 32rpx;
}

.form-label {
  display: block;
  font-size: 24rpx;
  color: var(--text-secondary);
  margin-bottom: 16rpx;
  font-weight: 500;
}

.avatar-picker {
  width: 100%;
  padding: 0;
  margin: 0;
  background: none;
  border: none;
  display: flex;
  justify-content: center;
}

.avatar-picker::after {
  border: none;
}

.temp-avatar {
  width: 160rpx;
  height: 160rpx;
  border-radius: 80rpx;
  display: block;
}

.temp-avatar-placeholder {
  width: 160rpx;
  height: 160rpx;
  border-radius: 80rpx;
  background-color: var(--bg-secondary);
  border: 4rpx dashed var(--border-secondary);
  display: flex;
  flex-direction: column;
  align-items: center;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.temp-avatar-placeholder:active {
  border-color: var(--accent-primary);
  color: var(--accent-primary);
}

.picker-hint {
  font-size: 20rpx;
  line-height: 20rpx;
}

.nickname-picker {
  padding: 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.nickname-picker:focus {
  border-color: var(--accent-primary);
}

.input-placeholder {
  color: var(--text-tertiary);
}

.form-tips {
  margin-top: 24rpx;
  padding: 20rpx;
  background-color: var(--bg-secondary);
  border-radius: 12rpx;
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.tips-text {
  font-size: 22rpx;
  color: var(--text-secondary);
  line-height: 1.5;
}

.modal-footer {
  padding: 24rpx 32rpx 32rpx;
}

.confirm-btn {
  width: 100%;
  padding: 0 24rpx;
  background: linear-gradient(135deg, #00f3ff 0%, #00a8ff 100%);
  color: #000;
  border: none;
  border-radius: 12rpx;
  font-size: 28rpx;
  font-weight: 600;
  transition: var(--transition-base);
}

.confirm-btn::after {
  border: none;
}

.confirm-btn:active {
  transform: scale(0.98);
  opacity: 0.9;
}
</style>


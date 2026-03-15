// 用户状态管理
import { defineStore } from 'pinia'
import { userAPI, followAPI } from '../api/index.js'

export const useUserStore = defineStore('user', {
  state: () => ({
    isLoggedIn: false,
    userInfo: null,
    openid: '',
    syncEnabled: false,
    currentUser: null, // 当前用户完整信息
    isLoading: false,
    error: null
  }),

  getters: {
    // 是否已登录
    hasLogin: (state) => state.isLoggedIn,
    
    // 获取用户昵称
    nickname: (state) => {
      if (state.currentUser) return state.currentUser.name
      return state.userInfo?.nickName || '微信用户'
    },
    
    // 获取用户头像
    avatar: (state) => {
      if (state.currentUser) return state.currentUser.avatar
      return state.userInfo?.avatarUrl || ''
    },
    
    // 是否开启云同步
    isSyncEnabled: (state) => state.syncEnabled && state.isLoggedIn,
    
    // 获取用户统计信息
    userStats: (state) => {
      if (state.currentUser) {
        return {
          level: state.currentUser.level,
          followersCount: state.currentUser.followersCount || state.currentUser.followers || 0,
          followingCount: state.currentUser.followingCount || state.currentUser.following || 0,
          worksCount: state.currentUser.worksCount || state.currentUser.artworks || 0,
          totalLikes: state.currentUser.totalLikes || state.currentUser.likes || 0,
          badges: state.currentUser.badges || []
        }
      }
      return null
    }
  },

  actions: {
    // 初始化
    async init() {
      this.loadUserFromStorage()
      if (this.isLoggedIn) {
        await this.loadCurrentUser()
      }
    },
    
    // 从本地存储加载用户信息
    loadUserFromStorage() {
      const isLoggedIn = uni.getStorageSync('isLoggedIn')
      const userInfo = uni.getStorageSync('userInfo')
      const syncEnabled = uni.getStorageSync('syncEnabled')
      const loginTime = uni.getStorageSync('loginTime')
      
      if (isLoggedIn) {
        // 检查登录是否过期（30天）
        const now = Date.now()
        const thirtyDays = 30 * 24 * 60 * 60 * 1000
        
        if (loginTime && (now - loginTime) < thirtyDays) {
          // 未过期，恢复登录状态
          this.isLoggedIn = true
          this.userInfo = userInfo || { nickName: '微信用户', avatarUrl: '' }
          this.syncEnabled = syncEnabled || false
          this.openid = uni.getStorageSync('openid') || ''
        } else {
          // 已过期，清除登录状态
          this.logout()
        }
      }
    },
    
    // 加载当前用户完整信息
    async loadCurrentUser() {
      if (!this.isLoggedIn) return

      this.isLoading = true
      this.error = null

      try {
        const res = await userAPI.getProfile()
        if (res.success) {
          this.currentUser = res.data
          this.userInfo = {
            nickName: this.currentUser.name || this.currentUser.nickname,
            avatarUrl: this.currentUser.avatar
          }
          uni.setStorageSync('userInfo', this.userInfo)
          uni.setStorageSync('currentUser', this.currentUser)
        }
      } catch (e) {
        console.error('加载用户信息失败:', e)
        this.error = '加载用户信息失败'
      } finally {
        this.isLoading = false
      }
    },

    // 微信登录
    async login() {
      try {
        // #ifdef MP-WEIXIN
        const loginRes = await new Promise((resolve, reject) => {
          uni.login({
            provider: 'weixin',
            success: (res) => resolve(res),
            fail: (err) => reject(err)
          })
        })

        const res = await userAPI.login(loginRes.code)
        if (res.success) {
          this.isLoggedIn = true
          this.openid = res.data.openid || ''
          uni.setStorageSync('auth_token', res.data.token)
          await this.loadCurrentUser()

          const loginTime = Date.now()
          uni.setStorageSync('isLoggedIn', true)
          uni.setStorageSync('openid', this.openid)
          uni.setStorageSync('loginTime', loginTime)

          return { success: true, message: '登录成功' }
        }
        return { success: false, error: res.message || '登录失败' }
        // #endif

        // #ifndef MP-WEIXIN
        const res2 = await userAPI.login('h5_dev')
        if (res2.success) {
          this.isLoggedIn = true
          this.openid = res2.data.openid || ''
          uni.setStorageSync('auth_token', res2.data.token)
          await this.loadCurrentUser()

          const loginTime = Date.now()
          uni.setStorageSync('isLoggedIn', true)
          uni.setStorageSync('openid', this.openid)
          uni.setStorageSync('loginTime', loginTime)

          return { success: true, message: '登录成功' }
        }
        return { success: false, error: res2.message || '登录失败' }
        // #endif
      } catch (error) {
        return {
          success: false,
          error: error.errMsg || error.message || '登录失败'
        }
      }
    },

    // 获取用户资料（兼容旧接口）
    async getUserProfile() {
      if (!this.isLoggedIn) {
        return { success: false, error: '请先登录' }
      }
      
      try {
        // 重新加载用户信息
        await this.loadCurrentUser()
        return { success: true, message: '获取成功' }
      } catch (error) {
        return { success: false, error: '获取失败' }
      }
    },

    // 更新用户信息
    async updateUserInfo(updates) {
      if (!this.isLoggedIn || !this.currentUser) {
        return { success: false, error: '请先登录' }
      }
      
      try {
        // 更新当前用户信息
        this.currentUser = { ...this.currentUser, ...updates }
        
        // 如果更新了基础信息，同步到userInfo
        if (updates.name) {
          this.userInfo.nickName = updates.name
        }
        if (updates.avatar) {
          this.userInfo.avatarUrl = updates.avatar
        }
        
        // 保存到本地存储
        uni.setStorageSync('userInfo', this.userInfo)
        uni.setStorageSync('currentUser', this.currentUser)
        
        return { success: true, message: '更新成功' }
      } catch (error) {
        return { success: false, error: '更新失败' }
      }
    },

    // 关注/取消关注用户
    async toggleFollowUser(userId) {
      if (!this.isLoggedIn) {
        return { success: false, error: '请先登录' }
      }
      
      try {
        const res = await followAPI.toggleFollow(userId)
        const result = res.success ? res.data : { success: false }
        
        // 更新当前用户的关注数
        if (result.success && this.currentUser) {
          if (result.isFollowing) {
            this.currentUser.following += 1
          } else {
            this.currentUser.following -= 1
          }
          uni.setStorageSync('currentUser', this.currentUser)
        }
        
        return result
      } catch (error) {
        return { success: false, error: '操作失败' }
      }
    },

    // 同步用户信息（已废弃，保留兼容性）
    async syncUserInfo() {
      return { success: true }
    },

    // 开启云同步
    enableSync() {
      this.syncEnabled = true
      uni.setStorageSync('syncEnabled', true)
    },

    // 关闭云同步
    disableSync() {
      this.syncEnabled = false
      uni.setStorageSync('syncEnabled', false)
    },

    // 退出登录
    logout() {
      this.isLoggedIn = false
      this.userInfo = null
      this.currentUser = null
      this.openid = ''
      this.syncEnabled = false
      
      // 清除本地存储
      uni.removeStorageSync('openid')
      uni.removeStorageSync('isLoggedIn')
      uni.removeStorageSync('userInfo')
      uni.removeStorageSync('currentUser')
      uni.removeStorageSync('syncEnabled')
      uni.removeStorageSync('loginTime')
    },

    // 获取用户设置
    getUserSettings() {
      if (this.currentUser && this.currentUser.settings) {
        return this.currentUser.settings
      }
      return {
        autoSync: true,
        notifications: true,
        privacy: 'public',
        language: 'zh-CN'
      }
    },
    
    // 更新用户设置
    updateUserSettings(settings) {
      if (this.currentUser) {
        this.currentUser.settings = { ...this.currentUser.settings, ...settings }
        uni.setStorageSync('currentUser', this.currentUser)
      }
    },
    
    // 获取用户统计
    getUserStats() {
      if (this.currentUser && this.currentUser.stats) {
        return this.currentUser.stats
      }
      return {
        totalProjects: 0,
        completedProjects: 0,
        totalWorkingHours: 0,
        favoriteColors: [],
        mostUsedSize: '52x52'
      }
    }
  }
})

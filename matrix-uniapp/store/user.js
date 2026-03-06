// 用户状态管理
import { defineStore } from 'pinia'

export const useUserStore = defineStore('user', {
  state: () => ({
    isLoggedIn: false,
    userInfo: null,
    openid: '',
    syncEnabled: false
  }),

  getters: {
    // 是否已登录
    hasLogin: (state) => state.isLoggedIn,
    
    // 获取用户昵称
    nickname: (state) => state.userInfo?.nickName || '微信用户',
    
    // 获取用户头像
    avatar: (state) => state.userInfo?.avatarUrl || '',
    
    // 是否开启云同步
    isSyncEnabled: (state) => state.syncEnabled && state.isLoggedIn
  },

  actions: {
    // 微信登录（符合最新规范，不强制获取用户信息）
    async login() {
      try {
        // #ifdef MP-WEIXIN
        
        // 调用 wx.login 获取 code
        const loginRes = await new Promise((resolve, reject) => {
          uni.login({
            provider: 'weixin',
            success: (res) => {
              resolve(res)
            },
            fail: (err) => {
              reject(err)
            }
          })
        })
        
        // 标记为已登录，用户信息由用户主动设置
        this.isLoggedIn = true
        this.openid = 'auto' // 云开发会自动处理
        this.userInfo = {
          nickName: '微信用户',
          avatarUrl: ''
        }
        
        // 保存到本地，包含登录时间戳
        const loginTime = Date.now()
        uni.setStorageSync('isLoggedIn', true)
        uni.setStorageSync('userInfo', this.userInfo)
        uni.setStorageSync('loginTime', loginTime)
        
        return {
          success: true,
          message: '登录成功'
        }
        // #endif
        
        // #ifndef MP-WEIXIN
        return {
          success: false,
          error: '仅支持微信小程序'
        }
        // #endif
      } catch (error) {
        
        return {
          success: false,
          error: error.errMsg || error.message || '登录失败'
        }
      }
    },

    // 同步用户信息（已废弃，使用本地存储）
    async syncUserInfo() {
      // 功能已移除，保留接口兼容性
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
      this.openid = ''
      this.syncEnabled = false
      
      // 清除本地存储
      uni.removeStorageSync('openid')
      uni.removeStorageSync('isLoggedIn')
      uni.removeStorageSync('userInfo')
      uni.removeStorageSync('syncEnabled')
      uni.removeStorageSync('loginTime')
    },

    // 初始化（从本地恢复登录状态）
    init() {
      const isLoggedIn = uni.getStorageSync('isLoggedIn')
      const userInfo = uni.getStorageSync('userInfo')
      const syncEnabled = uni.getStorageSync('syncEnabled')
      const loginTime = uni.getStorageSync('loginTime')
      
      if (isLoggedIn) {
        // 检查登录是否过期（30天）
        const now = Date.now()
        const thirtyDays = 30 * 24 * 60 * 60 * 1000 // 30天的毫秒数
        
        if (loginTime && (now - loginTime) < thirtyDays) {
          // 未过期，恢复登录状态
          this.isLoggedIn = true
          this.userInfo = userInfo || { nickName: '微信用户', avatarUrl: '' }
          this.syncEnabled = syncEnabled || false
          this.openid = 'auto'
        } else {
          // 已过期，清除登录状态
          this.logout()
        }
      } else {
        // 未找到登录状态
      }
    }
  }
})

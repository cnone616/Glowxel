// 设备连接状态管理 - 单设备 IP 直连
import { defineStore } from 'pinia'
import DeviceWebSocket from '../utils/webSocket.js'

const DEVICE_MODE_KEY = 'device_mode'
const DEVICE_LAST_BUSINESS_MODE_KEY = 'device_last_business_mode'

export const useDeviceStore = defineStore('device', {
  state: () => ({
    webSocket: null,
    connected: false,
    deviceIp: '',
    deviceInfo: null,
    deviceMode: 'clock', // 前端当前工作模式
    lastBusinessMode: 'clock',
    isLoading: false,
    error: null
  }),

  getters: {
    isConnected: (state) => state.connected,
    getDeviceIp: (state) => state.deviceIp,
    getDeviceInfo: (state) => state.deviceInfo
  },

  actions: {
    resolveDeviceModeFromStatus(data) {
      if (!data || typeof data !== 'object') {
        return null
      }

      if (typeof data.mode !== 'string') {
        return null
      }

      if (data.mode === 'animation' && typeof data.effectMode === 'string') {
        if (
          data.effectMode === 'text_display' ||
          data.effectMode === 'breath_effect' ||
          data.effectMode === 'rhythm_effect' ||
          data.effectMode === 'tetris' ||
          data.effectMode === 'ambient_effect' ||
          data.effectMode === 'eyes'
        ) {
          return data.effectMode
        }
      }

      if (
        data.mode === 'clock' ||
        data.mode === 'animation' ||
        data.mode === 'theme' ||
        data.mode === 'canvas' ||
        data.mode === 'tetris' ||
        data.mode === 'weather' ||
        data.mode === 'countdown' ||
        data.mode === 'stopwatch' ||
        data.mode === 'notification'
      ) {
        return data.mode
      }

      return null
    },

    // 初始化
    init() {
      if (!this.webSocket) {
        this.webSocket = new DeviceWebSocket()
        this.setupCallbacks()
      }

      // 从缓存读取设备 IP
      const savedIp = uni.getStorageSync('device_ip')
      if (savedIp) {
        this.deviceIp = savedIp
      }

      const savedMode = uni.getStorageSync(DEVICE_MODE_KEY)
      if (savedMode) {
        this.deviceMode = savedMode
      }

      const savedBusinessMode = uni.getStorageSync(DEVICE_LAST_BUSINESS_MODE_KEY)
      if (savedBusinessMode) {
        this.lastBusinessMode = savedBusinessMode
      } else if (savedMode && savedMode !== 'canvas') {
        this.lastBusinessMode = savedMode
      }
    },

    // 设置 WebSocket 回调
    setupCallbacks() {
      this.webSocket.onConnect(() => {
        this.connected = true
        this.error = null
      })

      this.webSocket.onDisconnect(() => {
        this.connected = false
        this.deviceInfo = null
      })

      this.webSocket.onError((err) => {
        this.error = err || null
      })

      this.webSocket.onMessage((data) => {
        if (data.status === 'ok') {
          this.deviceInfo = data
        }
        if (data.status === 'connected') {
          this.deviceInfo = data
        }
        const resolvedMode = this.resolveDeviceModeFromStatus(data)
        if (resolvedMode) {
          this.deviceMode = resolvedMode
          uni.setStorageSync(DEVICE_MODE_KEY, resolvedMode)
          if (resolvedMode !== 'canvas') {
            this.lastBusinessMode = resolvedMode
            uni.setStorageSync(DEVICE_LAST_BUSINESS_MODE_KEY, resolvedMode)
          }
        }
      })
    },

    setDeviceMode(mode, options = {}) {
      const persist = options.persist !== false
      const businessMode = options.businessMode !== false

      this.deviceMode = mode
      if (persist) {
        uni.setStorageSync(DEVICE_MODE_KEY, mode)
      }
      if (businessMode && mode !== 'canvas') {
        this.lastBusinessMode = mode
        uni.setStorageSync(DEVICE_LAST_BUSINESS_MODE_KEY, mode)
      }
    },

    // 连接设备（只支持 IP 直连）
    async connect(ip) {
      if (!this.webSocket) {
        this.init()
      }

      // 如果已经连接到同一个 IP，直接返回
      if (this.connected && this.deviceIp === ip) {
        return { success: true }
      }

      // 如果连接到不同的 IP，先断开旧连接
      if (this.connected && this.deviceIp !== ip) {
        this.webSocket.disconnect()
        this.connected = false
      }

      this.deviceIp = ip
      uni.setStorageSync('device_ip', ip)

      try {
        await this.webSocket.connect(ip)
        setTimeout(async () => {
          try {
            await this.webSocket.getStatus()
          } catch (statusErr) {
            console.warn('后台获取设备状态失败:', statusErr)
          }
        }, 800)
        return { success: true }
      } catch (err) {
        console.error('连接失败:', err)
        return { success: false, error: err }
      }
    },

    // 断开连接
    disconnect() {
      if (this.webSocket) {
        this.webSocket.disconnect()
      }
      this.connected = false
      this.deviceInfo = null
    },

    // 发送图片数据到设备（画板模式）
    async sendImage(pixels, width, height) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      try {
        await this.webSocket.setMode('canvas')
        await this.webSocket.showImage(pixels, width, height)
        return { success: true }
      } catch (err) {
        console.error('发送图片失败:', err)
        return { success: false, error: err }
      }
    },

    // 发送稀疏像素数据到设备（画板模式，只发送有颜色的像素）
    async sendSparseImage(sparsePixels, width, height) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      try {
        await this.webSocket.setMode('canvas')
        await this.webSocket.showSparseImage(sparsePixels, width, height)
        return { success: true }
      } catch (err) {
        console.error('发送稀疏图片失败:', err)
        return { success: false, error: err }
      }
    },

    // 发送部分更新（仅更新指定像素，画板模式）
    async sendPartialUpdate(pixelData, width, height) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      try {
        await this.webSocket.sendPartialUpdate(pixelData, width, height)
        return { success: true }
      } catch (err) {
        console.error('发送部分更新失败:', err)
        return { success: false, error: err }
      }
    },

    // 设置亮度
    async setBrightness(value) {
      if (!this.connected || !this.webSocket) {
        return { success: false, message: '设备未连接' }
      }

      try {
        await this.webSocket.setBrightness(value)
        return { success: true }
      } catch (err) {
        console.error('设置亮度失败:', err)
        return { success: false, error: err }
      }
    },

    // 清空屏幕
    async clear() {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      try {
        await this.webSocket.clear()
        return { success: true }
      } catch (err) {
        console.error('清空屏幕失败:', err)
        return { success: false, error: err }
      }
    },

    // 获取 WebSocket 实例（供页面直接使用）
    getWebSocket() {
      if (!this.webSocket) {
        this.init()
      }
      return this.webSocket
    }
  }
})

// 设备连接状态管理 - 单设备 IP 直连
import { defineStore } from 'pinia'
import DeviceWebSocket from '../utils/webSocket.js'

export const useDeviceStore = defineStore('device', {
  state: () => ({
    webSocket: null,
    connected: false,
    deviceIp: '',
    deviceInfo: null,
    deviceMode: 'clock', // 板子当前模式: clock / animation
    isLoading: false,
    error: null
  }),

  getters: {
    isConnected: (state) => state.connected,
    getDeviceIp: (state) => state.deviceIp,
    getDeviceInfo: (state) => state.deviceInfo
  },

  actions: {
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
    },

    // 设置 WebSocket 回调
    setupCallbacks() {
      this.webSocket.onConnect(() => {
        this.connected = true
      })

      this.webSocket.onDisconnect(() => {
        this.connected = false
        this.deviceInfo = null
      })

      this.webSocket.onError((err) => {
        this.connected = false
        this.deviceInfo = null
      })

      this.webSocket.onMessage((data) => {
        if (data.status === 'ok') {
          this.deviceInfo = data
        }
        // ESP32 连接时返回当前模式
        if (data.status === 'connected') {
          this.deviceInfo = data
          // canvas 映射为 clock（前端叫法）
          const mode = data.mode === 'animation' ? 'animation' : 'clock'
          this.deviceMode = mode
        }
      })
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
        await this.webSocket.getStatus()
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

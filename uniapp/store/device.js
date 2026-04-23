// 设备连接状态管理 - 单设备 IP 直连
import { defineStore } from 'pinia'
import DeviceWebSocket from '../utils/webSocket.js'

const DEVICE_MODE_KEY = 'device_mode'
const DEVICE_LAST_BUSINESS_MODE_KEY = 'device_last_business_mode'
const VALID_BUSINESS_MODES = [
  'clock',
  'animation',
  'theme',
  'canvas',
  'gif_player',
  'led_matrix_showcase',
  'game_screensaver',
  'tetris',
  'tetris_clock',
  'planet_screensaver',
  'eyes'
]

function isPersistedBusinessMode(mode) {
  return typeof mode === 'string' && VALID_BUSINESS_MODES.includes(mode)
}

function resolveTopLevelModeFromBusinessMode(mode) {
  if (mode === 'clock') {
    return 'clock'
  }

  if (mode === 'theme') {
    return 'theme'
  }

  if (mode === 'canvas') {
    return 'canvas'
  }

  if (
    mode === 'animation' ||
    mode === 'gif_player' ||
    mode === 'led_matrix_showcase' ||
    mode === 'game_screensaver' ||
    mode === 'tetris' ||
    mode === 'tetris_clock' ||
    mode === 'planet_screensaver' ||
    mode === 'eyes' ||
    mode === 'text_display' ||
    mode === 'weather' ||
    mode === 'countdown' ||
    mode === 'stopwatch' ||
    mode === 'notification' ||
    mode === 'breath_effect' ||
    mode === 'rhythm_effect' ||
    mode === 'ambient_effect'
  ) {
    return 'animation'
  }

  if (mode === 'transferring') {
    return 'transferring'
  }

  return null
}

export const useDeviceStore = defineStore('device', {
  state: () => ({
    webSocket: null,
    connected: false,
    deviceIp: '',
    deviceInfo: null,
    deviceMode: 'clock', // 前端当前工作模式
    lastBusinessMode: 'clock',
    isLoading: false,
    error: null,
    statusSyncInFlight: false
  }),

  getters: {
    isConnected: (state) => state.connected,
    getDeviceIp: (state) => state.deviceIp,
    getDeviceInfo: (state) => state.deviceInfo
  },

  actions: {
    isDeviceStatusPayload(data) {
      if (!data || typeof data !== 'object') {
        return false
      }

      return (
        data.status === 'ok' &&
        typeof data.ip === 'string' &&
        typeof data.width === 'number' &&
        typeof data.height === 'number' &&
        typeof data.brightness === 'number' &&
        typeof data.mode === 'string' &&
        typeof data.businessMode === 'string' &&
        typeof data.effectMode === 'string'
      )
    },

    async syncDeviceStatus() {
      if (!this.webSocket || !this.connected || this.statusSyncInFlight) {
        return null
      }

      this.statusSyncInFlight = true
      try {
        const status = await this.webSocket.getStatus()
        if (!this.connected) {
          return null
        }
        if (!this.isDeviceStatusPayload(status)) {
          return null
        }

        this.applyDeviceStatus(status)
        return status
      } catch (err) {
        console.error('同步设备状态失败:', err)
        return null
      } finally {
        this.statusSyncInFlight = false
      }
    },

    matchesModeExpectation(status, expectation = {}) {
      if (!this.isDeviceStatusPayload(status)) {
        return false
      }

      if (
        typeof expectation.businessMode === 'string' &&
        status.businessMode !== expectation.businessMode
      ) {
        return false
      }

      if (typeof expectation.mode === 'string' && status.mode !== expectation.mode) {
        return false
      }

      return true
    },

    async syncAndRequireMode(expectation, errorMessage = '设备模式确认失败') {
      const status = await this.syncDeviceStatus()
      if (!status || !this.matchesModeExpectation(status, expectation)) {
        throw new Error(errorMessage)
      }
      return status
    },

    buildModeExpectation(expectation = {}, options = {}) {
      const normalizedExpectation = { ...expectation }

      if (
        options.requireTopLevelMode === true &&
        typeof normalizedExpectation.businessMode === 'string'
      ) {
        const expectedTopLevelMode = resolveTopLevelModeFromBusinessMode(
          normalizedExpectation.businessMode
        )
        if (!expectedTopLevelMode) {
          throw new Error(`未支持的业务模式校验: ${normalizedExpectation.businessMode}`)
        }
        normalizedExpectation.mode = expectedTopLevelMode
      }

      return normalizedExpectation
    },

    async syncAndRequireBusinessMode(
      businessMode,
      errorMessage = '设备模式确认失败',
      options = {}
    ) {
      return this.syncAndRequireMode(
        this.buildModeExpectation({ businessMode }, options),
        errorMessage
      )
    },

    resolveDeviceModeFromStatus(data) {
      if (!data || typeof data !== 'object') {
        return null
      }

      if (
        typeof data.businessMode === 'string' &&
        VALID_BUSINESS_MODES.includes(data.businessMode)
      ) {
        return data.businessMode
      }

      return null
    },

    applyResolvedBusinessMode(mode) {
      if (!isPersistedBusinessMode(mode)) {
        return false
      }

      this.deviceMode = mode
      uni.setStorageSync(DEVICE_MODE_KEY, mode)

      if (mode !== 'canvas') {
        this.lastBusinessMode = mode
        uni.setStorageSync(DEVICE_LAST_BUSINESS_MODE_KEY, mode)
      }

      return true
    },

    applyDeviceStatus(status) {
      if (!this.isDeviceStatusPayload(status)) {
        return false
      }

      this.deviceInfo = status
      const resolvedMode = this.resolveDeviceModeFromStatus(status)
      if (resolvedMode) {
        this.applyResolvedBusinessMode(resolvedMode)
      }
      return true
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
      if (isPersistedBusinessMode(savedMode)) {
        this.deviceMode = savedMode
      }

      const savedBusinessMode = uni.getStorageSync(DEVICE_LAST_BUSINESS_MODE_KEY)
      if (isPersistedBusinessMode(savedBusinessMode)) {
        this.lastBusinessMode = savedBusinessMode
      } else if (isPersistedBusinessMode(savedMode) && savedMode !== 'canvas') {
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
        this.statusSyncInFlight = false
      })

      this.webSocket.onError((err) => {
        this.error = err || null
      })

      this.webSocket.onMessage((data) => {
        if (data.status === 'connected') {
          const resolvedMode = this.resolveDeviceModeFromStatus(data)
          if (resolvedMode) {
            this.applyResolvedBusinessMode(resolvedMode)
          }
          return
        }

        if (this.isDeviceStatusPayload(data)) {
          this.applyDeviceStatus(data)
        }
      })
    },

    setDeviceMode(mode, options = {}) {
      const source = options.source || 'local'
      if (this.connected && source !== 'device') {
        return
      }

      const persist = options.persist !== false
      const businessMode = options.businessMode !== false

      this.deviceMode = mode
      if (persist && isPersistedBusinessMode(mode)) {
        uni.setStorageSync(DEVICE_MODE_KEY, mode)
      }
      if (businessMode && isPersistedBusinessMode(mode) && mode !== 'canvas') {
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
      this.statusSyncInFlight = false
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

// 设备连接状态管理 - 单设备 IP 直连
import { defineStore } from 'pinia'
import DeviceWebSocket from '../utils/webSocket.js'
import {
  createDefaultMazeModeConfig,
  readSavedMazeModeConfig
} from '../utils/mazeModeConfig.js'

const DEVICE_MODE_KEY = 'device_mode'
const DEVICE_LAST_BUSINESS_MODE_KEY = 'device_last_business_mode'
const STATUS_SYNC_TIMEOUT_MS = 15000
const CONNECT_STATUS_VERIFY_TIMEOUT_MS = 4000
const INITIAL_CONNECT_MAX_ATTEMPTS = 2
const INITIAL_CONNECT_WS_TIMEOUT_MS = 10000
const INITIAL_CONNECT_RETRY_DELAY_MS = 1200
const TETRIS_CONFIG_STORAGE_KEY = 'tetris_config'
const TETRIS_CLOCK_STORAGE_KEY = 'tetris_clock_config'
const SNAKE_MODE_CONFIG_KEY = 'snake_mode_config'
const TETRIS_CLOCK_SPEED_OPTIONS = Object.freeze({
  slow: 300,
  normal: 150,
  fast: 80
})
const DEFAULT_TETRIS_MODE_PAYLOAD = Object.freeze({
  clearMode: true,
  cellSize: 2,
  speed: 150,
  showClock: true,
  pieces: [0, 1, 2, 3, 4, 5, 6]
})
const DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD = Object.freeze({
  cellSize: 2,
  speed: 150,
  hourFormat: 24
})
const DEFAULT_SNAKE_MODE_PAYLOAD = Object.freeze({
  speed: 6,
  snakeWidth: 2,
  snakeColor: '#56d678',
  foodColor: '#ffa854',
  font: 'minimal_3x5',
  showSeconds: false,
  snakeSkin: 'gradient'
})
const VALID_BUSINESS_MODES = [
  'clock',
  'animation',
  'theme',
  'canvas',
  'gif_player',
  'led_matrix_showcase',
  'maze',
  'snake',
  'tetris',
  'tetris_clock',
  'planet_screensaver',
  'eyes'
]

function isPersistedBusinessMode(mode) {
  return typeof mode === 'string' && VALID_BUSINESS_MODES.includes(mode)
}

function normalizeStoredTetrisModePayload(saved) {
  const normalized = {
    clearMode: DEFAULT_TETRIS_MODE_PAYLOAD.clearMode,
    cellSize: DEFAULT_TETRIS_MODE_PAYLOAD.cellSize,
    speed: DEFAULT_TETRIS_MODE_PAYLOAD.speed,
    showClock: DEFAULT_TETRIS_MODE_PAYLOAD.showClock,
    pieces: DEFAULT_TETRIS_MODE_PAYLOAD.pieces.slice()
  }

  if (!saved || typeof saved !== 'object') {
    return normalized
  }

  if (typeof saved.clearMode === 'boolean') {
    normalized.clearMode = saved.clearMode
  }
  if (saved.cellSize === 1 || saved.cellSize === 2 || saved.cellSize === 3) {
    normalized.cellSize = saved.cellSize
  }
  if (
    saved.speed === 'slow' ||
    saved.speed === 'normal' ||
    saved.speed === 'fast'
  ) {
    normalized.speed = TETRIS_CLOCK_SPEED_OPTIONS[saved.speed]
  }
  if (typeof saved.showClock === 'boolean') {
    normalized.showClock = saved.showClock
  }
  if (Array.isArray(saved.pieces) && saved.pieces.length > 0) {
    const pieces = saved.pieces.filter(
      (piece) => Number.isInteger(piece) && piece >= 0 && piece <= 6
    )
    if (pieces.length > 0) {
      normalized.pieces = pieces
    }
  }

  return normalized
}

function normalizeStoredTetrisClockModePayload(saved) {
  const normalized = {
    cellSize: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.cellSize,
    speed: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.speed,
    hourFormat: DEFAULT_TETRIS_CLOCK_MODE_PAYLOAD.hourFormat
  }

  if (!saved || typeof saved !== 'object') {
    return normalized
  }

  if (
    saved.speed === 'slow' ||
    saved.speed === 'normal' ||
    saved.speed === 'fast'
  ) {
    normalized.speed = TETRIS_CLOCK_SPEED_OPTIONS[saved.speed]
  }
  if (saved.hourFormat === 12 || saved.hourFormat === 24) {
    normalized.hourFormat = saved.hourFormat
  }

  return normalized
}

function getStoredTetrisModePayload() {
  return normalizeStoredTetrisModePayload(
    uni.getStorageSync(TETRIS_CONFIG_STORAGE_KEY)
  )
}

function getStoredTetrisClockModePayload() {
  return normalizeStoredTetrisClockModePayload(
    uni.getStorageSync(TETRIS_CLOCK_STORAGE_KEY)
  )
}

function normalizeStoredSnakeModePayload(saved) {
  const normalized = {
    speed: DEFAULT_SNAKE_MODE_PAYLOAD.speed,
    snakeWidth: DEFAULT_SNAKE_MODE_PAYLOAD.snakeWidth,
    snakeColor: DEFAULT_SNAKE_MODE_PAYLOAD.snakeColor,
    foodColor: DEFAULT_SNAKE_MODE_PAYLOAD.foodColor,
    font: DEFAULT_SNAKE_MODE_PAYLOAD.font,
    showSeconds: DEFAULT_SNAKE_MODE_PAYLOAD.showSeconds,
    snakeSkin: DEFAULT_SNAKE_MODE_PAYLOAD.snakeSkin
  }

  if (!saved || typeof saved !== 'object') {
    return normalized
  }

  if (Number.isFinite(Number(saved.speed))) {
    normalized.speed = Number(saved.speed)
  }
  if (Number.isFinite(Number(saved.snakeWidth))) {
    normalized.snakeWidth = Number(saved.snakeWidth)
  }
  if (typeof saved.snakeColor === 'string') {
    normalized.snakeColor = saved.snakeColor
  }
  if (typeof saved.foodColor === 'string') {
    normalized.foodColor = saved.foodColor
  }
  if (typeof saved.font === 'string') {
    normalized.font = saved.font
  }
  if (saved.showSeconds === true || saved.showSeconds === false) {
    normalized.showSeconds = saved.showSeconds
  }
  if (
    saved.snakeSkin === 'solid' ||
    saved.snakeSkin === 'gradient' ||
    saved.snakeSkin === 'spotted'
  ) {
    normalized.snakeSkin = saved.snakeSkin
  }

  return normalized
}

function getStoredSnakeModePayload() {
  return normalizeStoredSnakeModePayload(
    uni.getStorageSync(SNAKE_MODE_CONFIG_KEY)
  )
}

function delay(ms) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
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

    async syncDeviceStatus(options = {}) {
      if (!this.webSocket || !this.connected || this.statusSyncInFlight) {
        return null
      }

      const timeout =
        Number.isInteger(options.timeout) && options.timeout > 0
          ? options.timeout
          : STATUS_SYNC_TIMEOUT_MS

      this.statusSyncInFlight = true
      try {
        const status = await this.webSocket.getStatus(timeout)
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

    async rollbackBusinessMode(previousMode, options = {}) {
      if (
        !this.webSocket ||
        !this.connected ||
        !isPersistedBusinessMode(previousMode)
      ) {
        return false
      }

      if (
        typeof options.expectedMode === 'string' &&
        options.expectedMode === previousMode
      ) {
        return false
      }

      if (this.deviceMode === previousMode) {
        return false
      }

      try {
        if (
          previousMode === 'clock' ||
          previousMode === 'canvas' ||
          previousMode === 'animation' ||
          previousMode === 'gif_player' ||
          previousMode === 'theme' ||
          previousMode === 'led_matrix_showcase' ||
          previousMode === 'planet_screensaver'
        ) {
          await this.webSocket.runModeTransaction({
            mode: previousMode,
            params: {}
          })
        } else if (previousMode === 'eyes') {
          await this.webSocket.runModeTransaction({
            mode: previousMode,
            params: {}
          })
        } else if (previousMode === 'maze') {
          const savedMazeConfig = readSavedMazeModeConfig()
          let mazePayload = createDefaultMazeModeConfig()
          if (savedMazeConfig) {
            mazePayload = savedMazeConfig
          }
          await this.webSocket.startMaze(mazePayload)
        } else if (previousMode === 'snake') {
          await this.webSocket.startSnake(getStoredSnakeModePayload())
        } else if (previousMode === 'tetris') {
          await this.webSocket.startTetris(getStoredTetrisModePayload())
        } else if (previousMode === 'tetris_clock') {
          await this.webSocket.startTetrisClock(
            getStoredTetrisClockModePayload()
          )
        } else {
          return false
        }
        return true
      } catch (err) {
        console.error('恢复上一次模式失败:', err)
        return false
      }
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

        if (
          (data.status === 'final_ok' || data.status === 'final_error') &&
          typeof data.activeMode === 'string'
        ) {
          this.applyResolvedBusinessMode(data.activeMode)
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
        let lastError = null
        for (
          let attempt = 1;
          attempt <= INITIAL_CONNECT_MAX_ATTEMPTS;
          attempt += 1
        ) {
          try {
            await this.webSocket.connect(ip, 80, {
              connectTimeoutMs: INITIAL_CONNECT_WS_TIMEOUT_MS,
            })

            const status = await this.syncDeviceStatus({
              timeout: CONNECT_STATUS_VERIFY_TIMEOUT_MS,
            })

            if (!status) {
              throw new Error('WebSocket 已连接，但设备状态同步失败')
            }

            return { success: true, status }
          } catch (err) {
            lastError =
              err instanceof Error
                ? err
                : new Error(
                    err && typeof err.message === 'string'
                      ? err.message
                      : '设备连接失败'
                  )
            this.webSocket.disconnect()
            if (attempt < INITIAL_CONNECT_MAX_ATTEMPTS) {
              await delay(INITIAL_CONNECT_RETRY_DELAY_MS)
            }
          }
        }

        throw lastError || new Error('设备连接失败')
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

    async ensureCanvasMode() {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      return this.webSocket.ensureCanvasMode()
    },

    // 发送图片数据到设备（画板模式）
    async sendImage(pixels, width, height) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }

      try {
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
        await this.webSocket.showSparseImage(sparsePixels, width, height)
        return { success: true }
      } catch (err) {
        console.error('发送稀疏图片失败:', err)
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

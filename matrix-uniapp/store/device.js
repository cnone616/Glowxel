// 设备连接状态管理 - 使用Mock数据
import { defineStore } from 'pinia'
import { MockAPI } from '../data/mock/index.js'
import WebSocket from '../utils/webSocket.js'

export const useDeviceStore = defineStore('device', {
  state: () => ({
    webSocket: null,
    connected: false,
    deviceIp: '',
    deviceInfo: null,
    devices: [], // Mock设备列表
    currentDevice: null, // 当前选中的设备
    isLoading: false,
    error: null
  }),

  getters: {
    isConnected: (state) => state.connected,
    getDeviceIp: (state) => state.deviceIp,
    getDeviceInfo: (state) => state.deviceInfo,
    
    // 获取已连接的设备
    connectedDevices: (state) => {
      return state.devices.filter(device => device.status === 'connected')
    },
    
    // 获取设备统计
    deviceStats: (state) => {
      const total = state.devices.length
      const connected = state.devices.filter(d => d.status === 'connected').length
      const disconnected = state.devices.filter(d => d.status === 'disconnected').length
      const lowBattery = state.devices.filter(d => d.battery < 20).length
      
      return { total, connected, disconnected, lowBattery }
    }
  },

  actions: {
    // 初始化
    async init() {
      await this.loadDevices()
      
      if (!this.webSocket) {
        this.webSocket = new WebSocket()
        this.setupCallbacks()
      }
      
      // 从缓存读取设备 IP
      const savedIp = uni.getStorageSync('device_ip')
      if (savedIp) {
        this.deviceIp = savedIp
      }
    },
    
    // 加载设备列表 - 使用Mock数据
    async loadDevices() {
      this.isLoading = true
      this.error = null
      
      try {
        // 从本地存储加载设备状态
        const localDevices = uni.getStorageSync('devices')
        if (localDevices) {
          this.devices = JSON.parse(localDevices)
        } else {
          // 使用Mock数据初始化
          this.devices = MockAPI.devices.getAll()
          this.saveDevicesToStorage()
        }
      } catch (e) {
        console.error('加载设备失败:', e)
        this.error = '加载设备失败'
        // 使用Mock数据作为备选
        this.devices = MockAPI.devices.getAll()
      } finally {
        this.isLoading = false
      }
    },
    
    // 保存设备到本地存储
    saveDevicesToStorage() {
      try {
        uni.setStorageSync('devices', JSON.stringify(this.devices))
      } catch (e) {
        console.error('保存设备失败:', e)
      }
    },
    
    // 获取设备列表
    getDevices() {
      return this.devices
    },
    
    // 根据ID获取设备
    getDeviceById(id) {
      return this.devices.find(device => device.id === id)
    },
    
    // 更新设备状态
    updateDeviceStatus(deviceId, status, additionalData = {}) {
      const deviceIndex = this.devices.findIndex(d => d.id === deviceId)
      if (deviceIndex !== -1) {
        this.devices[deviceIndex] = {
          ...this.devices[deviceIndex],
          status,
          lastConnected: status === 'connected' ? new Date().toLocaleString() : this.devices[deviceIndex].lastConnected,
          ...additionalData
        }
        this.saveDevicesToStorage()
      }
    },

    // 设置 WebSocket 回调
    setupCallbacks() {
      this.webSocket.onConnect(() => {
        this.connected = true
        // 更新当前设备状态
        if (this.currentDevice) {
          this.updateDeviceStatus(this.currentDevice.id, 'connected')
        }
      })
      
      this.webSocket.onDisconnect(() => {
        this.connected = false
        this.deviceInfo = null
        // 更新当前设备状态
        if (this.currentDevice) {
          this.updateDeviceStatus(this.currentDevice.id, 'disconnected')
        }
      })
      
      this.webSocket.onError((err) => {
        this.connected = false
        this.deviceInfo = null
        if (this.currentDevice) {
          this.updateDeviceStatus(this.currentDevice.id, 'disconnected')
        }
      })
      
      this.webSocket.onMessage((data) => {
        if (data.status === 'ok') {
          this.deviceInfo = data
        }
      })
    },

    // 连接设备 - 支持Mock设备
    async connect(deviceIdOrIp) {
      if (!this.webSocket) {
        this.init()
      }
      
      let device = null
      let ip = deviceIdOrIp
      
      // 如果传入的是设备ID，获取设备信息
      if (typeof deviceIdOrIp === 'number') {
        device = this.getDeviceById(deviceIdOrIp)
        if (device) {
          // 模拟从设备获取IP地址
          ip = `192.168.1.${100 + device.id}`
          this.currentDevice = device
          
          // 更新设备状态为连接中
          this.updateDeviceStatus(device.id, 'connecting')
          
          // 模拟连接过程
          setTimeout(() => {
            this.connected = true
            this.deviceIp = ip
            this.updateDeviceStatus(device.id, 'connected', {
              lastConnected: new Date().toLocaleString()
            })
            uni.setStorageSync('device_ip', ip)
          }, 2000)
          
          return { success: true, message: '正在连接设备...' }
        } else {
          return { success: false, error: '设备不存在' }
        }
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
        // 连接成功后获取设备信息
        await this.webSocket.getStatus()
        return { success: true }
      } catch (err) {
        console.error('连接失败:', err)
        if (device) {
          this.updateDeviceStatus(device.id, 'disconnected')
        }
        return { success: false, error: err }
      }
    },

    // 断开连接
    disconnect(deviceId = null) {
      if (this.webSocket) {
        this.webSocket.disconnect()
      }
      this.connected = false
      this.deviceInfo = null
      
      // 更新设备状态
      if (deviceId) {
        this.updateDeviceStatus(deviceId, 'disconnected')
      } else if (this.currentDevice) {
        this.updateDeviceStatus(this.currentDevice.id, 'disconnected')
      }
      
      this.currentDevice = null
    },

    // 发送图片数据到设备（画板模式）
    async sendImage(pixels, width, height) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }
      
      try {
        // 发送画布数据前必须先切换到画板模式，否则会被误存为闹钟背景图
        await this.webSocket.setMode('canvas')
        await this.webSocket.showImage(pixels, width, height)
        return { success: true }
      } catch (err) {
        console.error('发送图片失败:', err)
        return { success: false, error: err }
      }
    },
    
    // 发送稀疏像素数据到设备（画板模式，只发送有颜色的像素）
    async sendSparseImage(sparsePixels) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }
      
      try {
        // 发送画布数据前必须先切换到画板模式
        await this.webSocket.setMode('canvas')
        await this.webSocket.showSparseImage(sparsePixels)
        return { success: true }
      } catch (err) {
        console.error('发送稀疏图片失败:', err)
        return { success: false, error: err }
      }
    },
    
    // 发送部分更新（仅更新指定像素，画板模式）
    async sendPartialUpdate(pixelData) {
      if (!this.connected || !this.webSocket) {
        throw new Error('设备未连接')
      }
      
      try {
        await this.webSocket.sendPartialUpdate(pixelData)
        return { success: true }
      } catch (err) {
        console.error('发送部分更新失败:', err)
        return { success: false, error: err }
      }
    },

    // 设置亮度
    async setBrightness(value, deviceId = null) {
      // 更新Mock设备的亮度
      if (deviceId) {
        this.updateDeviceStatus(deviceId, null, { brightness: value })
      }
      
      if (!this.connected || !this.webSocket) {
        // 如果设备未连接，只更新本地状态
        return { success: true, message: '亮度设置已保存' }
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
    },
    
    // 添加新设备
    addDevice(deviceInfo) {
      const newDevice = {
        id: Date.now(),
        name: deviceInfo.name || '新设备',
        type: deviceInfo.type || '52x52',
        icon: 'electronics',
        status: 'disconnected',
        battery: 100,
        brightness: 80,
        lastConnected: null,
        macAddress: deviceInfo.macAddress || 'AA:BB:CC:DD:EE:XX',
        firmwareVersion: 'v2.1.0',
        connectionType: deviceInfo.connectionType || 'bluetooth',
        signalStrength: 0,
        temperature: 25,
        workingHours: 0
      }
      
      this.devices.push(newDevice)
      this.saveDevicesToStorage()
      return newDevice
    },
    
    // 删除设备
    removeDevice(deviceId) {
      this.devices = this.devices.filter(d => d.id !== deviceId)
      this.saveDevicesToStorage()
      
      // 如果删除的是当前设备，断开连接
      if (this.currentDevice && this.currentDevice.id === deviceId) {
        this.disconnect()
      }
    }
  }
})

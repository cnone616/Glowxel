<template>
  <view class="control-page" :class="{ 'light-theme': themeStore && !themeStore.isDarkMode }">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 头部 -->
    <view class="header">
      <view class="header-left">
        <view class="back-btn" @click="goBack">
          <text class="icon">‹</text>
        </view>
        <view class="header-info">
          <text class="header-title">设备连接</text>
          <text class="header-subtitle">ESP32 矩阵控制器</text>
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :show-scrollbar="true">
      <!-- 设备连接卡片 -->
      <view class="connection-card">
        <view class="card-title-section">
          <text class="card-title">连接设备</text>
          <text class="card-subtitle">通过 Wi-Fi 连接到 LED 矩阵板</text>
        </view>

        <view class="connection-btn-wrapper">
          <view 
            class="connection-btn"
            :class="{ 'disabled': connectionStatus === 'connected' }"
            @click="handleConnect"
          >
            <Icon name="link" :size="48" />
            <text class="connection-label">{{ connectionStatus === 'connected' ? '已连接' : '连接设备' }}</text>
          </view>
        </view>
        
        <view v-if="connectionStatus === 'connected' && deviceIp" class="device-info">
          <text class="info-label">设备地址</text>
          <text class="info-value">{{ deviceIp }}</text>
        </view>
      </view>
      
      <!-- 设备模式和功能 -->
      <view v-if="connectionStatus === 'connected'" class="settings-section">
        <!-- 模式切换 -->
        <view class="mode-switch-card">
          <view class="card-title-section">
            <text class="card-title">设备模式</text>
            <text class="card-subtitle">切换闹钟或画板模式</text>
          </view>
          
          <view class="mode-switch-buttons">
            <view 
              class="mode-btn"
              :class="{ 'active': deviceMode === 'clock' }"
              @click="switchMode('clock')"
            >
              <Icon name="time" :size="40" />
              <text class="mode-label">闹钟模式</text>
              <text class="mode-desc">显示时间日期</text>
            </view>
            
            <view 
              class="mode-btn"
              :class="{ 'active': deviceMode === 'canvas' }"
              @click="switchMode('canvas')"
            >
              <Icon name="picture" :size="40" />
              <text class="mode-label">画板模式</text>
              <text class="mode-desc">显示拼豆图案</text>
            </view>
          </view>
          
          <view v-if="deviceMode === 'clock'" class="clock-actions">
            <view class="clock-edit-btn" @click="editClock">
              <Icon name="edit" :size="32" />
              <text class="btn-text">自定义闹钟样式</text>
            </view>
            <view class="clock-edit-btn secondary" @click="importJSON">
              <Icon name="add" :size="32" />
              <text class="btn-text">导入 JSON 配置</text>
            </view>
          </view>
        </view>
        
        <!-- 亮度控制 -->
        <view class="brightness-card">
          <view class="setting-item">
            <view class="setting-header">
              <view class="setting-label">
                <view class="setting-icon">
                  <Icon name="prompt" :size="28" />
                </view>
                <text class="setting-text">亮度</text>
              </view>
              <text class="setting-value accent-primary">{{ brightness }}%</text>
            </view>
            <slider 
              :value="brightness"
              @change="handleBrightnessChange"
              min="0"
              max="100"
              :activeColor="(themeStore && themeStore.isDarkMode) ? '#00f3ff' : '#0099cc'"
              :backgroundColor="(themeStore && themeStore.isDarkMode) ? '#333333' : '#e0e0e0'"
              block-size="20"
              class="setting-slider"
            />
          </view>
        </view>
      </view>
    </scroll-view>
    
    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />
    
    <!-- 连接弹窗 -->
    <ConnectModal
      :visible.sync="showConnectModal"
      title="连接设备"
      description="请输入 LED 矩阵板的 IP 地址"
      :placeholder="deviceIp || '192.168.31.84'"
      :defaultValue="deviceIp"
      ref="connectModal"
      @confirm="handleConnectConfirm"
      @cancel="handleConnectCancel"
      @timeout="handleConnectTimeout"
    />
    
    <!-- JSON 导入弹窗 -->
    <JsonImportModal
      :visible.sync="showJsonImportModal"
      :sending="jsonImportSending"
      title="导入 JSON 配置"
      description="从 LED 模拟器复制的完整配置数据"
      placeholder="粘贴 JSON 数据..."
      @confirm="handleJsonImport"
      @error="handleJsonImportError"
    />
  </view>
</template>

<script>
import { useThemeStore } from '../../store/theme.js'
import { useDeviceStore } from '../../store/device.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'
import ConnectModal from '../../components/ConnectModal.vue'
import JsonImportModal from '../../components/JsonImportModal.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ConnectModal,
    JsonImportModal
  },
  data() {
    return {
      themeStore: null,
      deviceStore: null,
      toast: null,
      
      // 连接状态
      connectionStatus: 'disconnected', // disconnected, connecting, connected
      deviceIp: '',
      
      // 设备模式
      deviceMode: 'clock', // clock, canvas
      
      // 设置
      brightness: 50,
      
      // 连接弹窗
      showConnectModal: false,
      
      // JSON 导入弹窗
      showJsonImportModal: false,
      jsonImportSending: false
    }
  },

  onLoad(options) {
    this.themeStore = useThemeStore()
    this.deviceStore = useDeviceStore()
    this.toast = useToast()
    
    // 注册自定义 Toast 实例
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
    
    // 立即应用主题，避免闪烁
    this.themeStore.applyTheme()
    
    // 从缓存读取设备 IP
    const savedIp = uni.getStorageSync('device_ip')
    if (savedIp) {
      this.deviceIp = savedIp
    }
    
    // 从缓存读取设备模式
    const savedMode = uni.getStorageSync('device_mode')
    if (savedMode) {
      this.deviceMode = savedMode
    }
    
    // 从 URL 参数读取模式（优先级更高）
    if (options && options.mode) {
      this.deviceMode = options.mode
      uni.setStorageSync('device_mode', options.mode)
    }
    
    // 从缓存读取亮度
    const savedBrightness = uni.getStorageSync('device_brightness')
    if (savedBrightness !== undefined && savedBrightness !== null) {
      this.brightness = savedBrightness
    }
    
    // 监听设备连接状态
    this.updateConnectionStatus()
  },

  onShow() {
    this.themeStore.applyTheme()
    this.updateConnectionStatus()
  },

  computed: {
    isDeviceConnected() {
      return this.deviceStore?.connected || false
    }
  },

  methods: {
    updateConnectionStatus() {
      if (this.deviceStore?.connected) {
        this.connectionStatus = 'connected'
      } else {
        this.connectionStatus = 'disconnected'
      }
    },
    
    async handleConnect() {
      if (this.connectionStatus === 'connected') {
        // 已连接，断开
        this.deviceStore.disconnect()
        this.connectionStatus = 'disconnected'
        this.toast.showInfo('设备已断开')
        return
      }
      
      if (this.connectionStatus !== 'disconnected') {
        return
      }
      
      // 显示连接弹窗
      this.showConnectModal = true
    },
    
    async handleConnectConfirm(ip) {
      this.deviceIp = ip
      
      try {
        const result = await this.deviceStore.connect(ip)
        if (result.success) {
          this.connectionStatus = 'connected'
          
          // 连接成功后，ESP32 肯定是闹钟模式（断开连接后会回到闹钟模式）
          // 重置前端的模式状态为闹钟模式
          this.deviceMode = 'clock'
          uni.setStorageSync('device_mode', 'clock')
          
          // 获取设备状态（包括亮度）
          try {
            const ws = this.deviceStore.getWebSocket()
            const status = await ws.getStatus()
            
            if (status && status.brightness !== undefined) {
              // 将板子的亮度值（0-178）映射回前端显示（0-100）
              const maxBrightness = 178 // 255 * 0.7
              this.brightness = Math.round(status.brightness * 100 / maxBrightness)
              // 保存到缓存
              uni.setStorageSync('device_brightness', this.brightness)
              console.log('同步板子亮度:', status.brightness, '→', this.brightness + '%')
            }
          } catch (err) {
            console.error('获取设备状态失败:', err)
          }
          
          this.$refs.connectModal.onSuccess()
          this.toast.showSuccess('已连接到 LED 矩阵板')
        } else {
          this.$refs.connectModal.onError('连接失败，请检查 IP 地址')
        }
      } catch (err) {
        console.error('连接失败:', err)
        this.$refs.connectModal.onError('连接失败，请检查 IP 地址')
      }
    },
    
    handleConnectCancel() {
      // 取消连接
    },
    
    handleConnectTimeout() {
      this.toast.showError('连接超时，请重试')
    },

    async handleBrightnessChange(e) {
      this.brightness = e.detail.value
      
      // 保存到缓存
      uni.setStorageSync('device_brightness', this.brightness)
      
      // 如果已连接，发送亮度控制命令
      if (this.connectionStatus === 'connected') {
        try {
          // 将 0-100 的值映射到 0-178 (70% of 255)，限制最大亮度为70%
          const maxBrightness = 178 // 255 * 0.7
          const value = Math.round(this.brightness * maxBrightness / 100)
          await this.deviceStore.setBrightness(value)
        } catch (err) {
          console.error('设置亮度失败:', err)
        }
      }
    },
    
    async switchMode(mode) {
      if (this.deviceMode === mode) return
      
      try {
        const ws = this.deviceStore.getWebSocket()
        await ws.setMode(mode)
        this.deviceMode = mode
        
        // 保存到缓存
        uni.setStorageSync('device_mode', mode)
        
        if (mode === 'clock') {
          this.toast.showSuccess('已切换到闹钟模式')
        } else {
          this.toast.showSuccess('已切换到画板模式')
        }
      } catch (err) {
        console.error('切换模式失败:', err)
        this.toast.showError('切换模式失败')
      }
    },

    editClock() {
      uni.navigateTo({
        url: '/pages/clock-editor/clock-editor'
      })
    },
    
    importJSON() {
      this.showJsonImportModal = true
    },
    
    async handleJsonImport(jsonData) {
      try {
        // 验证 JSON 数据格式
        if (!jsonData || typeof jsonData !== 'object') {
          this.toast.showError('无效的配置数据')
          return
        }
        
        // 开始传输，禁止关闭弹窗
        this.jsonImportSending = true
        
        // 保存配置到本地存储
        uni.setStorageSync('clock_config', JSON.stringify(jsonData))
        
        // 如果有像素数据，也保存
        if (jsonData.imagePixels) {
          uni.setStorageSync('clock_image_pixels', JSON.stringify(jsonData.imagePixels))
        }
        
        // 检查设备是否连接
        if (!this.deviceStore.connected) {
          this.jsonImportSending = false
          this.showJsonImportModal = false
          this.toast.showError('设备未连接，配置已保存到本地')
          return
        }
        
        // 直接发送到设备
        try {
          const ws = this.deviceStore.getWebSocket()
          
          // 转换配置格式
          const configData = {
            time: {
              fontSize: jsonData.timeFontSize || 1,
              x: jsonData.timeX || 17,
              y: jsonData.timeY || 5,
              color: this.hexToRgb(jsonData.timeColor || '#64c8ff')
            },
            date: {
              show: jsonData.showDate !== false,
              fontSize: jsonData.dateFontSize || 1,
              x: jsonData.dateX || 3,
              y: jsonData.dateY || 30,
              color: this.hexToRgb(jsonData.dateColor || '#787878')
            },
            week: {
              show: jsonData.showWeek !== false,
              x: jsonData.weekX || 23,
              y: jsonData.weekY || 44,
              color: this.hexToRgb(jsonData.weekColor || '#646464')
            },
            image: {
              show: jsonData.showImage || false,
              x: jsonData.imageX || 0,
              y: jsonData.imageY || 0,
              width: jsonData.imageWidth || 64,
              height: jsonData.imageHeight || 64
            }
          }
          
          // 闹钟背景图必须用闹钟模式接收，先切换模式
          await ws.setMode('clock')
          
          // 发送配置
          await ws.send({ cmd: 'set_clock_config', config: configData })
          
          // 如果有像素数据，使用二进制方式发送（闹钟背景图）
          if (jsonData.imagePixels && jsonData.imagePixels.length > 0) {
            await this.sendImagePixelsBinary(jsonData.imagePixels)
            this.toast.showSuccess(`配置已发送！包含 ${jsonData.imagePixels.length} 个像素点`)
          } else {
            this.toast.showSuccess('配置已发送到设备！')
          }
          
          // 传输完成，关闭弹窗
          this.jsonImportSending = false
          this.showJsonImportModal = false
          
        } catch (err) {
          console.error('发送配置失败:', err)
          this.jsonImportSending = false
          this.showJsonImportModal = false
          this.toast.showError('发送失败：' + err.message)
        }
      } catch (e) {
        console.error('导入配置失败:', e)
        this.jsonImportSending = false
        this.showJsonImportModal = false
        this.toast.showError('导入失败：' + e.message)
      }
    },
    
    async sendImagePixelsBinary(pixels) {
      // 使用二进制方式发送像素数据
      // 格式：每个像素5字节 (x, y, r, g, b)
      const ws = this.deviceStore.getWebSocket()
      
      console.log(`开始发送像素数据，共 ${pixels.length} 个像素`)
      console.log(`前3个像素:`, pixels.slice(0, 3))
      
      // 每批 64 个像素，间隔 200ms
      const batchSize = 64
      let sentCount = 0
      
      for (let i = 0; i < pixels.length; i += batchSize) {
        const batch = pixels.slice(i, Math.min(i + batchSize, pixels.length))
        const binaryData = []
        
        for (const pixel of batch) {
          binaryData.push(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b)
        }
        
        const buffer = new Uint8Array(binaryData)
        
        // 打印第一批的前15个字节（3个像素）
        if (i === 0) {
          console.log(`第一批前15字节:`, Array.from(buffer.slice(0, 15)))
        }
        
        await new Promise((resolve, reject) => {
          ws.socket.send({
            data: buffer.buffer,
            success: () => {
              sentCount += batch.length
              if (sentCount % 100 === 0 || sentCount === pixels.length) {
                console.log(`已发送 ${sentCount}/${pixels.length} 个像素 (${Math.round(sentCount/pixels.length*100)}%)`)
              }
              resolve()
            },
            fail: (err) => {
              console.error('发送失败:', err)
              reject(err)
            }
          })
        })
        
        // 每批之间等待 200ms
        await new Promise(resolve => setTimeout(resolve, 200))
      }
      
      console.log(`✓ 像素数据发送完成，共 ${sentCount} 个像素`)
      console.log(`最后3个像素:`, pixels.slice(-3))
      
      // 等待 ESP32 确认消息（发送完成后等待1秒，ESP32需要500ms保存）
      const receivedCount = await this.waitForPixelConfirmation(1000)
      
      if (receivedCount === null) {
        console.warn('⚠ 未收到 ESP32 确认消息，无法验证数量')
        return
      }
      
      console.log(`ESP32 确认接收: ${receivedCount} 个像素`)
      
      if (receivedCount === sentCount) {
        console.log(`✓✓✓ 数量匹配！发送 ${sentCount}，接收 ${receivedCount}`)
      } else {
        console.error(`✗✗✗ 数量不匹配！发送 ${sentCount}，接收 ${receivedCount}，丢失 ${sentCount - receivedCount} 个像素`)
        throw new Error(`数据传输不完整：发送 ${sentCount} 个，接收 ${receivedCount} 个`)
      }
    },
    
    async waitForPixelConfirmation(timeout) {
      // 等待 ESP32 发送确认消息
      return new Promise((resolve) => {
        const ws = this.deviceStore.getWebSocket()
        let timeoutId = null
        
        const messageHandler = (data) => {
          if (data.status === 'ok' && data.message === 'pixels_received' && data.count !== undefined) {
            clearTimeout(timeoutId)
            ws.offMessage(messageHandler)
            resolve(data.count)
          }
        }
        
        ws.onMessage(messageHandler)
        
        timeoutId = setTimeout(() => {
          ws.offMessage(messageHandler)
          resolve(null)
        }, timeout)
      })
    },
    
    handleJsonImportError(message) {
      this.toast.showError(message)
      // 错误时不关闭弹窗，让用户可以修改
    },
    
    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
      return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
      } : { r: 255, g: 255, b: 255 }
    },

    goBack() {
      uni.navigateBack()
    }
  }
}
</script>

<style scoped>
.control-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

/* 头部 */
.header {
  flex-shrink: 0;
  height: 112rpx;
  display: flex;
  align-items: center;
  padding: 0 48rpx;
  border-bottom: 2rpx solid var(--border-primary);
  background-color: var(--bg-elevated);
}

.header-left {
  display: flex;
  align-items: center;
  gap: 32rpx;
}

.back-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.back-btn:active {
  background-color: var(--bg-tertiary);
  transform: scale(0.95);
}

.icon {
  font-size: 56rpx;
  color: var(--text-primary);
}

.header-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
}

.header-title {
  font-size: 32rpx;
  font-weight: 300;
  color: var(--text-primary);
}

.header-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

/* 内容区域 */
.content {
  flex: 1;
  padding: 32rpx;
  box-sizing: border-box;
  overflow-y: auto;
}

/* 连接卡片 */
.connection-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  padding: 48rpx;
  margin-bottom: 32rpx;
  box-shadow: var(--shadow-md);
}

.card-title-section {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  margin-bottom: 32rpx;
}

.card-title {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.card-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.connection-btn-wrapper {
  display: flex;
  justify-content: center;
  margin-bottom: 24rpx;
}

.connection-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 48rpx 80rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  box-shadow: var(--shadow-sm);
  transition: var(--transition-base);
}

.connection-btn:active:not(.disabled) {
  transform: scale(0.98);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.connection-btn.disabled {
  opacity: 0.5;
}

.connection-label {
  font-size: 26rpx;
  color: var(--text-primary);
  margin-top: 16rpx;
}

.device-info {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  padding: 24rpx;
  background-color: var(--bg-secondary);
  border-radius: 16rpx;
  border: 2rpx solid var(--border-secondary);
}

.info-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.info-value {
  font-size: 24rpx;
  font-family: monospace;
  color: var(--accent-primary);
  font-weight: 500;
}

.status-badge {
  padding: 12rpx 24rpx;
  border-radius: 999rpx;
  border: 2rpx solid;
  font-size: 22rpx;
  font-family: monospace;
  font-weight: bold;
}

.status-connected {
  background-color: rgba(0, 170, 136, 0.1);
  border-color: var(--success-color);
  box-shadow: 0 0 16rpx rgba(0, 170, 136, 0.3);
}

.status-connected .status-text {
  color: var(--success-color);
}

.status-connecting {
  background-color: rgba(238, 153, 0, 0.1);
  border-color: var(--warning-color);
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.6; }
}

.status-connecting .status-text {
  color: var(--warning-color);
}

.status-disconnected {
  background-color: var(--bg-tertiary);
  border-color: var(--border-secondary);
}

.status-disconnected .status-text {
  color: var(--text-secondary);
}

/* 设置区域 */
.settings-section {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

/* 模式切换卡片 */
.mode-switch-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  padding: 48rpx;
  box-shadow: var(--shadow-md);
}

.mode-switch-buttons {
  display: flex;
  gap: 24rpx;
}

.mode-btn {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  transition: var(--transition-base);
  gap: 12rpx;
}

.mode-btn:active {
  transform: scale(0.98);
}

.mode-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.mode-label {
  font-size: 24rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.mode-btn.active .mode-label {
  color: var(--accent-primary);
}

.mode-desc {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.clock-edit-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  padding: 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.clock-edit-btn:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.clock-edit-btn.secondary {
  border-color: var(--border-secondary);
}

.clock-actions {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
  margin-top: 24rpx;
}

.btn-text {
  font-size: 24rpx;
  color: var(--text-primary);
}

/* 亮度卡片 */
.brightness-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 32rpx;
  padding: 48rpx;
  box-shadow: var(--shadow-md);
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.setting-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.setting-label {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.setting-icon {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 12rpx;
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
}

.setting-text {
  font-size: 26rpx;
  color: var(--text-secondary);
}

.setting-value {
  font-size: 24rpx;
  font-family: monospace;
  font-weight: bold;
}

.accent-primary {
  color: var(--accent-primary);
  text-shadow: 0 0 8rpx rgba(0, 243, 255, 0.3);
}

.accent-secondary {
  color: var(--accent-secondary);
  text-shadow: 0 0 8rpx rgba(255, 0, 255, 0.3);
}

.setting-slider {
  width: 100%;
}

</style>


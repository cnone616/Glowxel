<template>
  <view class="clock-editor-page" :class="{ 'light-theme': themeStore && !themeStore.isDarkMode }">
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
          <text class="header-title">闹钟编辑器</text>
          <text class="header-subtitle">自定义时钟样式</text>
        </view>
      </view>
      <view class="header-actions">
        <view class="action-btn" @click="saveConfig">
          <Icon name="save" :size="40" />
        </view>
        <view class="action-btn primary" @click="sendToDevice">
          <Icon name="send" :size="40" />
        </view>
      </view>
    </view>

    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <!-- 预览区域 -->
        <view class="preview-card">
          <view class="card-title-section">
            <text class="card-title">实时预览</text>
            <text class="card-subtitle">64x64 像素</text>
          </view>
          <view class="preview-container">
            <canvas 
              type="2d"
              canvas-id="previewCanvas" 
              id="previewCanvas"
              class="preview-canvas"
              :style="{ width: canvasSize + 'px', height: canvasSize + 'px' }"
            ></canvas>
          </view>
        </view>

        <!-- Tab 切换 -->
        <view class="tabs-inline">
          <view 
            v-for="(tab, index) in tabs" 
            :key="index"
            class="tab-item"
            :class="{ 'active': currentTab === index }"
            @click="currentTab = index"
          >
            <text class="tab-text">{{ tab }}</text>
          </view>
        </view>

        <!-- 时间设置 -->
        <view v-show="currentTab === 0" class="settings-card">
          <view class="card-title-section">
            <Icon name="time" :size="32" />
            <text class="card-title">时间显示</text>
          </view>
          
          <view class="setting-item">
            <text class="setting-label">字体大小</text>
            <view class="setting-control">
              <text class="setting-value">{{ config.time.fontSize }}</text>
              <slider 
                :value="config.time.fontSize"
                @change="e => updateConfig('time', 'fontSize', e.detail.value)"
                min="1"
                max="3"
                step="1"
                :activeColor="accentColor"
                class="setting-slider"
              />
            </view>
          </view>

          <view class="setting-item">
            <text class="setting-label">X 位置</text>
            <view class="setting-control">
              <text class="setting-value">{{ config.time.x }}</text>
              <slider 
                :value="config.time.x"
                @change="e => updateConfig('time', 'x', e.detail.value)"
                min="0"
                max="64"
                :activeColor="accentColor"
                class="setting-slider"
              />
            </view>
          </view>

          <view class="setting-item">
            <text class="setting-label">Y 位置</text>
            <view class="setting-control">
              <text class="setting-value">{{ config.time.y }}</text>
              <slider 
                :value="config.time.y"
                @change="e => updateConfig('time', 'y', e.detail.value)"
                min="0"
                max="64"
                :activeColor="accentColor"
                class="setting-slider"
              />
            </view>
          </view>

          <view class="setting-item">
            <text class="setting-label">颜色</text>
            <view class="color-picker">
              <view 
                v-for="color in presetColors" 
                :key="color.name"
                class="color-item"
                :class="{ 'active': config.time.color === color.hex }"
                :style="{ backgroundColor: color.hex }"
                @click="updateConfig('time', 'color', color.hex)"
              ></view>
            </view>
          </view>
        </view>

        <!-- 日期设置 -->
        <view v-show="currentTab === 1" class="settings-card">
          <view class="card-title-section">
            <Icon name="calendar" :size="32" />
            <text class="card-title">日期显示</text>
            <view class="toggle-switch" @click="toggleDateShow">
              <view class="switch-track" :class="{ 'active': config.date.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
          
          <view v-if="config.date.show" class="setting-group">
            <view class="setting-item">
              <text class="setting-label">字体大小</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.date.fontSize }}</text>
                <slider 
                  :value="config.date.fontSize"
                  @change="e => updateConfig('date', 'fontSize', e.detail.value)"
                  min="1"
                  max="2"
                  step="1"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">X 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.date.x }}</text>
                <slider 
                  :value="config.date.x"
                  @change="e => updateConfig('date', 'x', e.detail.value)"
                  min="0"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.date.y }}</text>
                <slider 
                  :value="config.date.y"
                  @change="e => updateConfig('date', 'y', e.detail.value)"
                  min="0"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">颜色</text>
              <view class="color-picker">
                <view 
                  v-for="color in presetColors" 
                  :key="color.name"
                  class="color-item"
                  :class="{ 'active': config.date.color === color.hex }"
                  :style="{ backgroundColor: color.hex }"
                  @click="updateConfig('date', 'color', color.hex)"
                ></view>
              </view>
            </view>
          </view>
        </view>

        <!-- 星期设置 -->
        <view v-show="currentTab === 2" class="settings-card">
          <view class="card-title-section">
            <Icon name="calendar-days" :size="32" />
            <text class="card-title">星期显示</text>
            <view class="toggle-switch" @click="toggleWeekShow">
              <view class="switch-track" :class="{ 'active': config.week.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
          
          <view v-if="config.week.show" class="setting-group">
            <view class="setting-item">
              <text class="setting-label">X 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.week.x }}</text>
                <slider 
                  :value="config.week.x"
                  @change="e => updateConfig('week', 'x', e.detail.value)"
                  min="0"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.week.y }}</text>
                <slider 
                  :value="config.week.y"
                  @change="e => updateConfig('week', 'y', e.detail.value)"
                  min="0"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">颜色</text>
              <view class="color-picker">
                <view 
                  v-for="color in presetColors" 
                  :key="color.name"
                  class="color-item"
                  :class="{ 'active': config.week.color === color.hex }"
                  :style="{ backgroundColor: color.hex }"
                  @click="updateConfig('week', 'color', color.hex)"
                ></view>
              </view>
            </view>
          </view>
        </view>

        <!-- 预览图片 -->
        <view v-show="currentTab === 3" class="settings-card">
          <view class="card-title-section">
            <Icon name="picture" :size="32" />
            <text class="card-title">背景图片</text>
            <view class="toggle-switch" @click="toggleImageShow">
              <view class="switch-track" :class="{ 'active': config.image.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
          
          <view class="image-upload">
            <view v-if="!config.image.data" class="upload-placeholder" @click="chooseImage">
              <Icon name="add" :size="64" />
              <text class="upload-text">点击上传图片</text>
              <text class="upload-hint">建议尺寸 64x64 像素</text>
            </view>
            <view v-else class="image-preview">
              <image :src="config.image.data" class="preview-image" mode="aspectFit" />
              <view class="image-actions">
                <view class="image-action-btn" @click="chooseImage">
                  <Icon name="refresh" :size="28" />
                  <text>更换</text>
                </view>
                <view class="image-action-btn danger" @click="removeImage">
                  <Icon name="ashbin" :size="28" />
                  <text>删除</text>
                </view>
              </view>
            </view>
          </view>
          
          <view v-if="config.image.show && config.image.data" class="setting-group">
            <view class="setting-item">
              <view class="setting-header-row">
                <text class="setting-label">尺寸设置</text>
                <view class="quick-size-btn" @click="setSquareSize">
                  <text class="btn-text">长宽一致</text>
                </view>
              </view>
            </view>
            
            <view class="setting-item">
              <text class="setting-label">宽度 (最大64)</text>
              <view class="setting-control">
                <input 
                  type="number" 
                  v-model.number="config.image.width"
                  @input="handleImageSizeChange('width', $event)"
                  class="size-input"
                  :class="{ 'warning': config.image.width > 64 }"
                />
                <slider 
                  :value="config.image.width"
                  @change="e => updateConfig('image', 'width', e.detail.value)"
                  min="8"
                  max="128"
                  :activeColor="config.image.width > 64 ? '#ff6464' : accentColor"
                  class="setting-slider"
                />
              </view>
              <text v-if="config.image.width > 64" class="warning-text">⚠️ 宽度超过64可能显示不完整</text>
            </view>

            <view class="setting-item">
              <text class="setting-label">高度 (最大64)</text>
              <view class="setting-control">
                <input 
                  type="number" 
                  v-model.number="config.image.height"
                  @input="handleImageSizeChange('height', $event)"
                  class="size-input"
                  :class="{ 'warning': config.image.height > 64 }"
                />
                <slider 
                  :value="config.image.height"
                  @change="e => updateConfig('image', 'height', e.detail.value)"
                  min="8"
                  max="128"
                  :activeColor="config.image.height > 64 ? '#ff6464' : accentColor"
                  class="setting-slider"
                />
              </view>
              <text v-if="config.image.height > 64" class="warning-text">⚠️ 高度超过64可能显示不完整</text>
            </view>

            <view class="setting-item">
              <text class="setting-label">X 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.image.x }}</text>
                <slider 
                  :value="config.image.x"
                  @change="e => updateConfig('image', 'x', e.detail.value)"
                  min="-32"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>

            <view class="setting-item">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control">
                <text class="setting-value">{{ config.image.y }}</text>
                <slider 
                  :value="config.image.y"
                  @change="e => updateConfig('image', 'y', e.detail.value)"
                  min="-32"
                  max="64"
                  :activeColor="accentColor"
                  class="setting-slider"
                />
              </view>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
    
    <Toast ref="toastRef" />
  </view>
</template>

<script>
import { useThemeStore } from '../../store/theme.js'
import { useDeviceStore } from '../../store/device.js'
import { useToast } from '../../composables/useToast.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast
  },
  data() {
    return {
      themeStore: null,
      deviceStore: null,
      toast: null,
      
      canvasSize: 280,
      ctx: null,
      contentHeight: 'calc(100vh - 112rpx)',
      
      currentTab: 0,
      tabs: ['时间', '日期', '星期', '图片'],
      
      config: {
        time: {
          fontSize: 1,
          x: 17,
          y: 18,
          color: '#64c8ff'
        },
        date: {
          show: true,
          fontSize: 1,
          x: 3,
          y: 30,
          color: '#787878'
        },
        week: {
          show: true,
          x: 23,
          y: 44,
          color: '#646464'
        },
        image: {
          show: false,
          x: 0,
          y: 0,
          width: 64,
          height: 64,
          data: null
        }
      },
      
      presetColors: [
        { name: '青色', hex: '#64c8ff' },
        { name: '绿色', hex: '#00ff9d' },
        { name: '黄色', hex: '#ffdc00' },
        { name: '橙色', hex: '#ffa500' },
        { name: '红色', hex: '#ff6464' },
        { name: '紫色', hex: '#c864ff' },
        { name: '白色', hex: '#ffffff' },
        { name: '灰色', hex: '#787878' },
        { name: '深灰', hex: '#646464' }
      ]
    }
  },

  computed: {
    accentColor() {
      return this.themeStore && this.themeStore.isDarkMode ? '#00f3ff' : '#0099cc'
    },
    
    previewTime() {
      const now = new Date()
      return `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
    },
    
    previewDate() {
      const now = new Date()
      return `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')}`
    },
    
    previewWeek() {
      const weekDays = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']
      return weekDays[new Date().getDay()]
    }
  },

  onLoad() {
    this.themeStore = useThemeStore()
    this.deviceStore = useDeviceStore()
    this.toast = useToast()
    
    const systemInfo = uni.getSystemInfoSync()
    const statusBarHeight = systemInfo.statusBarHeight || 0
    const headerHeight = 56
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight}px`
    
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
      
      setTimeout(() => {
        this.initCanvas()
        this.loadConfig()
      }, 300)
    })
    
    this.themeStore.applyTheme()
  },

  onShow() {
    this.themeStore.applyTheme()
  },

  methods: {
    initCanvas() {
      // #ifdef MP-WEIXIN
      const query = uni.createSelectorQuery().in(this)
      query.select('#previewCanvas')
        .fields({ node: true, size: true })
        .exec((res) => {
          if (res && res[0] && res[0].node) {
            const canvas = res[0].node
            this.ctx = canvas.getContext('2d')
            
            const dpr = uni.getSystemInfoSync().pixelRatio
            canvas.width = 64 * dpr
            canvas.height = 64 * dpr
            this.ctx.scale(dpr, dpr)
            
            this.drawPreview()
          } else {
            console.error('Canvas 初始化失败')
          }
        })
      // #endif
      
      // #ifndef MP-WEIXIN
      this.ctx = uni.createCanvasContext('previewCanvas', this)
      this.drawPreview()
      // #endif
    },

    drawPreview() {
      if (!this.ctx) {
        console.log('ctx 未初始化')
        return
      }
      
      const ctx = this.ctx
      
      // 清空画布
      ctx.fillStyle = '#000000'
      ctx.fillRect(0, 0, 64, 64)
      
      // 如果有背景图片，先绘制图片
      if (this.config.image.show && this.config.image.data) {
        // #ifdef MP-WEIXIN
        // 微信小程序使用 canvas.createImage()
        const query = uni.createSelectorQuery().in(this)
        query.select('#previewCanvas')
          .fields({ node: true })
          .exec((res) => {
            if (res && res[0] && res[0].node) {
              const canvas = res[0].node
              const img = canvas.createImage()
              img.onload = () => {
                ctx.drawImage(
                  img, 
                  this.config.image.x, 
                  this.config.image.y, 
                  this.config.image.width, 
                  this.config.image.height
                )
                // 图片加载完成后再绘制文字
                this.drawText(ctx)
              }
              img.onerror = (err) => {
                console.error('图片加载失败:', err)
                this.drawText(ctx)
              }
              img.src = this.config.image.data
            }
          })
        // #endif
        
        // #ifndef MP-WEIXIN
        // 非微信小程序使用 Image
        const img = new Image()
        img.onload = () => {
          ctx.drawImage(
            img, 
            this.config.image.x, 
            this.config.image.y, 
            this.config.image.width, 
            this.config.image.height
          )
          this.drawText(ctx)
        }
        img.src = this.config.image.data
        // #endif
      } else {
        // 没有图片，直接绘制文字
        this.drawText(ctx)
      }
    },
    
    drawText(ctx) {
      const now = new Date()
      const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
      const dateStr = `${now.getMonth() + 1}-${String(now.getDate()).padStart(2, '0')}`
      const weekDays = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']
      const weekStr = weekDays[now.getDay()]
      
      // 设置文本基线
      ctx.textBaseline = 'top'
      
      // 绘制时间
      const timeFontSize = this.config.time.fontSize * 8
      ctx.font = `${timeFontSize}px Arial`
      ctx.fillStyle = this.config.time.color
      ctx.fillText(timeStr, this.config.time.x, this.config.time.y)
      
      // 绘制日期
      if (this.config.date.show) {
        const dateFontSize = this.config.date.fontSize * 6
        ctx.font = `${dateFontSize}px Arial`
        ctx.fillStyle = this.config.date.color
        ctx.fillText(dateStr, this.config.date.x, this.config.date.y)
      }
      
      // 绘制星期
      if (this.config.week.show) {
        ctx.font = '8px Arial'
        ctx.fillStyle = this.config.week.color
        ctx.fillText(weekStr, this.config.week.x, this.config.week.y)
      }
      
      // #ifndef MP-WEIXIN
      // 非微信小程序需要调用 draw() 来渲染
      ctx.draw()
      // #endif
    },

    updateConfig(section, key, value) {
      this.config[section][key] = value
      this.drawPreview()
    },
    
    handleImageSizeChange(dimension, event) {
      const value = parseInt(event.detail.value) || 0
      this.config.image[dimension] = value
      
      if (value > 64) {
        this.toast.showWarning(`${dimension === 'width' ? '宽度' : '高度'}超过64，可能显示不完整`)
      }
      
      this.drawPreview()
    },
    
    setSquareSize() {
      // 取宽高中较小的值，确保不超过64
      const size = Math.min(this.config.image.width, this.config.image.height, 64)
      this.config.image.width = size
      this.config.image.height = size
      this.toast.showSuccess(`已设置为 ${size}x${size}`)
      this.drawPreview()
    },

    toggleDateShow() {
      this.config.date.show = !this.config.date.show
      this.drawPreview()
    },

    toggleWeekShow() {
      this.config.week.show = !this.config.week.show
      this.drawPreview()
    },
    
    toggleImageShow() {
      this.config.image.show = !this.config.image.show
      this.drawPreview()
    },
    
    chooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ['compressed'],
        sourceType: ['album', 'camera'],
        success: (res) => {
          const tempFilePath = res.tempFilePaths[0]
          
          uni.getFileSystemManager().readFile({
            filePath: tempFilePath,
            encoding: 'base64',
            success: (fileRes) => {
              this.config.image.data = 'data:image/png;base64,' + fileRes.data
              this.config.image.show = true
              this.drawPreview()
              this.toast.showSuccess('图片已上传')
            },
            fail: (err) => {
              console.error('读取图片失败:', err)
              this.toast.showError('图片读取失败')
            }
          })
        },
        fail: (err) => {
          console.error('选择图片失败:', err)
        }
      })
    },
    
    removeImage() {
      this.config.image.data = null
      this.config.image.show = false
      this.drawPreview()
      this.toast.showInfo('图片已删除')
    },

    saveConfig() {
      uni.setStorageSync('clock_config', JSON.stringify(this.config))
      this.toast.showSuccess('配置已保存')
    },

    loadConfig() {
      const saved = uni.getStorageSync('clock_config')
      if (saved) {
        try {
          this.config = JSON.parse(saved)
          this.drawPreview()
        } catch (e) {
          console.error('加载配置失败:', e)
        }
      }
    },

    async sendToDevice() {
      if (!this.deviceStore.connected) {
        this.toast.showError('设备未连接')
        return
      }
      
      try {
        const ws = this.deviceStore.getWebSocket()
        
        const configData = {
          time: {
            fontSize: this.config.time.fontSize,
            x: this.config.time.x,
            y: this.config.time.y,
            color: this.hexToRgb(this.config.time.color)
          },
          date: {
            show: this.config.date.show,
            fontSize: this.config.date.fontSize,
            x: this.config.date.x,
            y: this.config.date.y,
            color: this.hexToRgb(this.config.date.color)
          },
          week: {
            show: this.config.week.show,
            x: this.config.week.x,
            y: this.config.week.y,
            color: this.hexToRgb(this.config.week.color)
          }
        }
        
        await ws.send({ cmd: 'set_clock_config', config: configData })
        this.toast.showSuccess('配置已发送到设备')
      } catch (err) {
        console.error('发送配置失败:', err)
        this.toast.showError('发送失败')
      }
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
.clock-editor-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  overflow: hidden;
}

.header {
  height: 112rpx;
  display: flex;
  align-items: center;
  justify-content: space-between;
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
}

.header-actions {
  display: flex;
  gap: 16rpx;
}

.action-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn:active {
  transform: scale(0.95);
  border-color: var(--accent-primary);
  box-shadow: var(--shadow-glow);
}

.action-btn.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.content {
  width: 100%;
  box-sizing: border-box;
}

.content-wrapper {
  padding: 24rpx;
  padding-bottom: 48rpx;
}

.tabs-inline {
  display: flex;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  padding: 8rpx;
  margin-bottom: 24rpx;
  gap: 8rpx;
}

.tabs-inline .tab-item {
  flex: 1;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  transition: var(--transition-base);
  border-radius: 16rpx;
}

.tabs-inline .tab-item:active {
  background-color: var(--bg-secondary);
}

.tabs-inline .tab-item.active {
  background-color: var(--bg-secondary);
  box-shadow: var(--shadow-sm);
}

.tabs-inline .tab-text {
  font-size: 24rpx;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.tabs-inline .tab-item.active .tab-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.preview-card,
.settings-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 24rpx;
  padding: 32rpx;
  margin-bottom: 24rpx;
  box-shadow: var(--shadow-md);
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 12rpx;
  margin-bottom: 24rpx;
}

.card-title {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--text-primary);
  flex: 1;
}

.card-subtitle {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.preview-container {
  display: flex;
  justify-content: center;
  padding: 24rpx;
  background-color: var(--bg-secondary);
  border-radius: 16rpx;
  border: 2rpx solid var(--border-secondary);
}

.preview-canvas {
  border: 2rpx solid var(--border-primary);
  border-radius: 8rpx;
  display: block;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 20rpx;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.quick-size-btn {
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--accent-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.quick-size-btn:active {
  transform: scale(0.95);
  background-color: rgba(0, 243, 255, 0.1);
}

.quick-size-btn .btn-text {
  font-size: 22rpx;
  color: var(--accent-primary);
  font-weight: 500;
}

.size-input {
  width: 120rpx;
  height: 56rpx;
  padding: 0 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-secondary);
  border-radius: 12rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  text-align: center;
  font-family: monospace;
  transition: var(--transition-base);
}

.size-input:focus {
  border-color: var(--accent-primary);
}

.size-input.warning {
  border-color: #ff6464;
  background-color: rgba(255, 100, 100, 0.1);
}

.warning-text {
  font-size: 20rpx;
  color: #ff6464;
  margin-top: 4rpx;
}

.setting-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.setting-control {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.setting-value {
  font-size: 22rpx;
  font-family: monospace;
  color: var(--accent-primary);
  min-width: 48rpx;
  text-align: right;
}

.setting-slider {
  flex: 1;
}

.color-picker {
  display: flex;
  gap: 12rpx;
  flex-wrap: wrap;
}

.color-item {
  width: 64rpx;
  height: 64rpx;
  border-radius: 16rpx;
  border: 4rpx solid transparent;
  transition: var(--transition-base);
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.2);
}

.color-item:active {
  transform: scale(0.95);
}

.color-item.active {
  border-color: var(--accent-primary);
  box-shadow: 0 0 20rpx var(--accent-primary);
  transform: scale(1.05);
}

.toggle-switch {
  margin-left: auto;
}

.switch-track {
  width: 80rpx;
  height: 44rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 22rpx;
  position: relative;
  transition: var(--transition-base);
}

.switch-track.active {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
}

.switch-thumb {
  width: 36rpx;
  height: 36rpx;
  background-color: var(--text-primary);
  border-radius: 18rpx;
  position: absolute;
  top: 2rpx;
  left: 2rpx;
  transition: var(--transition-base);
}

.switch-track.active .switch-thumb {
  left: 38rpx;
  background-color: #ffffff;
}

.image-upload {
  margin-bottom: 24rpx;
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 64rpx 32rpx;
  background-color: var(--bg-secondary);
  border: 4rpx dashed var(--border-secondary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.upload-placeholder:active {
  border-color: var(--accent-primary);
  background-color: var(--bg-tertiary);
}

.upload-text {
  font-size: 26rpx;
  color: var(--text-primary);
  margin-top: 16rpx;
}

.upload-hint {
  font-size: 22rpx;
  color: var(--text-tertiary);
  margin-top: 8rpx;
}

.image-preview {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.preview-image {
  width: 100%;
  height: 400rpx;
  background-color: var(--bg-secondary);
  border-radius: 16rpx;
  border: 2rpx solid var(--border-secondary);
}

.image-actions {
  display: flex;
  gap: 16rpx;
}

.image-action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8rpx;
  padding: 20rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 24rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.image-action-btn:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
}

.image-action-btn.danger {
  border-color: rgba(239, 68, 68, 0.3);
  color: var(--error-color);
}

.image-action-btn.danger:active {
  background-color: rgba(239, 68, 68, 0.1);
}
</style>

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

    <!-- Canvas 预览区域 -->
    <view class="canvas-section">
      <view class="canvas-wrapper">
        <canvas 
          type="2d"
          canvas-id="clockCanvas" 
          id="clockCanvas"
          class="clock-canvas"
          @touchstart="handleTouchStart"
          @touchmove="handleTouchMove"
          @touchend="handleTouchEnd"
        ></canvas>
      </view>
    </view>

    <!-- 主内容：当前 Tab 的表单 -->
    <scroll-view scroll-y class="content" :style="{ height: contentHeight }">
      <view class="content-wrapper">
        <!-- 时间设置 -->
        <view v-show="currentTab === 0" class="settings-card">
          <view class="card-title-section">
            <Icon name="time" :size="32" />
            <text class="card-title">时间显示</text>
            <view class="toggle-switch" @click="toggleTimeShow">
              <view class="switch-track" :class="{ 'active': config.time.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
          
          <view v-if="config.time.show" class="setting-group">
            <view class="setting-item-row">
              <text class="setting-label">字体大小</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('time', 'fontSize', -1, 1, 3)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.time.fontSize }}</text>
                <view class="control-btn" @click="adjustValue('time', 'fontSize', 1, 1, 3)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">X 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('time', 'x', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.time.x }}</text>
                <view class="control-btn" @click="adjustValue('time', 'x', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('time', 'y', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.time.y }}</text>
                <view class="control-btn" @click="adjustValue('time', 'y', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
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
                  @click="updateConfigAndRedraw('time', 'color', color.hex)"
                ></view>
              </view>
            </view>
            
            <view class="setting-item">
              <text class="setting-label">对齐方式</text>
              <view class="align-buttons">
                <view 
                  class="align-btn"
                  :class="{ 'active': config.time.align === 'left' }"
                  @click="updateConfigAndRedraw('time', 'align', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.time.align === 'center' }"
                  @click="updateConfigAndRedraw('time', 'align', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.time.align === 'right' }"
                  @click="updateConfigAndRedraw('time', 'align', 'right')"
                >
                  <text class="align-text">右对齐</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <!-- 日期设置 -->
        <view v-show="currentTab === 3" class="settings-card">
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
            <view class="setting-item-row">
              <text class="setting-label">字体大小</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('date', 'fontSize', -1, 1, 2)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.date.fontSize }}</text>
                <view class="control-btn" @click="adjustValue('date', 'fontSize', 1, 1, 2)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">X 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('date', 'x', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.date.x }}</text>
                <view class="control-btn" @click="adjustValue('date', 'x', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('date', 'y', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.date.y }}</text>
                <view class="control-btn" @click="adjustValue('date', 'y', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
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
                  @click="updateConfigAndRedraw('date', 'color', color.hex)"
                ></view>
              </view>
            </view>
            
            <view class="setting-item">
              <text class="setting-label">对齐方式</text>
              <view class="align-buttons">
                <view 
                  class="align-btn"
                  :class="{ 'active': config.date.align === 'left' }"
                  @click="updateConfigAndRedraw('date', 'align', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.date.align === 'center' }"
                  @click="updateConfigAndRedraw('date', 'align', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.date.align === 'right' }"
                  @click="updateConfigAndRedraw('date', 'align', 'right')"
                >
                  <text class="align-text">右对齐</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <!-- 星期设置 -->
        <view v-show="currentTab === 4" class="settings-card">
          <view class="card-title-section">
            <Icon name="rili5" :size="32" />
            <text class="card-title">星期显示</text>
            <view class="toggle-switch" @click="toggleWeekShow">
              <view class="switch-track" :class="{ 'active': config.week.show }">
                <view class="switch-thumb"></view>
              </view>
            </view>
          </view>
          
          <view v-if="config.week.show" class="setting-group">
            <view class="setting-item-row">
              <text class="setting-label">X 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('week', 'x', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.week.x }}</text>
                <view class="control-btn" @click="adjustValue('week', 'x', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>

            <view class="setting-item-row">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustValue('week', 'y', -1, 0, 64)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.week.y }}</text>
                <view class="control-btn" @click="adjustValue('week', 'y', 1, 0, 64)">
                  <text class="control-icon">+</text>
                </view>
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
                  @click="updateConfigAndRedraw('week', 'color', color.hex)"
                ></view>
              </view>
            </view>
            
            <view class="setting-item">
              <text class="setting-label">对齐方式</text>
              <view class="align-buttons">
                <view 
                  class="align-btn"
                  :class="{ 'active': config.week.align === 'left' }"
                  @click="updateConfigAndRedraw('week', 'align', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.week.align === 'center' }"
                  @click="updateConfigAndRedraw('week', 'align', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.week.align === 'right' }"
                  @click="updateConfigAndRedraw('week', 'align', 'right')"
                >
                  <text class="align-text">右对齐</text>
                </view>
              </view>
            </view>
          </view>
        </view>

        <!-- 背景图片 -->
        <view v-show="currentTab === 2" class="settings-card">
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
              <text class="upload-hint">建议尺寸 52x52 像素</text>
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
          
          <!-- 图片尺寸和位置控制 -->
          <view v-if="config.image.data" class="setting-group">
            <view class="setting-item">
              <view class="setting-header-row">
                <text class="setting-label">尺寸设置</text>
                <view class="quick-size-btn" @click="setSquareSize">
                  <text class="btn-text">长宽一致</text>
                </view>
              </view>
            </view>
            
            <view class="setting-item-row">
              <text class="setting-label">宽度</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('width', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large" :class="{ 'warning-color': config.image.width > 64 }">{{ config.image.width }}</text>
                <view class="control-btn" @click="adjustImageValue('width', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.width > 64" class="warning-text">⚠️ 宽度超过64可能显示不完整</text>
            
            <view class="setting-item-row">
              <text class="setting-label">高度</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('height', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large" :class="{ 'warning-color': config.image.height > 64 }">{{ config.image.height }}</text>
                <view class="control-btn" @click="adjustImageValue('height', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.height > 64" class="warning-text">⚠️ 高度超过64可能显示不完整</text>
            
            <view class="setting-item-row">
              <text class="setting-label">X 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('x', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.image.x }}</text>
                <view class="control-btn" @click="adjustImageValue('x', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            
            <view class="setting-item-row">
              <text class="setting-label">Y 位置</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('y', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large">{{ config.image.y }}</text>
                <view class="control-btn" @click="adjustImageValue('y', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
          </view>
        </view>
        
        <!-- 绘制工具 -->
        <view v-show="currentTab === 1" class="settings-card">
          <view class="card-title-section">
            <Icon name="edit" :size="32" />
            <text class="card-title">绘制工具</text>
          </view>
          
          <!-- 工具选择 -->
          <view class="tool-selection">
            <view 
              class="tool-option" 
              :class="{ active: currentTool === 'pencil' }"
              @click="currentTool = 'pencil'"
            >
              <Icon name="edit" :size="48" />
              <text class="tool-name">画笔</text>
            </view>
            <view 
              class="tool-option" 
              :class="{ active: currentTool === 'eraser' }"
              @click="currentTool = 'eraser'"
            >
              <Icon name="ashbin" :size="48" />
              <text class="tool-name">橡皮擦</text>
            </view>
          </view>
          
          <!-- 当前颜色显示 -->
          <view class="current-color-section">
            <text class="setting-label">当前颜色</text>
            <view class="current-color-display">
              <view class="current-color-box" :style="{ backgroundColor: selectedColor }"></view>
              <text class="current-color-hex">{{ selectedColor }}</text>
            </view>
          </view>
          
          <!-- 常用颜色 -->
          <view class="setting-item">
            <text class="setting-label">常用颜色</text>
            <view class="color-picker">
              <view 
                v-for="color in commonColors" 
                :key="color.hex"
                class="color-item"
                :class="{ 'active': selectedColor === color.hex }"
                :style="{ backgroundColor: color.hex }"
                @click="selectedColor = color.hex"
              ></view>
            </view>
          </view>
          
          <!-- 完整调色板切换 -->
          <view class="palette-toggle" @click="showColorPicker = !showColorPicker">
            <text class="palette-toggle-text">
              {{ showColorPicker ? '收起' : '展开' }}完整调色板 (221色)
            </text>
            <text class="palette-toggle-icon">{{ showColorPicker ? '▲' : '▼' }}</text>
          </view>
          
          <!-- 完整调色板 -->
          <view v-if="showColorPicker" class="full-color-palette">
            <view 
              v-for="color in allColors" 
              :key="color.hex"
              class="palette-color-item"
              :class="{ 'active': selectedColor === color.hex }"
              :style="{ backgroundColor: color.hex }"
              @click="selectedColor = color.hex"
            ></view>
          </view>
          
          <!-- 操作按钮 -->
          <view class="action-buttons">
            <view class="action-button" @click="togglePreview">
              <Icon :name="showPreview ? 'eye-close' : 'eye'" :size="40" />
              <text>{{ showPreview ? '隐藏预览' : '显示预览' }}</text>
            </view>
            <view class="action-button danger" @click="clearCanvas">
              <Icon name="delete" :size="40" />
              <text>清空画布</text>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>

    <!-- 底部 Tab 切换 -->
    <view class="bottom-tabs">
      <view 
        v-for="(tab, index) in tabs" 
        :key="index"
        class="bottom-tab-item"
        :class="{ 'active': currentTab === index }"
        @click="currentTab = index"
      >
        <Icon 
          :name="tabIconNames[index]" 
          :size="36" 
          :color="currentTab === index ? accentColor : 'var(--text-secondary)'"
        />
        <text class="bottom-tab-text">{{ tab }}</text>
      </view>
    </view>
    
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
import { 
  font5x7, 
  hexToRgb, 
  drawCharToPixels, 
  drawTextToPixels,
  getCurrentTimeText,
  getCurrentDateText,
  getCurrentWeekText
} from '../../utils/clockCanvas.js'
import { COLOR_PALETTE_221, COMMON_COLORS } from '../../utils/colorPalette.js'

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
      isReady: false,
      
      contentHeight: 'calc(100vh - 112rpx - 120rpx - 80rpx)',
      
      // Canvas 相关
      canvasNode: null,
      canvasCtx: null,
      canvasPixels: new Map(), // 用户手绘的像素数据
      imagePixels: null, // 背景图片的像素数据
      currentTool: 'pencil', // pencil, eraser
      selectedColor: '#64c8ff',
      showPreview: true, // 是否显示时间日期等预览
      showColorPicker: false, // 是否显示完整调色板
      
      // 调色板
      allColors: COLOR_PALETTE_221,
      commonColors: COMMON_COLORS,
      
      currentTab: 0,
      tabs: ['时间', '绘制', '图片', '日期', '星期'],
      tabIconNames: ['time', 'edit', 'picture', 'calendar', 'rili5'],
      
      config: {
        time: {
          show: true,
          fontSize: 1,
          x: 32,  // 居中位置
          y: 18,
          color: '#64c8ff',
          align: 'center' // 居中对齐
        },
        date: {
          show: true,
          fontSize: 1,
          x: 32,  // 居中位置
          y: 30,
          color: '#787878',
          align: 'center' // 居中对齐
        },
        week: {
          show: true,
          x: 32,  // 居中位置
          y: 44,
          color: '#646464',
          align: 'center' // 居中对齐
        },
        image: {
          show: false,
          x: 6,
          y: 6,
          width: 52,
          height: 52,
          data: null
        }
      },
      
      drawColors: [
        { name: '青色', hex: '#64c8ff' },
        { name: '绿色', hex: '#00ff9d' },
        { name: '黄色', hex: '#ffdc00' },
        { name: '橙色', hex: '#ffa500' },
        { name: '红色', hex: '#ff6464' },
        { name: '紫色', hex: '#c864ff' },
        { name: '白色', hex: '#ffffff' },
        { name: '灰色', hex: '#787878' }
      ],
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
    }
  },

  onLoad() {
    // 先加载配置，确保 config 有值
    this.loadConfig()
    
    this.themeStore = useThemeStore()
    this.deviceStore = useDeviceStore()
    this.toast = useToast()
    
    const systemInfo = uni.getSystemInfoSync()
    const statusBarHeight = systemInfo.statusBarHeight || 0
    const headerHeight = 56
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight - 300}px`
    
    this.themeStore.applyTheme()
    
    // 延迟初始化 canvas，确保 DOM 已渲染
    this.$nextTick(() => {
      setTimeout(() => {
        this.initCanvas().then(() => {
          this.$nextTick(() => {
            if (this.$refs.toastRef) {
              this.toast.setToastInstance(this.$refs.toastRef)
            }
            this.isReady = true
            console.log('页面初始化完成，Canvas 已就绪')
          })
        }).catch(err => {
          console.error('Canvas 初始化失败:', err)
        })
      }, 100)
    })
  },

  onShow() {
    this.themeStore.applyTheme()
  },

  methods: {
    // ========== Canvas 相关方法 ==========
    initCanvas() {
      return new Promise((resolve, reject) => {
        const query = uni.createSelectorQuery().in(this)
        query.select('#clockCanvas')
          .fields({ node: true, size: true })
          .exec((res) => {
            if (res && res[0]) {
              const canvas = res[0].node
              this.canvasNode = canvas
              this.canvasCtx = canvas.getContext('2d')
              
              // 设置 canvas 实际大小
              const dpr = uni.getSystemInfoSync().pixelRatio || 1
              canvas.width = 320 * dpr
              canvas.height = 320 * dpr
              this.canvasCtx.scale(dpr, dpr)
              
              console.log('Canvas 初始化成功')
              
              // Canvas 初始化后绘制（配置已经在 onLoad 中加载）
              this.drawCanvas()
              
              resolve()
            } else {
              console.error('Canvas 节点获取失败')
              reject(new Error('Canvas 节点获取失败'))
            }
          })
      })
    },
    
    drawCanvas() {
      if (!this.canvasCtx || !this.canvasNode) return
      
      const ctx = this.canvasCtx
      const pixelSize = 5 // 每个像素的显示大小
      
      // 清空画布
      ctx.fillStyle = '#000000'
      ctx.fillRect(0, 0, 320, 320)
      
      // 绘制网格 - 更清晰的颜色
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)'
      ctx.lineWidth = 0.5
      for (let i = 0; i <= 64; i++) {
        ctx.beginPath()
        ctx.moveTo(i * pixelSize, 0)
        ctx.lineTo(i * pixelSize, 64 * pixelSize)
        ctx.stroke()
        
        ctx.beginPath()
        ctx.moveTo(0, i * pixelSize)
        ctx.lineTo(64 * pixelSize, i * pixelSize)
        ctx.stroke()
      }
      
      // 合并所有像素数据
      const allPixels = this.getAllPixels()
      
      // 绘制像素
      allPixels.forEach((color, key) => {
        const [x, y] = key.split(',').map(Number)
        ctx.fillStyle = color
        ctx.fillRect(x * pixelSize, y * pixelSize, pixelSize, pixelSize)
      })
    },
    
    getAllPixels() {
      const allPixels = new Map()
      
      // 1. 背景图片像素（只要有 imagePixels 就显示，不需要 image.data）
      if (this.imagePixels && this.imagePixels.size > 0) {
        this.imagePixels.forEach((color, key) => {
          allPixels.set(key, color)
        })
      }
      
      // 2. 用户手绘的像素
      this.canvasPixels.forEach((color, key) => {
        allPixels.set(key, color)
      })
      
      // 3. 时间日期等文字（如果开启预览）
      if (this.showPreview) {
        const previewPixels = this.getPreviewPixels()
        previewPixels.forEach((color, key) => {
          allPixels.set(key, color)
        })
      }
      
      return allPixels
    },
    
    getPreviewPixels() {
      const pixels = new Map()
      
      // 时间
      if (this.config.time.show) {
        const timeText = getCurrentTimeText()
        drawTextToPixels(
          timeText, 
          this.config.time.x, 
          this.config.time.y, 
          this.config.time.color, 
          this.config.time.fontSize, 
          pixels,
          this.config.time.align || 'left'
        )
      }
      
      // 日期
      if (this.config.date.show) {
        const dateText = getCurrentDateText()
        drawTextToPixels(
          dateText, 
          this.config.date.x, 
          this.config.date.y, 
          this.config.date.color, 
          this.config.date.fontSize, 
          pixels,
          this.config.date.align || 'left'
        )
      }
      
      // 星期
      if (this.config.week.show) {
        const weekText = getCurrentWeekText()
        drawTextToPixels(
          weekText, 
          this.config.week.x, 
          this.config.week.y, 
          this.config.week.color, 
          1, 
          pixels,
          this.config.week.align || 'left'
        )
      }
      
      return pixels
    },
    
    handleTouchStart(e) {
      // 只有在绘制 tab 才能绘画
      if (this.currentTab !== 4) return
      
      if (this.currentTool === 'pencil' || this.currentTool === 'eraser') {
        this.handleDraw(e)
      }
    },
    
    handleTouchMove(e) {
      // 只有在绘制 tab 才能绘画
      if (this.currentTab !== 4) return
      
      if (this.currentTool === 'pencil' || this.currentTool === 'eraser') {
        this.handleDraw(e)
      }
    },
    
    handleTouchEnd(e) {
      // 触摸结束
    },
    
    handleDraw(e) {
      const touch = e.touches[0]
      const pixelSize = 5
      
      // 获取 canvas 的位置
      const query = uni.createSelectorQuery().in(this)
      query.select('.clock-canvas').boundingClientRect(rect => {
        if (!rect) return
        
        const x = Math.floor((touch.x - rect.left) / pixelSize)
        const y = Math.floor((touch.y - rect.top) / pixelSize)
        
        if (x >= 0 && x < 64 && y >= 0 && y < 64) {
          const key = `${x},${y}`
          
          if (this.currentTool === 'pencil') {
            this.canvasPixels.set(key, this.selectedColor)
          } else if (this.currentTool === 'eraser') {
            this.canvasPixels.delete(key)
          }
          
          this.drawCanvas()
        }
      }).exec()
    },
    
    clearCanvas() {
      uni.showModal({
        title: '确认清空',
        content: '确定要清空画布吗？',
        success: (res) => {
          if (res.confirm) {
            this.canvasPixels.clear()
            this.drawCanvas()
            this.toast.showSuccess('画布已清空')
          }
        }
      })
    },
    
    togglePreview() {
      this.showPreview = !this.showPreview
      this.drawCanvas()
    },
    
    // ========== 配置更新方法 ==========
    adjustValue(section, key, delta, min, max) {
      const currentValue = this.config[section][key]
      const newValue = Math.max(min, Math.min(max, currentValue + delta))
      
      if (newValue !== currentValue) {
        this.config[section][key] = newValue
        this.drawCanvas()
      }
    },
    
    adjustImageValue(key, delta) {
      const limits = {
        width: { min: 8, max: 128 },
        height: { min: 8, max: 128 },
        x: { min: -32, max: 64 },
        y: { min: -32, max: 64 }
      }
      
      const { min, max } = limits[key]
      const currentValue = this.config.image[key]
      const newValue = Math.max(min, Math.min(max, currentValue + delta))
      
      if (newValue !== currentValue) {
        this.config.image[key] = newValue
        
        // 如果改变了尺寸，需要重新转换图片
        if ((key === 'width' || key === 'height') && this.config.image.data) {
          this.convertImageToPixels(this.config.image.data)
        } else {
          // 如果只是改变位置，重新计算像素位置
          if ((key === 'x' || key === 'y') && this.config.image.data) {
            this.convertImageToPixels(this.config.image.data)
          } else {
            this.drawCanvas()
          }
        }
        
        // 显示警告
        if ((key === 'width' || key === 'height') && newValue > 64) {
          this.toast.showWarning(`${key === 'width' ? '宽度' : '高度'}超过64，可能显示不完整`)
        }
      }
    },
    
    setSquareSize() {
      const size = Math.min(this.config.image.width, this.config.image.height, 64)
      this.config.image.width = size
      this.config.image.height = size
      
      if (this.config.image.data) {
        this.convertImageToPixels(this.config.image.data)
      }
      
      this.toast.showSuccess(`已设置为 ${size}x${size}`)
    },
    
    updateConfig(section, key, value) {
      this.config[section][key] = value
    },
    
    updateConfigAndRedraw(section, key, value) {
      this.config[section][key] = value
      this.drawCanvas()
    },
    
    toggleTimeShow() {
      this.config.time.show = !this.config.time.show
      this.drawCanvas()
    },
    
    handleImageSizeChange(dimension, event) {
      const value = parseInt(event.detail.value) || 0
      this.config.image[dimension] = value
      
      if (value > 64) {
        this.toast.showWarning(`${dimension === 'width' ? '宽度' : '高度'}超过64，可能显示不完整`)
      }
      this.drawCanvas()
    },
    
    setSquareSize() {
      const size = Math.min(this.config.image.width, this.config.image.height, 64)
      this.config.image.width = size
      this.config.image.height = size
      this.toast.showSuccess(`已设置为 ${size}x${size}`)
      this.drawCanvas()
    },

    toggleDateShow() {
      this.config.date.show = !this.config.date.show
      this.drawCanvas()
    },

    toggleWeekShow() {
      this.config.week.show = !this.config.week.show
      this.drawCanvas()
    },
    
    toggleImageShow() {
      this.config.image.show = !this.config.image.show
      this.drawCanvas()
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
              const imageData = 'data:image/png;base64,' + fileRes.data
              this.config.image.data = imageData
              this.config.image.show = true
              
              // 立即转换图片为像素数据
              this.convertImageToPixels(imageData)
              
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
    
    convertImageToPixels(imageData) {
      const targetWidth = this.config.image.width
      const targetHeight = this.config.image.height
      
      console.log('开始转换图片为像素数据，目标尺寸:', targetWidth, 'x', targetHeight)
      
      // 重新查询 canvas 节点
      const query = uni.createSelectorQuery().in(this)
      query.select('#clockCanvas')
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0] || !res[0].node) {
            console.error('Canvas 节点查询失败')
            this.toast.showError('Canvas 未就绪')
            return
          }
          
          const canvas = res[0].node
          const ctx = canvas.getContext('2d')
          
          // 创建图片对象
          const img = canvas.createImage()
          
          img.onload = () => {
            console.log('图片加载成功，原始尺寸:', img.width, 'x', img.height)
            try {
              // 临时设置 canvas 为目标尺寸
              const originalWidth = canvas.width
              const originalHeight = canvas.height
              canvas.width = targetWidth
              canvas.height = targetHeight
              
              // 关闭抗锯齿
              ctx.imageSmoothingEnabled = false
              
              // 绘制缩放后的图片
              ctx.drawImage(img, 0, 0, targetWidth, targetHeight)
              
              // 获取像素数据
              const imgData = ctx.getImageData(0, 0, targetWidth, targetHeight)
              const pixels = imgData.data
              
              // 恢复 canvas 原始尺寸
              canvas.width = originalWidth
              canvas.height = originalHeight
              const dpr = uni.getSystemInfoSync().pixelRatio || 1
              ctx.scale(dpr, dpr)
              
              // 转换为像素 Map
              const pixelMap = new Map()
              for (let y = 0; y < targetHeight; y++) {
                for (let x = 0; x < targetWidth; x++) {
                  const idx = (y * targetWidth + x) * 4
                  const r = pixels[idx]
                  const g = pixels[idx + 1]
                  const b = pixels[idx + 2]
                  const a = pixels[idx + 3]
                  
                  // 跳过完全透明的像素
                  if (a > 10) {
                    const finalX = this.config.image.x + x
                    const finalY = this.config.image.y + y
                    
                    // 只保存在有效范围内的像素
                    if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                      const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                      pixelMap.set(`${finalX},${finalY}`, hex)
                    }
                  }
                }
              }
              
              this.imagePixels = pixelMap
              console.log('图片转换完成，像素数量:', pixelMap.size, '/ 总数:', targetWidth * targetHeight)
              
              // 重新绘制 canvas
              this.drawCanvas()
            } catch (err) {
              console.error('提取像素失败:', err)
              this.toast.showError('图片处理失败')
            }
          }
          
          img.onerror = (err) => {
            console.error('图片加载失败:', err)
            this.toast.showError('图片加载失败')
          }
          
          img.src = imageData
        })
    },
    
    processImageToPixels(imageData) {
      if (!this.canvasNode || !this.canvasCtx) {
        console.warn('Canvas 未初始化，跳过图片处理')
        return
      }
      
      const targetWidth = this.config.image.width
      const targetHeight = this.config.image.height
      
      console.log('开始处理图片，目标尺寸:', targetWidth, 'x', targetHeight)
      
      // 使用主 canvas 的 createImage 方法
      const img = this.canvasNode.createImage()
      
      img.onload = () => {
        console.log('图片加载成功，原始尺寸:', img.width, 'x', img.height)
        try {
          const ctx = this.canvasCtx
          
          // 保存当前状态
          ctx.save()
          
          // 清空一个临时区域用于绘制图片
          ctx.clearRect(0, 0, targetWidth, targetHeight)
          
          // 绘制缩放后的图片
          ctx.drawImage(img, 0, 0, targetWidth, targetHeight)
          
          // 获取像素数据
          const imgData = ctx.getImageData(0, 0, targetWidth, targetHeight)
          const pixels = imgData.data
          
          // 恢复状态
          ctx.restore()
          
          // 转换为像素 Map
          const pixelMap = new Map()
          for (let y = 0; y < targetHeight; y++) {
            for (let x = 0; x < targetWidth; x++) {
              const idx = (y * targetWidth + x) * 4
              const r = pixels[idx]
              const g = pixels[idx + 1]
              const b = pixels[idx + 2]
              const a = pixels[idx + 3]
              
              // 跳过透明像素
              if (a > 128) {
                const finalX = this.config.image.x + x
                const finalY = this.config.image.y + y
                
                // 只保存在有效范围内的像素
                if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                  const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                  pixelMap.set(`${finalX},${finalY}`, hex)
                }
              }
            }
          }
          
          this.imagePixels = pixelMap
          console.log('图片处理完成，像素数量:', pixelMap.size)
          
          // 重新绘制完整的 canvas
          this.drawCanvas()
        } catch (err) {
          console.error('获取图片数据失败:', err)
          this.toast.showError('图片处理失败: ' + err.message)
        }
      }
      
      img.onerror = (err) => {
        console.error('图片加载失败:', err)
        this.toast.showError('图片加载失败')
      }
      
      img.src = imageData
    },
    
    drawImageOnCanvas() {
      if (!this.config.image.data) return
      
      const targetWidth = this.config.image.width
      const targetHeight = this.config.image.height
      
      console.log('准备处理图片，目标尺寸:', targetWidth, 'x', targetHeight)
      
      // 重新查询 canvas 节点，确保是最新的
      const query = uni.createSelectorQuery().in(this)
      query.select('#clockCanvas')
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0] || !res[0].node) {
            console.error('Canvas 节点查询失败')
            return
          }
          
          const canvas = res[0].node
          const ctx = canvas.getContext('2d')
          
          // 创建图片对象
          const img = canvas.createImage()
          
          img.onload = () => {
            console.log('图片加载成功')
            try {
              // 保存当前状态
              ctx.save()
              
              // 清空一个临时区域用于绘制图片
              ctx.clearRect(0, 0, targetWidth, targetHeight)
              
              // 绘制缩放后的图片
              ctx.drawImage(img, 0, 0, targetWidth, targetHeight)
              
              // 获取像素数据
              const imgData = ctx.getImageData(0, 0, targetWidth, targetHeight)
              const pixels = imgData.data
              
              // 恢复状态
              ctx.restore()
              
              // 转换为像素 Map 并保存
              const pixelMap = new Map()
              for (let y = 0; y < targetHeight; y++) {
                for (let x = 0; x < targetWidth; x++) {
                  const idx = (y * targetWidth + x) * 4
                  const r = pixels[idx]
                  const g = pixels[idx + 1]
                  const b = pixels[idx + 2]
                  const a = pixels[idx + 3]
                  
                  // 跳过透明像素
                  if (a > 128) {
                    const finalX = this.config.image.x + x
                    const finalY = this.config.image.y + y
                    
                    // 只保存在有效范围内的像素
                    if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                      const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                      pixelMap.set(`${finalX},${finalY}`, hex)
                    }
                  }
                }
              }
              
              this.imagePixels = pixelMap
              console.log('图片处理完成，像素数量:', pixelMap.size)
              
              // 重新绘制完整的 canvas
              this.drawCanvas()
            } catch (err) {
              console.error('绘制图片失败:', err)
              this.toast.showError('图片处理失败')
            }
          }
          
          img.onerror = (err) => {
            console.error('图片加载失败:', err)
            this.toast.showError('图片加载失败')
          }
          
          img.src = this.config.image.data
        })
    },
    
    removeImage() {
      this.config.image.data = null
      this.config.image.show = false
      this.imagePixels = null
      this.toast.showInfo('图片已删除')
      this.drawCanvas()
    },

    saveConfig() {
      // 创建配置副本，不保存图片的 base64 数据（太大）
      const configToSave = {
        time: this.config.time,
        date: this.config.date,
        week: this.config.week,
        image: {
          show: this.config.image.show,
          x: this.config.image.x,
          y: this.config.image.y,
          width: this.config.image.width,
          height: this.config.image.height,
          data: null // 不保存图片数据
        }
      }
      
      const saveData = {
        config: configToSave,
        pixels: Array.from(this.canvasPixels.entries()),
        imagePixels: this.imagePixels ? Array.from(this.imagePixels.entries()) : null // 保存图片像素数据
      }
      
      console.log('保存配置:', {
        手绘像素数量: this.canvasPixels.size,
        图片像素数量: this.imagePixels ? this.imagePixels.size : 0,
        配置: configToSave
      })
      
      uni.setStorageSync('clock_config', JSON.stringify(saveData))
      this.toast.showSuccess('配置已保存')
    },

    loadConfig() {
      const saved = uni.getStorageSync('clock_config')
      if (saved) {
        try {
          const savedData = JSON.parse(saved)
          
          // 加载配置
          if (savedData.config) {
            this.config = {
              time: { ...this.config.time, ...savedData.config.time },
              date: { ...this.config.date, ...savedData.config.date },
              week: { ...this.config.week, ...savedData.config.week },
              image: { ...this.config.image, ...savedData.config.image }
            }
          }
          
          // 加载手绘像素
          if (savedData.pixels && Array.isArray(savedData.pixels)) {
            this.canvasPixels = new Map(savedData.pixels)
          }
          
          // 加载图片像素数据
          if (savedData.imagePixels && Array.isArray(savedData.imagePixels)) {
            this.imagePixels = new Map(savedData.imagePixels)
          }
          
          console.log('配置加载成功，手绘像素数量:', this.canvasPixels.size, '图片像素数量:', this.imagePixels ? this.imagePixels.size : 0)
          
          // 如果 canvas 已经初始化，重新绘制
          if (this.canvasCtx) {
            this.drawCanvas()
          }
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
        
        // 1. 先切换到闹钟模式
        await ws.setMode('clock')
        
        // 2. 发送闹钟配置
        await ws.send({ 
          cmd: 'set_clock_config',
          config: {
            time: {
              show: this.config.time.show,
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
            },
            image: {
              show: this.config.image.show,
              x: this.config.image.x,
              y: this.config.image.y,
              width: this.config.image.width,
              height: this.config.image.height
            }
          }
        })
        
        console.log('闹钟配置发送成功')
        
        // 3. 如果有像素数据（手绘 + 图片），使用二进制方式发送
        const allPixels = new Map()
        
        // 合并手绘像素和图片像素
        if (this.canvasPixels) {
          this.canvasPixels.forEach((color, key) => {
            allPixels.set(key, color)
          })
        }
        
        if (this.imagePixels) {
          this.imagePixels.forEach((color, key) => {
            allPixels.set(key, color)
          })
        }
        
        if (allPixels.size > 0) {
          console.log('准备发送像素数据，总数:', allPixels.size)
          
          // 转换为像素数组
          const pixelArray = []
          allPixels.forEach((color, key) => {
            const [x, y] = key.split(',').map(Number)
            const rgb = this.hexToRgb(color)
            pixelArray.push({
              x: x,
              y: y,
              r: rgb.r,
              g: rgb.g,
              b: rgb.b
            })
          })
          
          // 使用二进制方式发送
          await this.sendImagePixelsBinary(pixelArray)
          
          this.toast.showSuccess(`已发送到设备！包含 ${pixelArray.length} 个像素点`)
        } else {
          this.toast.showSuccess('已发送到设备')
        }
      } catch (err) {
        console.error('发送失败:', err)
        this.toast.showError('发送失败: ' + err.message)
      }
    },
    
    async sendImagePixelsBinary(pixels) {
      // 使用二进制方式发送像素数据
      // 格式：每个像素5字节 (x, y, r, g, b)
      const ws = this.deviceStore.getWebSocket()
      
      console.log(`开始发送像素数据，共 ${pixels.length} 个像素`)
      
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
        if (i + batchSize < pixels.length) {
          await new Promise(resolve => setTimeout(resolve, 200))
        }
      }
      
      console.log('所有像素数据发送完成')
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

.preview-hint {
  padding: 20rpx 32rpx;
  background-color: var(--bg-tertiary);
  border-bottom: 2rpx solid var(--border-primary);
}

.preview-hint-text {
  font-size: 22rpx;
  color: var(--text-secondary);
  line-height: 1.4;
}

.canvas-section {
  background-color: var(--bg-elevated);
  border-bottom: 2rpx solid var(--border-primary);
  padding: 24rpx;
}

.canvas-wrapper {
  width: 100%;
  display: flex;
  justify-content: center;
}

.clock-canvas {
  width: 320px;
  height: 320px;
  background-color: #000;
  border: 2rpx solid var(--accent-primary);
  border-radius: 12rpx;
  box-shadow: 0 0 20rpx rgba(0, 243, 255, 0.3);
}

.tool-selection {
  display: flex;
  gap: 16rpx;
  margin-bottom: 24rpx;
}

.tool-option {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 32rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.tool-option:active {
  transform: scale(0.98);
}

.tool-option.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
  box-shadow: 0 0 16rpx rgba(0, 243, 255, 0.3);
}

.tool-name {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.tool-option.active .tool-name {
  color: var(--accent-primary);
  font-weight: 500;
}

.action-buttons {
  display: flex;
  gap: 16rpx;
  margin-top: 24rpx;
}

.action-button {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12rpx;
  padding: 24rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  font-size: 22rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.action-button:active {
  transform: scale(0.98);
  border-color: var(--accent-primary);
}

.action-button.danger {
  border-color: rgba(239, 68, 68, 0.3);
  color: var(--error-color);
}

.action-button.danger:active {
  background-color: rgba(239, 68, 68, 0.1);
}

.current-color-section {
  margin-bottom: 24rpx;
}

.current-color-display {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 12rpx;
  margin-top: 12rpx;
}

.current-color-box {
  width: 80rpx;
  height: 80rpx;
  border-radius: 12rpx;
  border: 2rpx solid var(--border-primary);
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.2);
}

.current-color-hex {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
}

.palette-toggle {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20rpx 24rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  margin: 24rpx 0;
  transition: var(--transition-base);
}

.palette-toggle:active {
  background-color: var(--bg-tertiary);
  border-color: var(--accent-primary);
}

.palette-toggle-text {
  font-size: 24rpx;
  color: var(--text-primary);
  font-weight: 500;
}

.palette-toggle-icon {
  font-size: 24rpx;
  color: var(--accent-primary);
}

.full-color-palette {
  display: grid;
  grid-template-columns: repeat(12, 1fr);
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-secondary);
  border-radius: 12rpx;
  margin-bottom: 24rpx;
  max-height: 600rpx;
  overflow-y: auto;
}

.palette-color-item {
  aspect-ratio: 1;
  border-radius: 8rpx;
  border: 2rpx solid transparent;
  transition: var(--transition-base);
}

.palette-color-item:active {
  transform: scale(0.9);
}

.palette-color-item.active {
  border-color: var(--accent-primary);
  border-width: 3rpx;
  box-shadow: 0 0 12rpx var(--accent-primary);
  transform: scale(1.1);
  z-index: 1;
}

.align-buttons {
  display: flex;
  gap: 12rpx;
  margin-top: 12rpx;
}

.align-btn {
  flex: 1;
  padding: 20rpx 16rpx;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  text-align: center;
  transition: var(--transition-base);
}

.align-btn:active {
  transform: scale(0.98);
}

.align-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.align-text {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.align-btn.active .align-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.setting-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12rpx;
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

.warning-color {
  color: #ff6464 !important;
}

.warning-text {
  font-size: 20rpx;
  color: #ff6464;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
  gap: 4rpx;
}


.content {
  flex: 1;
  width: 100%;
  min-height: 0;
  box-sizing: border-box;
}

.content-wrapper {
  padding: 24rpx;
  padding-bottom: 48rpx;
}

/* 底部 Tab 栏：与拼豆辅助页底部控制区风格一致 */
.bottom-tabs {
  display: flex;
  flex-shrink: 0;
  height: 100rpx;
  padding: 12rpx 16rpx;
  padding-bottom: calc(12rpx + env(safe-area-inset-bottom));
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  gap: 8rpx;
}

.bottom-tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 4rpx;
  border-radius: 16rpx;
  transition: var(--transition-base);
}

.bottom-tab-item:active {
  background-color: var(--bg-tertiary);
}

.bottom-tab-item.active {
  background-color: var(--bg-tertiary);
  box-shadow: var(--shadow-sm);
}

.bottom-tab-text {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.bottom-tab-item.active .bottom-tab-text {
  color: var(--accent-primary);
  font-weight: 500;
}

.settings-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-primary);
  border-radius: 16rpx;
  padding: 20rpx;
  margin-bottom: 16rpx;
  box-shadow: var(--shadow-md);
}

.card-title-section {
  display: flex;
  align-items: center;
  gap: 8rpx;
  margin-bottom: 16rpx;
}

.card-title {
  font-size: 22rpx;
  font-weight: 500;
  color: var(--text-primary);
  flex: 1;
}

.setting-group {
  display: flex;
  flex-direction: column;
  gap: 12rpx;
}

.setting-item {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
}

.setting-item-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16rpx;
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
  font-size: 18rpx;
  color: #ff6464;
  margin-top: 4rpx;
}

.setting-label {
  font-size: 20rpx;
  color: var(--text-secondary);
}

.setting-control {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.setting-control-buttons {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.control-btn {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-secondary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: var(--transition-base);
}

.control-btn:active {
  transform: scale(0.9);
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
}

.control-icon {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.setting-value-large {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-primary);
  min-width: 64rpx;
  text-align: center;
}

.setting-value {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--accent-primary);
  min-width: 40rpx;
  text-align: right;
}

.setting-slider {
  flex: 1;
}

.color-picker {
  display: flex;
  gap: 8rpx;
  flex-wrap: wrap;
}

.color-item {
  width: 52rpx;
  height: 52rpx;
  border-radius: 12rpx;
  border: 3rpx solid transparent;
  transition: var(--transition-base);
  box-shadow: 0 2rpx 8rpx rgba(0, 0, 0, 0.2);
}

.color-item:active {
  transform: scale(0.95);
}

.color-item.active {
  border-color: var(--accent-primary);
  box-shadow: 0 0 16rpx var(--accent-primary);
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
  align-items: center;
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

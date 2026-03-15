<template>
  <view class="clock-editor-page">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 头部 -->
    <view class="header">
      <view class="header-left">
        <view class="back-btn" @click="goBack">
          <Icon name="direction-left" :size="40" color="var(--color-text-primary)" />
        </view>
        <view class="header-info">
          <text class="header-title">闹钟编辑器</text>
          <text class="header-subtitle">自定义时钟样式</text>
        </view>
        <view class="header-actions-inline">
          <view class="action-btn-sm" @click="saveConfig">
            <Icon name="save" :size="36" color="var(--color-text-primary)" />
          </view>
          <view class="action-btn-sm primary" :class="{ 'disabled': isSending }" @click="sendToDevice">
            <Icon name="link" :size="36" color="#fff" />
          </view>
        </view>
      </view>
    </view>

    <!-- Canvas 预览区域 -->
    <view class="canvas-section" v-show="!canvasHidden">
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
                  @click="setAlignment('time', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.time.align === 'center' }"
                  @click="setAlignment('time', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.time.align === 'right' }"
                  @click="setAlignment('time', 'right')"
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
                  @click="setAlignment('date', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.date.align === 'center' }"
                  @click="setAlignment('date', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.date.align === 'right' }"
                  @click="setAlignment('date', 'right')"
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
                  @click="setAlignment('week', 'left')"
                >
                  <text class="align-text">左对齐</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.week.align === 'center' }"
                  @click="setAlignment('week', 'center')"
                >
                  <text class="align-text">居中</text>
                </view>
                <view 
                  class="align-btn"
                  :class="{ 'active': config.week.align === 'right' }"
                  @click="setAlignment('week', 'right')"
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
              <text class="upload-text">{{ clockMode === 'animation' ? '点击上传图片/GIF' : '点击上传背景图片' }}</text>
              <text class="upload-hint">{{ clockMode === 'animation' ? '支持静态图片和 GIF 动画，建议 64x64 以内' : '支持静态图片，建议 64x64 以内' }}</text>
            </view>
            <view v-else class="image-preview">
              <image :src="config.image.data" class="preview-image" mode="aspectFit" />
              <view v-if="gifAnimationData" class="gif-badge">
                <text class="gif-badge-text">GIF {{gifAnimationData.frameCount}}帧</text>
              </view>
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
                <text class="setting-value-large" :class="{ 'info-color': config.image.width > 64 }">{{ config.image.width }}</text>
                <view class="control-btn" @click="adjustImageValue('width', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.width > 64" class="info-text">💡 宽度超过64，超出部分不会显示</text>

            <view class="setting-item-row">
              <text class="setting-label">高度</text>
              <view class="setting-control-buttons">
                <view class="control-btn" @click="adjustImageValue('height', -1)">
                  <text class="control-icon">-</text>
                </view>
                <text class="setting-value-large" :class="{ 'info-color': config.image.height > 64 }">{{ config.image.height }}</text>
                <view class="control-btn" @click="adjustImageValue('height', 1)">
                  <text class="control-icon">+</text>
                </view>
              </view>
            </view>
            <text v-if="config.image.height > 64" class="info-text">💡 高度超过64，超出部分不会显示</text>
            
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
    
    <!-- 传输遮罩弹窗 -->
    <view v-if="isSending" class="sending-overlay" @touchmove.stop.prevent>
      <view class="sending-modal">
        <view class="sending-spinner"></view>
        <text class="sending-title">正在传输数据...</text>
        <text class="sending-tip">请勿切换网络或关闭程序</text>
      </view>
    </view>

    <Toast ref="toastRef" @show="canvasHidden = true" @hide="onToastHide" />
  </view>
</template>

<script>
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
  getTextWidth,
  getCurrentTimeText,
  getCurrentDateText,
  getCurrentWeekText
} from '../../utils/clockCanvas.js'
import { COLOR_PALETTE_221, COMMON_COLORS } from '../../utils/colorPalette.js'
import { GIFParser } from '../../utils/gifParser.js'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast
  },
  data() {
    return {
      deviceStore: null,
      toast: null,
      isReady: false,
      clockMode: 'clock', // clock=静态时钟, animation=动态时钟
      gifAnimationData: null, // GIF 动画数据 { frameCount, frames }
      gifRenderedFrameMaps: null, // GIF 所有帧的像素 Map 数组
      gifFrameIndex: 0, // 当前播放帧索引
      gifTimer: null, // GIF 动画定时器
      isSending: false, // 传输锁，防止传输过程中操作
      _gifParser: null, // GIF 解析器实例，改宽高时重新生成数据
      
      contentHeight: 'calc(100vh - 112rpx - 120rpx - 80rpx)',
      
      // Canvas 相关
      canvasHidden: false, // 弹窗显示时隐藏 canvas（微信小程序原生组件层级问题）
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
          width: 64,
          height: 64,
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

  watch: {
    canvasHidden(newVal) {
      // canvas 从隐藏恢复显示时重绘
      if (!newVal && this.canvasCtx) {
        this.$nextTick(() => {
          this.drawCanvas()
        })
      }
    }
  },

  computed: {
    accentColor() {
      return '#4F7FFF'
    }
  },

  onUnload() {
    this.stopGifAnimation()
  },

  onLoad(options) {
    // 接收模式参数：clock=静态时钟, animation=动态时钟
    this.clockMode = options.mode || 'clock'

    // 先加载配置，确保 config 有值
    this.loadConfig()
    
    this.deviceStore = useDeviceStore()
    this.toast = useToast()
    
    const systemInfo = uni.getSystemInfoSync()
    const statusBarHeight = systemInfo.statusBarHeight || 0
    const headerHeight = 56
    this.contentHeight = `${systemInfo.windowHeight - statusBarHeight - headerHeight - 300}px`
    
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
    // 页面显示时的处理
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
    
    getClockExcludeRegions() {
      const regions = []
      const c = this.config
      // 每个字符(含冒号) 5x7 像素，间隔 1px，fontSize 倍数缩放
      // "HH:MM" = 5字符, 宽 = 5*6-1 = 29 * fontSize, 高 = 7 * fontSize
      const fs = c.time.fontSize || 1
      regions.push({ x: c.time.x, y: c.time.y, w: (5 * 6 - 1) * fs, h: 7 * fs })
      // 日期 "MM-DD" = 5字符
      if (c.date.show) {
        const dfs = c.date.fontSize || 1
        regions.push({ x: c.date.x, y: c.date.y, w: (5 * 6 - 1) * dfs, h: 7 * dfs })
      }
      // 星期 "Wed" = 3字符
      if (c.week.show) {
        const wfs = c.week.fontSize || 1
        regions.push({ x: c.week.x, y: c.week.y, w: (3 * 6 - 1) * wfs, h: 7 * wfs })
      }
      return regions
    },

    getAllPixels() {
      const allPixels = new Map()
      
      // 1. 背景图片像素（使用相对坐标，动态应用偏移）
      if (this.imagePixels && this.imagePixels.size > 0) {
        const offsetX = this.config.image.x || 0
        const offsetY = this.config.image.y || 0
        this.imagePixels.forEach((color, key) => {
          const [rx, ry] = key.split(',').map(Number)
          const finalX = rx + offsetX
          const finalY = ry + offsetY
          if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
            allPixels.set(`${finalX},${finalY}`, color)
          }
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

    onToastHide() {
      // 传输中不恢复 canvas，避免遮挡传输弹窗
      if (!this.isSending) {
        this.canvasHidden = false
      }
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
        width: { min: 1, max: 256 },
        height: { min: 1, max: 256 },
        x: { min: -128, max: 128 },
        y: { min: -128, max: 128 }
      }

      const { min, max } = limits[key]
      const currentValue = this.config.image[key]
      const newValue = Math.max(min, Math.min(max, currentValue + delta))

      if (newValue !== currentValue) {
        this.config.image[key] = newValue

        // 如果改变了尺寸，需要重新转换图片
        if ((key === 'width' || key === 'height') && this.config.image.data) {
          if (this.gifAnimationData && this._gifParser) {
            // GIF：重新生成 ESP32 数据和预览帧
            const targetW = this.config.image.width || 64
            const targetH = this.config.image.height || 64
            this.gifAnimationData = this._gifParser.generateESP32Data(targetW, targetH, 20, this.getClockExcludeRegions())
            const renderedFrames = this._gifParser.renderFrames(targetW, targetH)
            this.gifRenderedFrameMaps = renderedFrames.map(frame => {
              const pixelMap = new Map()
              for (let y = 0; y < targetH; y++) {
                for (let x = 0; x < targetW; x++) {
                  const idx = (y * targetW + x) * 4
                  const r = frame.rgba[idx], g = frame.rgba[idx+1], b = frame.rgba[idx+2]
                  if (r > 0 || g > 0 || b > 0) {
                    const hex = '#' + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1)
                    pixelMap.set(`${x},${y}`, hex)
                  }
                }
              }
              return { pixels: pixelMap, delay: frame.delay || 100 }
            })
            console.log(`GIF 尺寸更新: ${targetW}x${targetH}, ${this.gifAnimationData.frameCount} 帧`)
          } else {
            this.convertImageToPixels(this.config.image.data)
          }
        } else {
          // 位置改变时直接重绘（像素存储相对坐标，偏移在绘制时动态应用）
          this.drawCanvas()
        }

        // 显示提示（不再是警告）
        if ((key === 'width' || key === 'height') && newValue > 64) {
          this.toast.showInfo(`${key === 'width' ? '宽度' : '高度'}为 ${newValue}，超出部分不会显示`)
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

    setAlignment(section, align) {
      this.config[section].align = align
      // 自动校正 x 坐标到对齐参考点
      if (align === 'left') {
        this.config[section].x = 0
      } else if (align === 'center') {
        this.config[section].x = 32
      } else if (align === 'right') {
        this.config[section].x = 63
      }
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
      // 使用 chooseMessageFile 支持选择 GIF 文件
      uni.chooseMessageFile({
        count: 1,
        type: 'image',
        success: (res) => {
          const file = res.tempFiles[0]
          const tempFilePath = file.path || file.tempFilePath
          const fileName = (file.name || tempFilePath || '').toLowerCase()
          const isGif = fileName.endsWith('.gif')

          console.log('选择文件:', fileName, '是否GIF:', isGif, '模式:', this.clockMode)

          if (isGif && this.clockMode === 'animation') {
            // 动态模式下才解析 GIF 动画
            this._handleGifFile(tempFilePath)
          } else {
            // 静态模式或非 GIF：当静态图片处理
            if (isGif && this.clockMode === 'clock') {
              this.toast.showInfo('静态时钟模式下 GIF 将作为静态图片使用')
            }
            this._handleStaticImage(tempFilePath)
          }
        },
        fail: (err) => {
          console.error('选择文件失败:', err)
          // 降级到 chooseImage
          this._fallbackChooseImage()
        }
      })
    },

    _fallbackChooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ['compressed'],
        sourceType: ['album', 'camera'],
        success: (res) => {
          this._handleStaticImage(res.tempFilePaths[0])
        },
        fail: (err) => {
          console.error('选择图片失败:', err)
        }
      })
    },

    _handleStaticImage(tempFilePath) {
      this.stopGifAnimation()
      this.gifAnimationData = null // 清除 GIF 数据
      this.gifRenderedFrameMaps = null
      this.gifFrameIndex = 0
      uni.getFileSystemManager().readFile({
        filePath: tempFilePath,
        encoding: 'base64',
        success: (fileRes) => {
          const imageData = 'data:image/png;base64,' + fileRes.data
          this.config.image.data = imageData
          this.config.image.show = true
          this.convertImageToPixels(imageData)
          this.toast.showSuccess('图片已上传')
        },
        fail: (err) => {
          console.error('读取图片失败:', err)
          this.toast.showError('图片读取失败')
        }
      })
    },

    _handleGifFile(tempFilePath) {
      uni.getFileSystemManager().readFile({
        filePath: tempFilePath,
        success: (fileRes) => {
          try {
            const parser = new GIFParser()
            const gifInfo = parser.parse(fileRes.data)
            console.log('GIF 解析成功:', gifInfo)
            this._gifParser = parser  // 保存 parser 实例，改宽高时重新生成数据

            // 生成 ESP32 动画数据
            const targetW = this.config.image.width || 64
            const targetH = this.config.image.height || 64
            const esp32Data = parser.generateESP32Data(targetW, targetH, 20, this.getClockExcludeRegions())
            this.gifAnimationData = esp32Data
            console.log(`ESP32 动画数据: ${esp32Data.frameCount} 帧`)

            // 渲染所有帧为像素 Map，用于画布动画预览
            const renderedFrames = parser.renderFrames(targetW, targetH)
            const frameMaps = renderedFrames.map(frame => {
              const pixelMap = new Map()
              for (let y = 0; y < targetH; y++) {
                for (let x = 0; x < targetW; x++) {
                  const idx = (y * targetW + x) * 4
                  const r = frame.rgba[idx]
                  const g = frame.rgba[idx + 1]
                  const b = frame.rgba[idx + 2]
                  const a = frame.rgba[idx + 3]
                  if (a > 10 && (r > 0 || g > 0 || b > 0)) {
                    const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                    pixelMap.set(`${x},${y}`, hex)
                  }
                }
              }
              return { pixels: pixelMap, delay: frame.delay || 100 }
            })
            this.gifRenderedFrameMaps = frameMaps
            console.log('GIF 帧像素 Map 已生成:', frameMaps.length, '帧')

            // 设置第一帧并启动动画
            if (frameMaps.length > 0) {
              this.imagePixels = frameMaps[0].pixels
              this.startGifAnimation()
            }

            // 读取 base64 用于缩略图预览
            uni.getFileSystemManager().readFile({
              filePath: tempFilePath,
              encoding: 'base64',
              success: (b64Res) => {
                this.config.image.data = 'data:image/gif;base64,' + b64Res.data
                this.config.image.show = true
                this.drawCanvas()
                this.toast.showSuccess(`GIF 已解析！${esp32Data.frameCount} 帧`)
              }
            })
          } catch (err) {
            console.error('GIF 解析失败:', err)
            this.toast.showError('GIF 解析失败: ' + err.message)
            // 降级为静态图片
            this._handleStaticImage(tempFilePath)
          }
        },
        fail: (err) => {
          console.error('读取 GIF 失败:', err)
          this.toast.showError('GIF 文件读取失败')
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
              
              // 转换为像素 Map（存储相对坐标，偏移在绘制时动态应用）
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
                    const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                    pixelMap.set(`${x},${y}`, hex)
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
          
          // 转换为像素 Map（存储相对坐标）
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
                const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                pixelMap.set(`${x},${y}`, hex)
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
              
              // 转换为像素 Map（存储相对坐标）
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
                    const hex = `#${[r, g, b].map(v => v.toString(16).padStart(2, '0')).join('')}`
                    pixelMap.set(`${x},${y}`, hex)
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
    
    // ========== GIF 动画播放 ==========
    startGifAnimation() {
      this.stopGifAnimation()
      if (!this.gifRenderedFrameMaps || this.gifRenderedFrameMaps.length <= 1) return

      this.gifFrameIndex = 0
      const playNextFrame = () => {
        if (!this.gifRenderedFrameMaps) return

        this.gifFrameIndex = (this.gifFrameIndex + 1) % this.gifRenderedFrameMaps.length
        const frameData = this.gifRenderedFrameMaps[this.gifFrameIndex]
        this.imagePixels = frameData.pixels
        this.drawCanvas()

        // 用当前帧的 delay 设置下一帧定时
        this.gifTimer = setTimeout(playNextFrame, frameData.delay || 100)
      }

      const firstDelay = this.gifRenderedFrameMaps[0].delay || 100
      this.gifTimer = setTimeout(playNextFrame, firstDelay)
    },

    stopGifAnimation() {
      if (this.gifTimer) {
        clearTimeout(this.gifTimer)
        this.gifTimer = null
      }
    },

    removeImage() {
      this.stopGifAnimation()
      this.config.image.data = null
      this.config.image.show = false
      this.imagePixels = null
      this.gifAnimationData = null
      this.gifRenderedFrameMaps = null
      this.gifFrameIndex = 0
      this.toast.showInfo('图片已删除')
      this.drawCanvas()
    },

    saveConfig() {
      // 创建配置副本，不保存图片的 base64 数据（太大）
      const configToSave = {
        time: {
          show: this.config.time.show,
          fontSize: this.config.time.fontSize,
          x: this.config.time.x,
          y: this.config.time.y,
          color: this.config.time.color,
          align: this.config.time.align
        },
        date: {
          show: this.config.date.show,
          fontSize: this.config.date.fontSize,
          x: this.config.date.x,
          y: this.config.date.y,
          color: this.config.date.color,
          align: this.config.date.align
        },
        week: {
          show: this.config.week.show,
          x: this.config.week.x,
          y: this.config.week.y,
          color: this.config.week.color,
          align: this.config.week.align
        },
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
      if (this.isSending) {
        this.toast.showInfo('正在传输中，请等待完成')
        return
      }

      this.isSending = true
      this.canvasHidden = true  // 隐藏 canvas，避免原生组件层级遮挡弹窗
      try {
        const ws = this.deviceStore.getWebSocket()

        // 统一的发送并等待回复方法，确保 ESP32 处理完一条再发下一条
        const sendAndWait = (data, timeoutMs = 10000) => {
          return new Promise((resolve, reject) => {
            let resolved = false
            const handler = (msg) => {
              if (resolved) return
              resolved = true
              ws.offMessage(handler)
              clearTimeout(timer)
              // 检查 ESP32 返回的错误响应
              try {
                const resp = typeof msg === 'string' ? JSON.parse(msg) : (msg.data ? JSON.parse(msg.data) : msg)
                if (resp.error) {
                  reject(new Error(`ESP32 错误: ${resp.error}${resp.details ? ' - ' + resp.details : ''}`))
                  return
                }
                resolve(resp)
              } catch (e) {
                // 非 JSON 响应，直接 resolve
                resolve(msg)
              }
            }
            ws.onMessage(handler)
            const timer = setTimeout(() => {
              if (!resolved) {
                resolved = true
                ws.offMessage(handler)
                reject(new Error('ESP32 响应超时'))
              }
            }, timeoutMs)
            ws.send(data).catch(err => {
              if (!resolved) {
                resolved = true
                ws.offMessage(handler)
                clearTimeout(timer)
                reject(err)
              }
            })
          })
        }

        // 1. 先切换到闹钟模式（等待回复）
        await sendAndWait({ cmd: 'set_mode', mode: 'clock' })
        console.log('模式切换成功')

        // 2. 根据对齐方式校正 x 坐标（与 drawTextToPixels 保持一致）
        const timeAlign = this.config.time.align || 'left'
        let timeX = this.config.time.x
        if (timeAlign === 'center') {
          timeX = timeX - Math.floor(getTextWidth(getCurrentTimeText(), this.config.time.fontSize) / 2)
        } else if (timeAlign === 'right') {
          timeX = timeX - getTextWidth(getCurrentTimeText(), this.config.time.fontSize)
        }

        const dateAlign = this.config.date.align || 'left'
        let dateX = this.config.date.x
        if (dateAlign === 'center') {
          dateX = dateX - Math.floor(getTextWidth(getCurrentDateText(), this.config.date.fontSize) / 2)
        } else if (dateAlign === 'right') {
          dateX = dateX - getTextWidth(getCurrentDateText(), this.config.date.fontSize)
        }

        const weekAlign = this.config.week.align || 'left'
        let weekX = this.config.week.x
        if (weekAlign === 'center') {
          weekX = weekX - Math.floor(getTextWidth(getCurrentWeekText(), 1) / 2)
        } else if (weekAlign === 'right') {
          weekX = weekX - getTextWidth(getCurrentWeekText(), 1)
        }

        // 发送闹钟配置（x 坐标已根据对齐方式校正，等待回复）
        await sendAndWait({
          cmd: 'set_clock_config',
          config: {
            time: {
              show: this.config.time.show,
              fontSize: this.config.time.fontSize,
              x: timeX,
              y: this.config.time.y,
              color: this.hexToRgb(this.config.time.color)
            },
            date: {
              show: this.config.date.show,
              fontSize: this.config.date.fontSize,
              x: dateX,
              y: this.config.date.y,
              color: this.hexToRgb(this.config.date.color)
            },
            week: {
              show: this.config.week.show,
              x: weekX,
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

        // 3. 检查是否有 GIF 动画数据
        if (this.gifAnimationData && this.gifAnimationData.frameCount > 0) {
          console.log(`发送 GIF 动画: ${this.gifAnimationData.frameCount} 帧`)

          // 不要提前切换到动画模式——animation_end 会自动切换并播放
          // 如果提前切换，ESP32 发现没有已加载的动画会直接清屏导致黑屏

          // 逐帧发送——每条命令发送后等待 ESP32 回复确认再发下一条
          const { frameCount, frames } = this.gifAnimationData

          let resp = await sendAndWait({ cmd: 'animation_begin', frameCount })
          console.log('animation_begin 回复:', JSON.stringify(resp))

          const CHUNK_SIZE = 200  // 二进制每批200像素 = 1000字节，很轻松
          const ws = this.deviceStore.getWebSocket()
          for (let i = 0; i < frames.length; i++) {
            const frame = frames[i]  // [type, delay, count, pixels]
            const type = frame[0]
            const delay = frame[1]
            const pixels = frame[3]
            const totalPixels = pixels.length

            // 所有帧都走 frame_begin + 二进制 chunk
            resp = await sendAndWait({ cmd: 'frame_begin', index: i, type, delay, totalPixels })
            console.log(`帧 ${i + 1}/${frameCount} 开始二进制传输 (${totalPixels} 像素):`, resp.status)
            if (resp.status === 'error') {
              this.toast.showError(`帧 ${i} 初始化失败: ${resp.message}`)
              return
            }

            // 二进制分片发送像素数据
            for (let offset = 0; offset < totalPixels; offset += CHUNK_SIZE) {
              const chunk = pixels.slice(offset, offset + CHUNK_SIZE)
              const binaryData = new Uint8Array(chunk.length * 5)
              for (let j = 0; j < chunk.length; j++) {
                const p = chunk[j]  // [x, y, r, g, b]
                binaryData[j * 5] = p[0]
                binaryData[j * 5 + 1] = p[1]
                binaryData[j * 5 + 2] = p[2]
                binaryData[j * 5 + 3] = p[3]
                binaryData[j * 5 + 4] = p[4]
              }

              // 发送二进制并等待ESP32文本回复确认
              resp = await new Promise((resolve, reject) => {
                let resolved = false
                const timeout = setTimeout(() => {
                  if (!resolved) {
                    resolved = true
                    ws.offMessage(handler)
                    reject(new Error('binary chunk timeout'))
                  }
                }, 5000)
                const handler = (msg) => {
                  if (resolved) return
                  resolved = true
                  clearTimeout(timeout)
                  ws.offMessage(handler)
                  resolve(msg)  // ws.onMessage 已经 JSON.parse 过了
                }
                ws.onMessage(handler)
                ws.socket.send({
                  data: binaryData.buffer,
                  fail: (err) => {
                    if (!resolved) {
                      resolved = true
                      ws.offMessage(handler)
                      clearTimeout(timeout)
                      reject(err)
                    }
                  }
                })
              })
              console.log(`  chunk ${Math.floor(offset / CHUNK_SIZE) + 1}: ${chunk.length} 像素, ${resp.status}`)
              if (resp.status === 'error') {
                this.toast.showError(`帧 ${i} chunk 发送失败: ${resp.message}`)
                return
              }
            }

            if (resp.status === 'error') {
              console.error(`帧 ${i} 发送失败:`, resp.message)
              this.toast.showError(`帧 ${i} 发送失败: ${resp.message}`)
              return
            }
          }

          resp = await sendAndWait({ cmd: 'animation_end' })
          console.log('animation_end 回复:', JSON.stringify(resp))
          this.toast.showSuccess(`GIF 动画已发送！${frameCount} 帧`)
        } else {
          // 4. 如果有像素数据（手绘 + 图片），使用二进制方式发送
          const allPixels = new Map()

          if (this.canvasPixels) {
            this.canvasPixels.forEach((color, key) => {
              allPixels.set(key, color)
            })
          }

          if (this.imagePixels) {
            const offsetX = this.config.image.x || 0
            const offsetY = this.config.image.y || 0
            this.imagePixels.forEach((color, key) => {
              const [rx, ry] = key.split(',').map(Number)
              const finalX = rx + offsetX
              const finalY = ry + offsetY
              if (finalX >= 0 && finalX < 64 && finalY >= 0 && finalY < 64) {
                allPixels.set(`${finalX},${finalY}`, color)
              }
            })
          }

          if (allPixels.size > 0) {
            console.log('准备发送像素数据，总数:', allPixels.size)

            const pixelArray = []
            allPixels.forEach((color, key) => {
              const [x, y] = key.split(',').map(Number)
              const rgb = this.hexToRgb(color)
              pixelArray.push({ x, y, r: rgb.r, g: rgb.g, b: rgb.b })
            })

            await this.sendImagePixelsBinary(pixelArray)
            this.toast.showSuccess(`已发送到设备并保存！包含 ${pixelArray.length} 个像素点`)
          } else {
            this.toast.showSuccess('已发送到设备并保存')
          }
        }

        // 传输全部成功后才保存到本地
        this.saveConfig()
      } catch (err) {
        console.error('发送失败:', err)
        this.toast.showError('发送失败: ' + err.message)
      } finally {
        this.isSending = false
        this.canvasHidden = false  // 恢复 canvas 显示
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
  gap: 16rpx;
  flex: 1;
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
  margin-right: 20rpx;
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

.header-actions-inline {
  display: flex;
  gap: 12rpx;
  margin-right: 16rpx;
}

.action-btn-sm {
  width: 56rpx;
  height: 56rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 14rpx;
  border: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
  transition: var(--transition-base);
}

.action-btn-sm:active {
  transform: scale(0.95);
}

.action-btn-sm.primary {
  background-color: var(--accent-primary);
  border-color: var(--accent-primary);
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

.info-color {
  color: #4F7FFF !important;
}

.info-text {
  font-size: 20rpx;
  color: #4F7FFF;
  margin-top: 8rpx;
  display: flex;
  align-items: center;
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
  position: relative;
}

.gif-badge {
  position: absolute;
  top: 12rpx;
  right: 12rpx;
  background: rgba(139, 92, 246, 0.9);
  border-radius: 8rpx;
  padding: 4rpx 12rpx;
}

.gif-badge-text {
  color: #fff;
  font-size: 22rpx;
  font-weight: 600;
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

/* 传输遮罩弹窗 */
.sending-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}

.sending-modal {
  background: var(--color-bg-secondary, #fff);
  border-radius: 24rpx;
  padding: 60rpx 50rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  min-width: 400rpx;
}

.sending-spinner {
  width: 60rpx;
  height: 60rpx;
  border: 6rpx solid rgba(99, 102, 241, 0.2);
  border-top-color: #6366f1;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.sending-title {
  font-size: 32rpx;
  font-weight: 600;
  color: var(--color-text-primary, #333);
}

.sending-tip {
  font-size: 24rpx;
  color: var(--color-text-secondary, #999);
}

.sending-text {
  font-size: 24rpx;
  color: #fff;
}

.action-btn.disabled {
  opacity: 0.5;
  pointer-events: none;
}
</style>

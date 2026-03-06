<template>
  <view class="create-page" :class="{ 'light-theme': themeStore && !themeStore.isDarkMode }">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 背景装饰 -->
    <view class="bg-decoration bg-decoration-1"></view>
    <view class="bg-decoration bg-decoration-2"></view>
    
    <!-- 顶部栏 -->
    <view class="header">
      <view class="back-btn" @click="handleBack">
        <text class="back-icon">‹</text>
      </view>
      <text class="header-title">新建画布</text>
    </view>
    
    <!-- 内容区域 -->
    <!-- 步骤 0: 名称 -->
    <scroll-view 
      v-if="step === 0"
      scroll-y 
      class="content-area"
    >
        <view 
          class="step-content step-name"
          :class="stepAnimationClass"
        >
          <view class="input-group">
            <text class="input-label">画布名称</text>
            <view 
              class="name-input-wrapper"
              :class="{ 'focused': isNameFocused }"
            >
              <input 
                v-model="name"
                type="text"
                class="name-input"
                placeholder="例如：2026新年快乐"
                cursor-color="#00f3ff"
                @focus="isNameFocused = true"
                @blur="isNameFocused = false"
                :focus="step === 0"
              />
            </view>
          </view>
          
          <view class="step-hint">
            <text class="hint-text">为您的创作赋予独特的身份。这将帮助您稍后在画布库中找到它。</text>
          </view>
        </view>
      </scroll-view>
      
      <!-- 步骤 1: 模式和尺寸 -->
      <view 
        v-if="step === 1" 
        class="step-1-wrapper"
      >
        <!-- 模式选择 - 固定在顶部 -->
        <view class="mode-selector-fixed">
          <view class="mode-selector">
            <view 
              class="mode-btn"
              :class="{ 'active': mode === 'blank' }"
              @click="mode = 'blank'"
            >
              <text class="mode-text">空白画布</text>
            </view>
            <view 
              class="mode-btn"
              :class="{ 'active': mode === 'image' }"
              @click="mode = 'image'"
            >
              <text class="mode-text">导入图片</text>
            </view>
          </view>
        </view>
        
        <!-- 可滚动内容区域 -->
        <scroll-view scroll-y class="content-area step-1-scroll">
          <view 
            class="step-content step-size"
            :class="stepAnimationClass"
          >
        <!-- 图片上传（仅图片模式） -->
        <view v-if="mode === 'image'" class="image-upload">
          <view v-if="!previewUrl" class="upload-area" @click="chooseImage">
            <view class="upload-icon-wrapper">
              <Icon name="upload" :size="64" />
            </view>
            <text class="upload-text">点击上传图片</text>
            <text class="upload-hint">支持 JPG, PNG</text>
          </view>
          
          <view v-else class="preview-area">
            <image :src="previewUrl" mode="aspectFit" class="preview-image" />
            <view class="preview-actions">
              <view class="preview-btn" @click="chooseImage">
                <text class="preview-btn-text">更换图片</text>
              </view>
              <view class="preview-btn danger" @click="clearImage">
                <Icon name="close" :size="28" />
              </view>
            </view>
          </view>
        </view>
        
        <!-- 尺寸选择 -->
        <view class="size-section">
          <view class="size-header">
            <Icon name="column-4" :size="32" />
            <text class="size-title">{{ mode === 'image' ? '目标尺寸 (像素化)' : '画布尺寸' }}</text>
          </view>
          
          <view class="size-grid">
            <!-- 自定义尺寸输入 -->
            <view class="size-column full-width">
              <view class="size-column-header">
                <text class="size-label">自定义尺寸</text>
                <view v-if="customWidth > 0 && customHeight > 0" class="size-preview-inline">
                  <text class="size-preview-value">{{ customWidth }}×{{ customHeight }}</text>
                </view>
                <text v-else class="size-hint">最大520×520像素</text>
              </view>
              
              <view class="custom-size-inputs">
                <view class="size-input-group">
                  <text class="size-input-label">宽度</text>
                  <input 
                    v-model.number="customWidth"
                    type="number"
                    class="size-input"
                    placeholder="例如: 30"
                    @input="handleSizeInput"
                  />
                </view>
                <text class="size-separator">×</text>
                <view class="size-input-group">
                  <text class="size-input-label">高度</text>
                  <input 
                    v-model.number="customHeight"
                    type="number"
                    class="size-input"
                    placeholder="例如: 20"
                    @input="handleSizeInput"
                  />
                </view>
              </view>
            </view>
            
            <!-- 快捷尺寸选择 -->
            <view class="size-column full-width">
              <view class="size-column-header">
                <text class="size-label">快捷尺寸 (正方形)</text>
                <text class="size-hint">52的倍数标准尺寸</text>
              </view>
              
              <view class="size-options">
                <view 
                  v-for="preset in sizePresets"
                  :key="`preset-${preset.width}-${preset.height}`"
                  class="size-option"
                  :class="{ 'active': customWidth === preset.width && customHeight === preset.height }"
                  @click="selectPresetSize(preset.width, preset.height)"
                >
                  <text class="size-option-text">{{ preset.width }}×{{ preset.height }}</text>
                </view>
              </view>
            </view>
          </view>
        </view>
        
        <!-- 统计信息 -->
        <view v-if="customWidth > 0 && customHeight > 0" class="stats-card">
          <view class="stat-row">
            <text class="stat-label">板子布局</text>
            <text class="stat-value">{{ boardsX }}×{{ boardsY }} ({{ totalBoards }}块)</text>
          </view>
          <view class="stat-row">
            <text class="stat-label">总像素数</text>
            <text class="stat-value">{{ (customWidth * customHeight).toLocaleString() }}</text>
          </view>
        </view>
          </view>
        </scroll-view>
      </view>
      
      <!-- 步骤 2: 颜色 -->
      <scroll-view 
        v-if="step === 2"
        scroll-y 
        class="content-area"
      >
        <view 
          class="step-content step-colors"
          :class="stepAnimationClass"
        >
          <view class="colors-header">
          <Icon name="picture" :size="32" />
          <text class="colors-title">选择 Artkal 拼豆套装</text>
        </view>
        
        <view class="preset-grid">
          <view 
            v-for="preset in presets"
            :key="preset.key"
            class="preset-card"
            :class="{ 'active': selectedPreset === preset.key }"
            @click="handlePresetChange(preset.key)"
          >
            <text class="preset-name">{{ preset.name }}</text>
            <text class="preset-count">{{ preset.count }} 种颜色</text>
          </view>
        </view>
        
        <view class="selected-info">
          <view class="info-row">
            <text class="info-label">已选择套装</text>
            <text class="info-value">{{ selectedColors.size }} 种颜色</text>
          </view>
          <text class="info-hint">套装包含精心挑选的 Artkal 拼豆颜色，适合各种创作需求</text>
        </view>
      </view>
    </scroll-view>
    
    <!-- 步骤 3: 预览确认 (仅图片模式) -->
    <scroll-view 
      v-if="step === 3"
      scroll-y 
      class="content-area"
    >
      <view 
        class="step-content step-preview"
        :class="stepAnimationClass"
      >
        <view class="preview-header">
          <text class="preview-title">预览效果</text>
          <text class="preview-subtitle">确认后将创建画布</text>
        </view>
        
        <!-- 预览图 -->
        <view class="preview-canvas-wrapper">
          <!-- #ifdef H5 -->
          <canvas 
            canvas-id="previewDisplayCanvas" 
            id="previewDisplayCanvas"
            type="2d"
            class="preview-canvas"
          ></canvas>
          <!-- #endif -->
          
          <!-- #ifdef MP-WEIXIN -->
          <image 
            v-if="previewImageUrl"
            :src="previewImageUrl"
            class="preview-image"
            mode="aspectFit"
          />
          <canvas 
            v-else
            canvas-id="previewDisplayCanvas" 
            id="previewDisplayCanvas"
            type="2d"
            class="preview-canvas"
            style="position: absolute; left: -9999px; opacity: 0;"
          ></canvas>
          <!-- #endif -->
        </view>
        
        <!-- 统计信息 -->
        <view class="preview-stats">
          <view class="preview-stat-card">
            <text class="preview-stat-label">画布尺寸</text>
            <text class="preview-stat-value">{{ customWidth }}×{{ customHeight }}</text>
          </view>
          
          <view class="preview-stat-card">
            <text class="preview-stat-label">板子数量</text>
            <text class="preview-stat-value">{{ totalBoards }} 块</text>
          </view>
          
          <view class="preview-stat-card">
            <text class="preview-stat-label">使用颜色</text>
            <text class="preview-stat-value">{{ usedColors.length }} 种</text>
          </view>
        </view>
        
        <!-- 使用的颜色列表 -->
        <view class="used-colors-section">
          <view class="used-colors-header">
            <text class="used-colors-title">使用的颜色</text>
            <text class="used-colors-total">共 {{ usedColors.reduce((sum, c) => sum + c.count, 0) }} 个</text>
          </view>
          <view class="used-colors-grid">
            <view 
              v-for="color in usedColors"
              :key="color.code"
              class="used-color-item"
            >
              <view class="color-swatch" :style="{ backgroundColor: color.hex }"></view>
              <text class="color-code">{{ color.code }}</text>
              <text class="color-count">{{ color.count }}</text>
            </view>
          </view>
        </view>
      </view>
    </scroll-view>
    
    <!-- 底部按钮 -->
    <view class="footer">
      <view 
        class="next-btn"
        :class="{ 'disabled': !canProceed || isProcessing }"
        @click="handleNext"
      >
        <text class="next-btn-text">{{ buttonText }}</text>
        <text class="next-icon">›</text>
      </view>
    </view>
    
    <!-- 隐藏的 Canvas 用于图片导入 -->
    <!-- #ifdef MP-WEIXIN -->
    <canvas 
      v-if="mode === 'image'" 
      canvas-id="importCanvas" 
      id="importCanvas"
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 1px; height: 1px;"
    />
    <!-- #endif -->
    
    <!-- #ifdef H5 -->
    <canvas 
      v-if="mode === 'image'" 
      canvas-id="importCanvas" 
      id="importCanvas" 
      style="position: fixed; left: -9999px; top: -9999px; width: 1px; height: 1px;"
    />
    <!-- #endif -->
    
    <!-- 隐藏的 Canvas 用于保存缩略图 -->
    <canvas 
      canvas-id="thumbnailSaveCanvas" 
      id="thumbnailSaveCanvas"
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 100px; height: 100px; opacity: 0; pointer-events: none;"
    />
    
    <!-- 隐藏的临时处理canvas（仅小程序使用） -->
    <!-- #ifdef MP-WEIXIN -->
    <canvas 
      canvas-id="tempProcessCanvas" 
      id="tempProcessCanvas"
      type="2d"
      style="position: absolute; left: -9999px; top: -9999px; width: 520px; height: 520px; opacity: 0; pointer-events: none;"
    />
    <!-- #endif -->
    
    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" />
    
    <!-- 需要登录弹窗 -->
    <ConfirmModal
      :visible.sync="showLoginModal"
      title="需要登录"
      content="创建项目需要先登录，是否前往登录？"
      confirmText="前往登录"
      cancelText="取消"
      @confirm="goToLogin"
    />
  </view>
</template>

<script>
import { useThemeStore } from '../../store/theme.js'
import { useUserStore } from '../../store/user.js'
import { useProjectStore } from '../../store/project.js'
import { useToast } from '../../composables/useToast.js'
import { ARTKAL_COLORS_FULL, ARTKAL_OFFICIAL_SETS, ARTKAL_PRESETS } from '../../data/artkal-colors-full.js'
import statusBarMixin from '../../mixins/statusBar.js'
import Icon from '../../components/Icon.vue'
import Toast from '../../components/Toast.vue'
import ConfirmModal from '../../components/ConfirmModal.vue'

export default {
  mixins: [statusBarMixin],
  components: {
    Icon,
    Toast,
    ConfirmModal
  },
  data() {
    return {
      themeStore: null,
      projectStore: null,
      toast: null,
      
      name: '',
      customWidth: 52,
      customHeight: 52,
      selectedPreset: 'set24',
      selectedColors: new Set(ARTKAL_OFFICIAL_SETS.set24.colors),
      step: 0,
      mode: 'blank',
      imageFile: null,
      previewUrl: null,
      previewPixels: null, // 预览的像素数据
      previewCanvas: null, // 预览用的canvas
      previewImageUrl: '', // 小程序预览图片URL
      usedColors: [], // 使用的颜色列表
      isProcessing: false,
      stepAnimationClass: 'step-enter',
      isNameFocused: false,
      
      presets: ARTKAL_PRESETS,
      sizePresets: [
        { width: 52, height: 52 },
        { width: 104, height: 104 },
        { width: 156, height: 156 },
        { width: 208, height: 208 },
        { width: 260, height: 260 },
        { width: 312, height: 312 },
        { width: 364, height: 364 },
        { width: 416, height: 416 },
        { width: 468, height: 468 },
        { width: 520, height: 520 }
      ],
      showLoginModal: false // 需要登录弹窗
    }
  },
  
  computed: {
    // 计算填充后的宽度（向上取整到52的倍数）
    paddedWidth() {
      return Math.ceil(this.customWidth / 52) * 52
    },
    
    // 计算填充后的高度（向上取整到52的倍数）
    paddedHeight() {
      return Math.ceil(this.customHeight / 52) * 52
    },
    
    // 计算横向板子数量
    boardsX() {
      return Math.ceil(this.customWidth / 52)
    },
    
    // 计算纵向板子数量
    boardsY() {
      return Math.ceil(this.customHeight / 52)
    },
    
    // 总板子数量
    totalBoards() {
      return this.boardsX * this.boardsY
    },
    
    canProceed() {
      if (this.step === 0) return this.name.trim().length > 0
      if (this.step === 1) {
        if (this.mode === 'image' && !this.imageFile) return false
        // 检查尺寸是否有效且不超过最大限制
        return this.customWidth > 0 && 
               this.customHeight > 0 && 
               this.paddedWidth <= 520 && 
               this.paddedHeight <= 520
      }
      if (this.step === 2) return true
      if (this.step === 3) return true // 预览步骤
      return true
    },
    
    buttonText() {
      if (this.step === 0) return '下一步'
      if (this.step === 1) return '选择颜色'
      if (this.step === 2 && this.mode === 'image') {
        if (this.isProcessing) return '正在生成中...'
        return '生成预览'
      }
      if (this.step === 2) return '创建画布'
      if (this.step === 3) {
        if (this.isProcessing) return '生成中...'
        return '确认创建'
      }
      return '下一步'
    }
  },
  
  onLoad() {
    // 检查登录状态
    const userStore = useUserStore()
    if (!userStore.hasLogin) {
      this.showLoginModal = true
      return
    }
    
    this.themeStore = useThemeStore()
    this.projectStore = useProjectStore()
    this.toast = useToast()
    
    // 注册自定义 Toast 实例
    this.$nextTick(() => {
      if (this.$refs.toastRef) {
        this.toast.setToastInstance(this.$refs.toastRef)
      }
    })
    
    // 立即应用主题，避免闪烁
    this.themeStore.applyTheme()
  },
  
  onShow() {
    this.themeStore.applyTheme()
  },
  
  methods: {
    // 前往登录
    goToLogin() {
      uni.reLaunch({
        url: '/pages/user/user'
      })
    },
    
    // 检测图片是否可能是像素画
    detectPixelArt(imageData, width, height) {
      const data = imageData.data
      const colorSet = new Set()
      let totalPixels = 0
      
      // 采样检测（不需要检查所有像素）
      const sampleRate = Math.max(1, Math.floor(width * height / 10000))
      
      for (let i = 0; i < data.length; i += 4 * sampleRate) {
        const a = data[i + 3]
        if (a < 128) continue // 跳过透明像素
        
        const r = data[i]
        const g = data[i + 1]
        const b = data[i + 2]
        const color = `${r},${g},${b}`
        colorSet.add(color)
        totalPixels++
      }
      
      // 像素画特征：颜色数量相对较少
      const uniqueColors = colorSet.size
      const colorRatio = uniqueColors / totalPixels
      
      // 如果颜色种类占比小于30%，很可能是像素画
      return colorRatio < 0.3 && uniqueColors < 256
    },
    
    handleSizeInput() {
      // 验证尺寸
      if (this.customWidth < 1) this.customWidth = 1
      if (this.customHeight < 1) this.customHeight = 1
      
      // 检查是否超过最大限制
      if (this.paddedWidth > 520) {
        this.customWidth = 520
        this.toast.showError('宽度最大为520像素（10块板子）')
      }
      if (this.paddedHeight > 520) {
        this.customHeight = 520
        this.toast.showError('高度最大为520像素（10块板子）')
      }
    },
    
    selectPresetSize(width, height) {
      this.customWidth = width
      this.customHeight = height
    },
    
    handleBack() {
      // 如果正在处理，禁止返回
      if (this.isProcessing) {
        return
      }
      
      if (this.step === 0) {
        uni.reLaunch({
          url: '/pages/library/library'
        })
      } else {
        this.stepAnimationClass = 'step-exit-reverse'
        setTimeout(() => {
          this.step--
          this.stepAnimationClass = 'step-enter-reverse'
        }, 150)
      }
    },
    
    handlePresetChange(preset) {
      this.selectedPreset = preset
      this.selectedColors = new Set(ARTKAL_OFFICIAL_SETS[preset].colors)
    },
    
    chooseImage() {
      uni.chooseImage({
        count: 1,
        sizeType: ['compressed'],
        sourceType: ['album', 'camera'],
        success: (res) => {
          this.imageFile = res.tempFilePaths[0]
          this.previewUrl = res.tempFilePaths[0]
        }
      })
    },
    
    clearImage() {
      this.imageFile = null
      this.previewUrl = null
    },
    
    async generatePreview() {
      if (!this.imageFile) return
      
      this.isProcessing = true
      this.previewImageUrl = '' // 清空之前的预览图
      this.toast.showInfo('正在生成预览...')
      
      try {
        // 使用已导入的 ARTKAL_COLORS_FULL
        const selectedArtkalColors = Array.from(this.selectedColors)
          .map(code => ARTKAL_COLORS_FULL.find(c => c.code === code))
          .filter(c => c !== undefined)
        
        const artkalPalette = selectedArtkalColors.map(c => c.hex)
        
        // #ifdef H5
        // H5环境：使用Image和document
        const img = new Image()
        img.crossOrigin = 'anonymous'
        
        img.onload = () => {
          // 先绘制原始尺寸以检测是否为像素画
          const tempCanvas = document.createElement('canvas')
          tempCanvas.width = img.width
          tempCanvas.height = img.height
          const tempCtx = tempCanvas.getContext('2d')
          tempCtx.drawImage(img, 0, 0)
          const detectData = tempCtx.getImageData(0, 0, img.width, img.height)
          
          // 检测是否为像素画
          const isPixelArt = this.detectPixelArt(detectData, img.width, img.height)
          console.log('检测到像素画:', isPixelArt, '原图尺寸:', img.width, 'x', img.height, '目标尺寸:', this.customWidth, 'x', this.customHeight)
          
          if (isPixelArt) {
            this.toast.showSuccess('检测到像素画，已自动优化处理')
          }
          
          // 创建最终canvas
          const canvas = document.createElement('canvas')
          canvas.width = this.customWidth
          canvas.height = this.customHeight
          const ctx = canvas.getContext('2d')
          if (!ctx) {
            this.toast.showError('Canvas 初始化失败')
            this.isProcessing = false
            return
          }
          
          if (isPixelArt) {
            // 像素画模式：使用整数倍缩放策略
            ctx.imageSmoothingEnabled = false
            ctx.mozImageSmoothingEnabled = false
            ctx.webkitImageSmoothingEnabled = false
            ctx.msImageSmoothingEnabled = false
            
            // 计算最接近的整数缩放比例
            const scaleX = this.customWidth / img.width
            const scaleY = this.customHeight / img.height
            const scale = Math.max(scaleX, scaleY)
            
            // 如果需要放大，使用向上取整；如果缩小，使用向下取整
            let integerScale
            if (scale >= 1) {
              integerScale = Math.ceil(scale)
            } else {
              // 缩小时，找最接近的整数分数（1/2, 1/3, 1/4...）
              integerScale = 1 / Math.ceil(1 / scale)
            }
            
            // 计算中间尺寸
            const intermediateWidth = Math.round(img.width * integerScale)
            const intermediateHeight = Math.round(img.height * integerScale)
            
            // 创建中间canvas进行整数倍缩放
            const intermediateCanvas = document.createElement('canvas')
            intermediateCanvas.width = intermediateWidth
            intermediateCanvas.height = intermediateHeight
            const intermediateCtx = intermediateCanvas.getContext('2d')
            intermediateCtx.imageSmoothingEnabled = false
            intermediateCtx.mozImageSmoothingEnabled = false
            intermediateCtx.webkitImageSmoothingEnabled = false
            intermediateCtx.msImageSmoothingEnabled = false
            
            // 整数倍缩放
            intermediateCtx.drawImage(img, 0, 0, intermediateWidth, intermediateHeight)
            
            // 居中裁剪到目标尺寸
            const offsetX = Math.floor((intermediateWidth - this.customWidth) / 2)
            const offsetY = Math.floor((intermediateHeight - this.customHeight) / 2)
            
            ctx.drawImage(
              intermediateCanvas,
              Math.max(0, offsetX),
              Math.max(0, offsetY),
              Math.min(intermediateWidth, this.customWidth),
              Math.min(intermediateHeight, this.customHeight),
              Math.max(0, -offsetX),
              Math.max(0, -offsetY),
              Math.min(intermediateWidth, this.customWidth),
              Math.min(intermediateHeight, this.customHeight)
            )
            
            console.log('像素画模式：整数倍缩放', integerScale, '中间尺寸:', intermediateWidth, 'x', intermediateHeight)
          } else {
            // 普通图片模式：直接高质量缩放
            ctx.imageSmoothingEnabled = true
            ctx.imageSmoothingQuality = 'high'
            ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight)
          }
          
          const imageData = ctx.getImageData(0, 0, this.customWidth, this.customHeight)
          const data = imageData.data
          
          this.processImageData(data, artkalPalette, selectedArtkalColors)
        }
        
        img.onerror = () => {
          this.toast.showError('图片加载失败')
          this.isProcessing = false
        }
        
        img.src = this.imageFile
        // #endif
        
        // #ifdef MP-WEIXIN
        // 小程序环境：使用uni.getImageInfo和canvas
        console.log('小程序环境：开始处理图片')
        
        const imageInfo = await new Promise((resolve, reject) => {
          uni.getImageInfo({
            src: this.imageFile,
            success: resolve,
            fail: reject
          })
        })
        
        console.log('图片信息获取成功:', imageInfo)
        
        // 延迟等待canvas准备好
        await new Promise(resolve => setTimeout(resolve, 500))
        
        // 创建临时canvas处理图片
        const query = uni.createSelectorQuery().in(this)
        query.select('#tempProcessCanvas')
          .fields({ node: true, size: true })
          .exec((res) => {
            console.log('Canvas查询结果:', res)
            
            if (!res || !res[0] || !res[0].node) {
              console.error('Canvas节点未找到')
              this.toast.showError('Canvas 初始化失败，请重试')
              this.isProcessing = false
              return
            }
            
            const canvas = res[0].node
            const ctx = canvas.getContext('2d', { willReadFrequently: true })
            
            if (!ctx) {
              console.error('Canvas context 获取失败')
              this.toast.showError('Canvas context 初始化失败')
              this.isProcessing = false
              return
            }
            
            console.log('Canvas初始化成功，开始绘制')
            
            canvas.width = this.customWidth
            canvas.height = this.customHeight
            
            const img = canvas.createImage()
            img.onload = () => {
              console.log('图片加载成功，开始处理像素')
              
              // 先绘制原始尺寸以检测是否为像素画
              const tempCanvas = canvas
              const tempCtx = ctx
              tempCanvas.width = img.width
              tempCanvas.height = img.height
              tempCtx.drawImage(img, 0, 0)
              const detectData = tempCtx.getImageData(0, 0, img.width, img.height)
              
              // 检测是否为像素画
              const isPixelArt = this.detectPixelArt(detectData, img.width, img.height)
              console.log('检测到像素画:', isPixelArt, '原图尺寸:', img.width, 'x', img.height, '目标尺寸:', this.customWidth, 'x', this.customHeight)
              
              if (isPixelArt) {
                this.toast.showSuccess('检测到像素画，已自动优化处理')
              }
              
              // 重置canvas为目标尺寸
              canvas.width = this.customWidth
              canvas.height = this.customHeight
              
              if (isPixelArt) {
                // 像素画模式：使用整数倍缩放策略
                ctx.imageSmoothingEnabled = false
                
                // 计算最接近的整数缩放比例
                const scaleX = this.customWidth / img.width
                const scaleY = this.customHeight / img.height
                const scale = Math.max(scaleX, scaleY)
                
                // 如果需要放大，使用向上取整；如果缩小，使用向下取整
                let integerScale
                if (scale >= 1) {
                  integerScale = Math.ceil(scale)
                } else {
                  // 缩小时，找最接近的整数分数（1/2, 1/3, 1/4...）
                  integerScale = 1 / Math.ceil(1 / scale)
                }
                
                // 计算中间尺寸
                const intermediateWidth = Math.round(img.width * integerScale)
                const intermediateHeight = Math.round(img.height * integerScale)
                
                // 创建临时canvas进行整数倍缩放
                const query2 = uni.createSelectorQuery().in(this)
                query2.select('#importCanvas')
                  .fields({ node: true })
                  .exec((res2) => {
                    if (!res2 || !res2[0] || !res2[0].node) {
                      // 如果获取不到临时canvas，直接缩放
                      ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight)
                      const imageData = ctx.getImageData(0, 0, this.customWidth, this.customHeight)
                      this.processImageData(imageData.data, artkalPalette, selectedArtkalColors)
                      return
                    }
                    
                    const intermediateCanvas = res2[0].node
                    const intermediateCtx = intermediateCanvas.getContext('2d')
                    intermediateCanvas.width = intermediateWidth
                    intermediateCanvas.height = intermediateHeight
                    intermediateCtx.imageSmoothingEnabled = false
                    
                    // 整数倍缩放
                    intermediateCtx.drawImage(img, 0, 0, intermediateWidth, intermediateHeight)
                    
                    // 居中裁剪到目标尺寸
                    const offsetX = Math.floor((intermediateWidth - this.customWidth) / 2)
                    const offsetY = Math.floor((intermediateHeight - this.customHeight) / 2)
                    
                    ctx.drawImage(
                      intermediateCanvas,
                      Math.max(0, offsetX),
                      Math.max(0, offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight),
                      Math.max(0, -offsetX),
                      Math.max(0, -offsetY),
                      Math.min(intermediateWidth, this.customWidth),
                      Math.min(intermediateHeight, this.customHeight)
                    )
                    
                    console.log('像素画模式：整数倍缩放', integerScale, '中间尺寸:', intermediateWidth, 'x', intermediateHeight)
                    
                    const imageData = ctx.getImageData(0, 0, this.customWidth, this.customHeight)
                    this.processImageData(imageData.data, artkalPalette, selectedArtkalColors)
                  })
              } else {
                // 普通图片模式：直接高质量缩放
                ctx.imageSmoothingEnabled = true
                ctx.drawImage(img, 0, 0, this.customWidth, this.customHeight)
                const imageData = ctx.getImageData(0, 0, this.customWidth, this.customHeight)
                this.processImageData(imageData.data, artkalPalette, selectedArtkalColors)
              }
            }
            
            img.onerror = (err) => {
              console.error('图片加载失败:', err)
              this.toast.showError('图片加载失败')
              this.isProcessing = false
            }
            
            img.src = this.imageFile
          })
        // #endif
        
      } catch (error) {
        console.error('生成预览失败:', error)
        this.toast.showError('生成预览失败，请重试')
        this.isProcessing = false
      }
    },
    
    processImageData(data, artkalPalette, selectedArtkalColors) {
      // 计算偏移量（居中）
      const offsetX = Math.floor((this.paddedWidth - this.customWidth) / 2)
      const offsetY = Math.floor((this.paddedHeight - this.customHeight) / 2)
      
      const artkalPixels = new Map()
      const colorUsage = new Map() // 统计颜色使用次数
      
      // 将每个像素映射到最接近的 Artkal 颜色
      for (let y = 0; y < this.customHeight; y++) {
        for (let x = 0; x < this.customWidth; x++) {
          const idx = (y * this.customWidth + x) * 4
          const alpha = data[idx + 3]
          
          // 跳过透明像素
          if (alpha < 128) continue
          
          const r = data[idx]
          const g = data[idx + 1]
          const b = data[idx + 2]
          
          // 找到最接近的 Artkal 颜色
          let minDist = Infinity
          let closestColor = artkalPalette[0]
          
          for (const artkalHex of artkalPalette) {
            const artkalRgb = this.hexToRgb(artkalHex)
            const dist = Math.sqrt(
              Math.pow(r - artkalRgb[0], 2) +
              Math.pow(g - artkalRgb[1], 2) +
              Math.pow(b - artkalRgb[2], 2)
            )
            if (dist < minDist) {
              minDist = dist
              closestColor = artkalHex
            }
          }
          
          // 使用偏移后的坐标
          artkalPixels.set(`${x + offsetX},${y + offsetY}`, closestColor)
          
          // 统计颜色使用次数
          colorUsage.set(closestColor, (colorUsage.get(closestColor) || 0) + 1)
        }
      }
      
      // 保存预览数据
      this.previewPixels = artkalPixels
      
      // 生成使用的颜色列表
      this.usedColors = Array.from(colorUsage.entries())
        .map(([hex, count]) => {
          const colorInfo = selectedArtkalColors.find(c => c.hex === hex)
          return {
            hex,
            code: colorInfo?.code || '',
            name: colorInfo?.name || '',
            count
          }
        })
        .sort((a, b) => b.count - a.count) // 按使用次数排序
      
      // 绘制预览
      this.drawPreview(artkalPixels)
      
      // 进入预览步骤
      this.isProcessing = false
      this.stepAnimationClass = 'step-exit'
      setTimeout(() => {
        this.step = 3
        this.stepAnimationClass = 'step-enter'
      }, 150)
    },
    
    drawPreview(pixels) {
      setTimeout(() => {
        const query = uni.createSelectorQuery().in(this)
        query.select('#previewDisplayCanvas')
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              console.error('Canvas not found, 重试中...')
              // 重试一次
              setTimeout(() => {
                this.drawPreview(pixels)
              }, 500)
              return
            }
            
            const canvas = res[0].node
            if (!canvas) {
              console.error('Canvas node is null')
              return
            }
            
            const ctx = canvas.getContext('2d')
            if (!ctx) {
              console.error('Canvas context is null')
              return
            }
            
            const containerWidth = res[0].width
            const containerHeight = res[0].height
            
            // canvas 物理尺寸 = 容器尺寸
            canvas.width = containerWidth
            canvas.height = containerHeight
            
            // 清空背景
            ctx.fillStyle = (this.themeStore && this.themeStore.isDarkMode) ? '#1a1a1a' : '#f5f5f5'
            ctx.fillRect(0, 0, containerWidth, containerHeight)
            
            // 使用最大边作为正方形基准
            const maxDimension = Math.max(this.paddedWidth, this.paddedHeight)
            
            // 根据项目大小选择倍数
            // #ifdef H5
            const multiplier = (maxDimension > 52) ? 3 : 2
            // #endif
            
            // #ifdef MP-WEIXIN
            const multiplier = (maxDimension > 52) ? 1.5 : 1
            // #endif
            
            let scale = Math.min(
              containerWidth / (maxDimension * multiplier), 
              containerHeight / (maxDimension * multiplier)
            )
            // scale >= 1 取整，scale < 1 保留两位小数
            if (scale >= 4) {
              // #ifdef H5
              scale = 1.65
              // #endif
              
              // #ifdef MP-WEIXIN
              scale = 4
              // #endif
            } else {
              // #ifdef H5
              scale = (Math.round(scale * 100) / 100)
              // #endif
              
              // #ifdef MP-WEIXIN
              scale = (Math.round(scale * 100) / 100) * 1.5
              // #endif
              
            }
            // 计算画布内容的实际尺寸
            const contentWidth = this.paddedWidth * scale
            const contentHeight = this.paddedHeight * scale
            
            // 计算居中偏移量（让整个画布内容在canvas中居中）
            const canvasCenterOffsetX = (containerWidth - contentWidth) / 2
            const canvasCenterOffsetY = (containerHeight - contentHeight) / 2
            
            // 计算图形在画布内的居中偏移量
            const centerOffsetX = (maxDimension - this.paddedWidth) / 2
            const centerOffsetY = (maxDimension - this.paddedHeight) / 2
            
            // 绘制像素（加上两层偏移）
            pixels.forEach((color, key) => {
              const [x, y] = key.split(',').map(Number)
              ctx.fillStyle = color
              ctx.fillRect(
                canvasCenterOffsetX + (x + centerOffsetX) * scale, 
                canvasCenterOffsetY + (y + centerOffsetY) * scale, 
                scale, 
                scale
              )
            })
            
            console.log('Preview drawn:', pixels.size, 'pixels', 'scale:', scale, 'canvasOffset:', canvasCenterOffsetX, canvasCenterOffsetY)
            
            // #ifdef MP-WEIXIN
            // 小程序：将canvas转为图片
            setTimeout(() => {
              uni.canvasToTempFilePath({
                canvas: canvas,
                success: (res) => {
                  this.previewImageUrl = res.tempFilePath
                  console.log('Canvas转图片成功:', res.tempFilePath)
                },
                fail: (err) => {
                  console.error('Canvas转图片失败:', err)
                }
              }, this)
            }, 100)
            // #endif
          })
      }, 500)
    },
    
    clearImage() {
      this.imageFile = null
      this.previewUrl = null
      this.previewPixels = null
      this.usedColors = []
    },
    
    handleNext() {
      if (!this.canProceed || this.isProcessing) return
      
      if (this.step < 2) {
        this.stepAnimationClass = 'step-exit'
        setTimeout(() => {
          this.step++
          this.stepAnimationClass = 'step-enter'
        }, 150)
      } else if (this.step === 2) {
        // 如果是图片模式，进入预览步骤
        if (this.mode === 'image') {
          this.generatePreview()
        } else {
          // 空白画布直接创建
          this.handleCreate()
        }
      } else if (this.step === 3) {
        // 预览步骤，确认创建
        this.handleCreate()
      }
    },
    
    async handleCreate() {
      if (this.mode === 'image' && this.previewPixels) {
        // 使用预览数据直接创建
        this.isProcessing = true
        
        try {
          const projectId = this.projectStore.addProject(
            this.name,
            this.customWidth,
            this.customHeight,
            this.selectedColors.size,
            Array.from(this.selectedColors),
            this.paddedWidth,
            this.paddedHeight
          )
          
          // 保存像素数据
          const pixelsData = JSON.stringify(Array.from(this.previewPixels.entries()))
          
          // 检查数据大小
          // #ifdef H5
          const dataSizeKB = new Blob([pixelsData]).size / 1024
          console.log(`像素数据大小: ${dataSizeKB.toFixed(2)} KB`)
          
          if (dataSizeKB > 5000) {
            this.toast.showError('图片过大，建议使用较小的尺寸')
            this.projectStore.deleteProject(projectId)
            this.isProcessing = false
            return
          }
          // #endif
          
          // #ifdef MP-WEIXIN
          const dataSizeKB = pixelsData.length / 1024
          console.log(`像素数据大小: ${dataSizeKB.toFixed(2)} KB`)
          
          if (dataSizeKB > 5000) {
            this.toast.showError('图片过大，建议使用较小的尺寸')
            this.projectStore.deleteProject(projectId)
            this.isProcessing = false
            return
          }
          // #endif
          
          uni.setStorageSync(`pixels-${projectId}`, pixelsData)
          
          // 生成并保存缩略图，完成后再跳转
          this.saveThumbnailForProject(projectId, this.previewPixels, () => {
            this.toast.showSuccess('画布创建成功！')
            this.isProcessing = false
            
            uni.reLaunch({
              url: '/pages/library/library'
            })
          })
        } catch (error) {
          console.error('保存失败:', error)
          this.toast.showError('保存失败，请重试')
          this.isProcessing = false
        }
      } else if (this.mode === 'blank') {
        // 空白画布直接创建
        const projectId = this.projectStore.addProject(
          this.name,
          this.customWidth,
          this.customHeight,
          this.selectedColors.size,
          Array.from(this.selectedColors),
          this.paddedWidth,
          this.paddedHeight
        )
        
        console.log('空白画布创建，projectId:', projectId)
        
        // 空白画布生成一个空缩略图，完成后再跳转
        this.saveThumbnailForProject(projectId, new Map(), () => {
          this.toast.showSuccess('画布创建成功！')
          uni.reLaunch({
            url: '/pages/library/library'
          })
        })
      }
    },
    
    saveThumbnailForProject(projectId, pixels, callback) {
      console.log('开始生成缩略图，projectId:', projectId, '像素数量:', pixels.size)
      
      const query = uni.createSelectorQuery().in(this)
      query.select('#thumbnailSaveCanvas')
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0] || !res[0].node) {
            console.error('Thumbnail canvas not found')
            if (callback) callback()
            return
          }
          
          const canvas = res[0].node
          const ctx = canvas.getContext('2d')
          if (!ctx) {
            console.error('Canvas context is null')
            if (callback) callback()
            return
          }
          
          // 缩小到 100x100 节省存储空间
          const thumbSize = 100
          canvas.width = thumbSize
          canvas.height = thumbSize
          
          ctx.clearRect(0, 0, thumbSize, thumbSize)
          
          // 计算缩放比例，让图片尽可能填充画布
          const scale = Math.min(thumbSize / this.paddedWidth, thumbSize / this.paddedHeight) * 0.9
          
          // 计算居中偏移
          const scaledWidth = this.paddedWidth * scale
          const scaledHeight = this.paddedHeight * scale
          const offsetX = (thumbSize - scaledWidth) / 2
          const offsetY = (thumbSize - scaledHeight) / 2
          
          // 绘制像素
          pixels.forEach((color, key) => {
            const [x, y] = key.split(',').map(Number)
            ctx.fillStyle = color
            ctx.fillRect(
              offsetX + x * scale, 
              offsetY + y * scale, 
              scale, 
              scale
            )
          })
          
          // 等待绘制完成后转换为 Base64
          setTimeout(() => {
            uni.canvasToTempFilePath({
              canvas: canvas,
              x: 0,
              y: 0,
              width: thumbSize,
              height: thumbSize,
              destWidth: thumbSize,
              destHeight: thumbSize,
              fileType: 'png',
              quality: 0.8,
              success: (res) => {
                // #ifdef MP-WEIXIN
                // 小程序：转为 Base64
                const fs = uni.getFileSystemManager()
                fs.readFile({
                  filePath: res.tempFilePath,
                  encoding: 'base64',
                  success: (fileRes) => {
                    const base64 = 'data:image/png;base64,' + fileRes.data
                    console.log('缩略图生成成功 (Base64):', Math.round(base64.length / 1024), 'KB')
                    this.projectStore.updateProjectThumbnail(projectId, base64)
                    if (callback) callback()
                  },
                  fail: (err) => {
                    console.error('读取缩略图文件失败:', err)
                    // 降级：保存临时文件路径
                    this.projectStore.updateProjectThumbnail(projectId, res.tempFilePath)
                    if (callback) callback()
                  }
                })
                // #endif
                
                // #ifdef H5
                // H5：直接使用 canvas.toDataURL
                try {
                  const base64 = canvas.toDataURL('image/png', 0.8)
                  console.log('缩略图生成成功 (Base64):', Math.round(base64.length / 1024), 'KB')
                  this.projectStore.updateProjectThumbnail(projectId, base64)
                  if (callback) callback()
                } catch (err) {
                  console.error('生成 Base64 失败:', err)
                  // 降级：保存临时文件路径
                  this.projectStore.updateProjectThumbnail(projectId, res.tempFilePath)
                  if (callback) callback()
                }
                // #endif
              },
              fail: (err) => {
                console.error('保存项目缩略图失败:', err)
                if (callback) callback()
              }
            }, this)
          }, 300)
        })
    },
    
    hexToRgb(hex) {
      const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
      return result ? [
        parseInt(result[1], 16),
        parseInt(result[2], 16),
        parseInt(result[3], 16)
      ] : [0, 0, 0]
    }
  }
}
</script>

<style scoped>
.create-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-secondary);
  position: relative;
  overflow: hidden;
  user-select: none;
  -webkit-user-drag: none;
  -webkit-touch-callout: none;
  box-sizing: border-box;
}

/* 背景装饰 */
.bg-decoration {
  position: absolute;
  border-radius: 50%;
  filter: blur(120rpx);
  opacity: 0.1;
  pointer-events: none;
  z-index: 0;
}

/* 状态栏占位 */
.status-bar {
  background-color: var(--bg-secondary);
  flex-shrink: 0;
}

.bg-decoration-1 {
  top: 0;
  right: 0;
  width: 400rpx;
  height: 400rpx;
  background-color: var(--accent-primary);
}

.bg-decoration-2 {
  bottom: 0;
  left: 0;
  width: 400rpx;
  height: 400rpx;
  background-color: var(--accent-secondary);
}

.header {
  display: flex;
  align-items: center;
  padding: 32rpx;
  z-index: 10;
  flex-shrink: 0;
  box-sizing: border-box;
}

/* 小程序需要额外的顶部padding */
/* #ifdef MP-WEIXIN */
.header {
  padding-top: 16rpx;
}
/* #endif */

.back-btn {
  width: 64rpx;
  height: 64rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  margin-right: 32rpx;
}

.back-icon {
  font-size: 64rpx;
  color: var(--text-primary);
  line-height: 1;
}

.header-title {
  font-size: 40rpx;
  font-weight: 300;
  color: var(--text-primary);
}

.content-area {
  flex: 1;
  padding: 0 32rpx 32rpx;
  overflow-y: auto;
  z-index: 10;
  box-sizing: border-box;
}

/* 步骤1特殊布局 */
.step-1-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.mode-selector-fixed {
  flex-shrink: 0;
  padding: 32rpx 48rpx 24rpx;
}

.step-1-scroll {
  flex: 1;
  padding-top: 24rpx;
  padding-right: 32rpx;
}

.step-content {
  display: flex;
  flex-direction: column;
  gap: 48rpx;
}

/* 步骤切换动画 */
.step-enter {
  animation: slideInRight var(--transition-base) ease-out;
}

.step-enter-reverse {
  animation: slideInLeft var(--transition-base) ease-out;
}

.step-exit {
  animation: slideOutLeft var(--transition-base) ease-in;
}

.step-exit-reverse {
  animation: slideOutRight var(--transition-base) ease-in;
}

@keyframes slideInRight {
  from {
    opacity: 0;
    transform: translateX(60rpx);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@keyframes slideInLeft {
  from {
    opacity: 0;
    transform: translateX(-60rpx);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@keyframes slideOutLeft {
  from {
    opacity: 1;
    transform: translateX(0);
  }
  to {
    opacity: 0;
    transform: translateX(-60rpx);
  }
}

@keyframes slideOutRight {
  from {
    opacity: 1;
    transform: translateX(0);
  }
  to {
    opacity: 0;
    transform: translateX(60rpx);
  }
}

/* 步骤 0: 名称 */
.input-group {
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.input-label {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.name-input-wrapper {
  position: relative;
  padding: 32rpx 0;
  border-bottom: 4rpx solid var(--border-primary);
  transition: var(--transition-base);
  min-height: 120rpx;
  display: flex;
  align-items: center;
}

.name-input-wrapper.focused {
  border-bottom-color: #00f3ff;
  box-shadow: 0 4rpx 0 rgba(0, 243, 255, 0.3);
}

.name-input {
  width: 100%;
  font-size: 64rpx;
  color: var(--text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  line-height: 1.5;
  min-height: 96rpx;
}

.name-input::placeholder {
  color: rgba(136, 136, 136, 0.5);
}

.step-hint {
  margin-top: 64rpx;
}

.hint-text {
  font-size: 28rpx;
  color: var(--text-secondary);
  line-height: 1.6;
}

/* 步骤 1: 模式和尺寸 */
.mode-selector {
  display: flex;
  background-color: var(--bg-tertiary);
  padding: 6rpx;
  border-radius: 16rpx;
  gap: 0;
  border: 2rpx solid var(--border-primary);
}

.mode-btn {
  flex: 1;
  padding: 20rpx 24rpx;
  border-radius: 12rpx;
  text-align: center;
  background-color: transparent;
  border: none;
  transition: all 0.3s ease;
}

.mode-btn.active {
  background-color: var(--create-tabs-active-bg);
  box-shadow: var(--create-tabs-active-box-shadow);
}

.mode-text {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-secondary);
  transition: all 0.3s ease;
}

.mode-btn.active .mode-text {
  color: #000000;
  font-weight: bold;
}

/* 浅色模式适配 */
.light-theme .mode-btn.active .mode-text {
  color: #ffffff;
}

/* 图片上传滚动容器 */
.image-upload-scroll {
  max-height: none;
  margin-bottom: 32rpx;
}

/* 图片上传 */
.image-upload {
  min-height: 400rpx;
}

.upload-area {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 400rpx;
  background-color: var(--bg-tertiary);
  border: 4rpx dashed var(--border-color);
  border-radius: 24rpx;
  padding: 48rpx;
}

.upload-icon-wrapper {
  width: 128rpx;
  height: 128rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--bg-tertiary);
  border-radius: 50%;
  margin-bottom: 32rpx;
}

.upload-icon {
  font-size: 64rpx;
}

.upload-text {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
  margin-bottom: 16rpx;
}

.upload-hint {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.preview-area {
  position: relative;
  border-radius: 24rpx;
  overflow: hidden;
  background-color: var(--bg-tertiary);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 480rpx;
}

.preview-image {
  width: 100%;
  max-height: 480rpx;
  object-fit: contain;
}

.preview-actions {
  display: flex;
  gap: 16rpx;
  padding: 24rpx;
}

.preview-btn {
  flex: 1;
  padding: 24rpx;
  background-color: var(--bg-elevated);
  border-radius: 12rpx;
  text-align: center;
}

.preview-btn.danger {
  flex: 0 0 auto;
  width: 96rpx;
  background-color: rgba(255, 51, 51, 0.1);
}

.preview-btn-text {
  font-size: 28rpx;
  color: var(--text-primary);
}

.preview-btn.danger .preview-btn-text {
  color: var(--error-color);
}

/* 自定义尺寸输入 */
.custom-size-inputs {
  display: flex;
  align-items: center;
  gap: 32rpx;
  margin-bottom: 32rpx;
}

.size-input-group {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 16rpx;
}

.size-input-label {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-input {
  background-color: var(--bg-elevated);
  border: 3rpx solid var(--border-primary);
  border-radius: 16rpx;
  font-size: 56rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
  transition: var(--transition-base);
  min-height: 120rpx;
}

.size-input:focus {
  border-color: var(--accent-primary);
  background-color: var(--bg-tertiary);
  box-shadow: 0 0 0 4rpx rgba(0, 243, 255, 0.1);
}

.size-input::placeholder {
  color: var(--text-tertiary);
  opacity: 0.5;
}

.size-separator {
  font-size: 64rpx;
  color: var(--text-secondary);
  margin-top: 48rpx;
  font-weight: bold;
}

.size-preview {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16rpx 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
  margin-bottom: 16rpx;
}

.size-preview-label {
  font-size: 24rpx;
  color: var(--text-secondary);
}

.size-preview-value {
  font-size: 28rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-primary);
}

/* 内联尺寸预览（在标题右侧） */
.size-preview-inline {
  display: flex;
  align-items: center;
}

.size-preview-inline .size-preview-value {
  font-size: 24rpx;
}

/* 尺寸选择 */
.size-section {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.size-header {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.size-icon {
  font-size: 32rpx;
}

.size-title {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-grid {
  display: grid;
  grid-template-columns: repeat(1, 1fr);
  gap: 64rpx;
}

.size-column {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.size-column.full-width {
  width: 100%;
}

.size-column-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.size-label {
  font-size: 24rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--accent-color);
}

.size-hint {
  font-size: 20rpx;
  color: var(--text-tertiary);
}

.size-options {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16rpx;
  justify-items: stretch;
}

.size-option {
  padding: 20rpx 16rpx;
  border-radius: 12rpx;
  background-color: var(--bg-tertiary);
  border: 3rpx solid var(--border-primary);
  transition: all 0.2s;
  text-align: center;
}

.size-option.active {
  border-color: var(--create-size-active-border-color);
  background-color: var(--create-size-active-bg);
  box-shadow: var(--create-size-active-box-shadow);
}

.size-option-text {
  font-size: 28rpx;
  font-family: monospace;
  color: var(--text-secondary);
  font-weight: 500;
}

.size-option.active .size-option-text {
  color: #00f3ff;
  font-weight: bold;
}

.light-theme .size-option.active .size-option-text {
  color: #374151;
}

.size-info {
  font-size: 24rpx;
  color: var(--text-tertiary);
  text-align: center;
  margin-top: 8rpx;
  width: 100%;
  display: block;
}

/* 统计卡片 */
.stats-card {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-color);
  border-radius: 24rpx;
  padding: 32rpx;
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.stat-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stat-label {
  font-size: 28rpx;
  color: var(--text-secondary);
}

.stat-value {
  font-size: 32rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--text-primary);
}

/* 步骤 2: 颜色 */
.colors-header {
  display: flex;
  align-items: center;
  gap: 16rpx;
}

.colors-icon {
  font-size: 32rpx;
}

.colors-title {
  font-size: 28rpx;
  color: var(--text-secondary);
  font-weight: 500;
}

.preset-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 24rpx;
}

.preset-card {
  padding: 48rpx 32rpx;
  border-radius: 24rpx;
  background-color: var(--bg-tertiary);
  border: 4rpx solid var(--border-color);
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8rpx;
  transition: all 0.3s;
}

.preset-card.active {
  background-color: var(--create-preset-card-bg);
  border-color: var(--create-preset-card-border-color);
  color:var(--create-preset-name);
}

.preset-card.active .preset-name{
  color: var(--create-preset-name);
}

.preset-card.active .preset-count{
  color: var(--create-preset-count);
}

.preset-name {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.preset-count {
  font-size: 24rpx;
  color: var(--text-secondary);
  opacity: 0.7;
}

.selected-info {
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--border-color);
  border-radius: 24rpx;
  padding: 48rpx;
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.info-label {
  font-size: 28rpx;
  color: var(--text-secondary);
}

.info-value {
  font-size: 36rpx;
  font-family: monospace;
  font-weight: bold;
  color: var(--create-info-value-clr);
}

.info-hint {
  font-size: 24rpx;
  color: var(--text-tertiary);
  line-height: 1.6;
}

/* 步骤 3: 预览 */
.step-preview {
  display: flex;
  flex-direction: column;
  gap: 32rpx;
}

.preview-header {
  display: flex;
  flex-direction: column;
  gap: 8rpx;
  text-align: center;
}

.preview-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.preview-subtitle {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-canvas-wrapper {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 24rpx;
  border: 2rpx solid var(--border-primary);
  position: relative;
  transform: translateZ(0);
}

.preview-canvas {
  border-radius: 12rpx;
  box-shadow: var(--shadow-md);
  display: block;
  background-color: var(--bg-primary);
  width: 260px;
  height: 260px;
  position: relative;
  transform: translateZ(0);
}

.preview-image {
  border-radius: 12rpx;
  box-shadow: var(--shadow-md);
  display: block;
  width: 260px;
  height: 260px;
}

.preview-stats {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16rpx;
}

.preview-stat-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 24rpx;
  background-color: var(--bg-tertiary);
  border-radius: 16rpx;
  border: 2rpx solid var(--border-primary);
}

.preview-stat-label {
  font-size: 22rpx;
  color: var(--text-secondary);
}

.preview-stat-value {
  font-size: 32rpx;
  font-weight: bold;
  font-family: monospace;
  color: var(--accent-primary);
}

.used-colors-section {
  display: flex;
  flex-direction: column;
  gap: 24rpx;
}

.used-colors-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.used-colors-title {
  font-size: 28rpx;
  font-weight: 500;
  color: var(--text-primary);
}

.used-colors-total {
  font-size: 24rpx;
  font-family: monospace;
  color: var(--text-secondary);
}

.used-colors-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16rpx;
}

.used-color-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8rpx;
  padding: 16rpx;
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
  border: 2rpx solid var(--border-primary);
}

.color-swatch {
  width: 48rpx;
  height: 48rpx;
  border-radius: 8rpx;
  border: 2rpx solid var(--border-primary);
}

.color-code {
  font-size: 20rpx;
  font-family: monospace;
  color: var(--text-primary);
  font-weight: bold;
}

.color-count {
  font-size: 18rpx;
  color: var(--text-secondary);
}

/* 底部按钮 */
.footer {
  position: relative;
  padding: 24rpx;
  padding-bottom: 24rpx;
  z-index: 20;
  box-sizing: border-box;
}

/* 小程序底部安全区域 */
/* #ifdef MP-WEIXIN */
.footer {
  padding-bottom: 48rpx;
}
/* #endif */

.next-btn {
  padding: 32rpx;
  background: linear-gradient(135deg, var(--text-primary) 0%, var(--text-secondary) 100%);
  color: var(--text-inverse);
  border-radius: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16rpx;
  box-shadow: var(--shadow-md);
  transition: var(--transition-base);
}

.next-btn:active {
  transform: scale(0.98);
}

.next-btn.disabled {
  background: linear-gradient(135deg, var(--text-primary) 0%, var(--text-secondary) 100%);
  box-shadow: none;
  opacity: 1;
  pointer-events: none;
}

.next-btn-text {
  font-size: 32rpx;
  font-weight: bold;
  letter-spacing: 0.2em;
  text-transform: uppercase;
}

.next-icon {
  font-size: 48rpx;
  line-height: 1;
}
</style>

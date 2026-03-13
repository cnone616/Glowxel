<template>
  <view class="editor-page light-theme">
    <!-- 状态栏占位 -->
    <!-- #ifdef MP-WEIXIN -->
    <view class="status-bar" :style="{ height: statusBarHeight + 'px' }"></view>
    <!-- #endif -->
    
    <!-- 隐藏的导出 Canvas -->
    <canvas 
      canvas-id="exportCanvas" 
      id="exportCanvas" 
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 1px; height: 1px;"
    ></canvas>
    
    <!-- 隐藏的缩略图 Canvas -->
    <canvas 
      canvas-id="thumbnailCanvas" 
      id="thumbnailCanvas" 
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 100px; height: 100px;"
    ></canvas>
    
    <!-- 隐藏的看板缩略图 Canvas -->
    <canvas 
      canvas-id="boardThumbnailCanvas" 
      id="boardThumbnailCanvas" 
      type="2d"
      style="position: fixed; left: -9999px; top: -9999px; width: 200px; height: 200px;"
    ></canvas>

    <!-- 顶部栏 -->
    <view class="header">
      <view class="header-left">
        <view class="back-btn" @click="handleBack">
          <Icon name="arrow-left-bold" :size="40" />
        </view>
        <view class="header-info">
          <view class="project-name-wrapper" @click="showRenameModal = true; newProjectName = project?.name || ''">
            <text class="project-name">{{ project?.name }} <text v-if="boardId" class="board-id">/ {{ boardId }}</text></text>
            <Icon name="edit" :size="24" class="edit-icon" />
          </view>
          <text class="project-size">{{ boardOffset.w }}x{{ boardOffset.h }}<text v-if="boardId" class="board-mode"> (Board Mode)</text></text>
        </view>
        
        <!-- 操作按钮移到左侧 -->
        <view class="header-actions">
          <view class="icon-btn" @click="handleSave">
            <Icon name="save" :size="36" />
          </view>
          <view class="icon-btn" @click="handleExport">
        
        
            <Icon name="download" :size="36" />
          </view>
          <view class="icon-btn" @click="handlePublish">
            <Icon name="upload" :size="36" />
          </view>
          <view class="icon-btn" @click="isHelpOpen = true">
            <Icon name="help" :size="36" />
          </view>
        </view>
      </view>
    </view>

    <!-- Canvas 画布区域 -->
    <view class="canvas-container" ref="containerRef" :style="{ visibility: hasModalOpen ? 'hidden' : 'visible' }">
      <PixelCanvas
        v-if="canvasReady && !hasModalOpen"
        :width="boardOffset.w"
        :height="boardOffset.h"
        :pixels="isCalculated ? displayPixels : new Map()"
        :zoom="zoom"
        :offset-x="pan.x"
        :offset-y="pan.y"
        :canvas-width="containerSize.width"
        :canvas-height="containerSize.height"
        :grid-visible="gridVisible"
        :allow-single-touch-pan="tool === 'move'"
        :is-dark-mode="false"
        canvas-id="editorCanvas"
        @pixel-click="handlePixelClick"
        @pan="handlePan"
        @zoom="handlePinchZoom"
      />
    </view>

    <!-- 底部工具栏 -->
    <view class="toolbar">
      <!-- 操作按钮 -->
      <view class="action-bar">
        <view class="action-group">
          <view 
            class="action-btn" 
            :class="{ 'disabled': historyIndex <= 0 }"
            @click="handleUndo"
          >
            <Icon name="back" :size="36" />
          </view>
          <view class="divider"></view>
          <view 
            class="action-btn"
            :class="{ 'disabled': historyIndex >= history.length - 1 }"
            @click="handleRedo"
          >
            <Icon name="forward" :size="36" />
          </view>
        </view>

        <view class="action-group">
          <view class="action-btn" @click="handleZoom(-1)">
            <Icon name="zoom-out" :size="36" />
          </view>
          <view class="divider"></view>
          <view class="action-btn" @click="handleZoom(1)">
            <Icon name="zoom-in" :size="36" />
          </view>
          <view class="divider"></view>
          <view 
            class="action-btn device-btn"
            :class="{ 'connected': deviceConnected }"
            @click="toggleDeviceSync"
          >
            <Icon :name="deviceConnected ? 'link' : 'unlink'" :size="36" />
          </view>
          <view class="divider"></view>
          <view 
            class="action-btn"
            :class="{ 'active': gridVisible }"
            @click="gridVisible = !gridVisible"
          >
            <Icon name="3column" :size="36" />
          </view>
          <view class="divider"></view>
          <view class="action-btn" @click="handleFit">
            <Icon name="fullscreen-expand" :size="36" />
          </view>
        </view>
      </view>

      <!-- 工具选择 -->
      <view class="tools">
        <view 
          class="tool-btn move"
          :class="{ 'active': tool === 'move' }"
          @click="setTool('move')"
        >
          <Icon name="move" :size="40" />
          <text class="tool-label">拖动</text>
        </view>

        <view 
          class="tool-btn"
          :class="{ 'active': tool === 'pencil' }"
          @click="setTool('pencil')"
        >
          <Icon name="edit" :size="40" />
          <text class="tool-label">绘画</text>
        </view>

        <view 
          class="tool-btn eraser"
          :class="{ 'active': tool === 'eraser' }"
          @click="setTool('eraser')"
        >
          <Icon name="delete" :size="40" />
          <text class="tool-label">擦除</text>
        </view>
      </view>

      <!-- 颜色选择器 -->
      <view class="color-palette">
        <ColorPalette 
          :colors="paletteColors" 
          :selected-color="selectedColor" 
          @select-color="setSelectedColor"
        />
      </view>
    </view>

    <!-- 保存确认弹窗 -->
    <view v-if="showSaveConfirm" class="modal-overlay" @click="showSaveConfirm = false">
      <view class="modal" @click.stop>
        <view class="modal-icon">
          <Icon name="save" :size="56" />
        </view>
        <text class="modal-title">保存修改？</text>
        <text class="modal-desc">是否保存本次修改？</text>
        <view class="modal-actions">
          <view class="modal-btn secondary" @click="showSaveConfirm = false">
            <text class="modal-btn-text">取消</text>
          </view>
          <view class="modal-btn primary" @click="confirmSave">
            <text class="modal-btn-text">确认保存</text>
          </view>
        </view>
      </view>
    </view>

    <!-- 离开确认弹窗 -->
    <view v-if="showLeaveConfirm" class="modal-overlay" @click="showLeaveConfirm = false">
      <view class="modal" @click.stop>
        <view class="modal-icon warning">
          <Icon name="close" :size="56" />
        </view>
        <text class="modal-title">有未保存的修改</text>
        <text class="modal-desc">您有未保存的修改，是否保存后再离开？</text>
        <view class="modal-actions vertical">
          <view class="modal-btn primary" @click="saveAndLeave">
            <text class="modal-btn-text">保存并离开</text>
          </view>
          <view class="modal-btn secondary" @click="confirmLeave">
            <text class="modal-btn-text">不保存，直接离开</text>
          </view>
          <view class="modal-btn text" @click="showLeaveConfirm = false">
            <text class="modal-btn-text">取消</text>
          </view>
        </view>
      </view>
    </view>
    
    <!-- 导出确认弹窗 -->
    <view v-if="showExportConfirm" class="modal-overlay" @click="showExportConfirm = false">
      <view class="modal" @click.stop>
        <view class="modal-icon warning">
          <Icon name="download" :size="56" />
        </view>
        <text class="modal-title">有未保存的修改</text>
        <text class="modal-desc">检测到您有未保存的修改，是否先保存再导出？</text>
        <view class="modal-actions vertical">
          <view class="modal-btn primary" @click="saveAndExport">
            <text class="modal-btn-text">保存并导出</text>
          </view>
          <view class="modal-btn secondary" @click="exportWithoutSave">
            <text class="modal-btn-text">直接导出</text>
          </view>
          <view class="modal-btn text" @click="showExportConfirm = false">
            <text class="modal-btn-text">取消</text>
          </view>
        </view>
      </view>
    </view>
    
    <!-- 保存到相册确认弹窗 -->
    <view v-if="showSaveToAlbumConfirm" class="modal-overlay" @click="cancelSaveToAlbum">
      <view class="modal" @click.stop>
        <view class="modal-icon">
          <Icon name="download" :size="56" />
        </view>
        <text class="modal-title">图纸已生成</text>
        <text class="modal-desc">是否保存到相册？</text>
        <view class="modal-actions">
          <view class="modal-btn secondary" @click="cancelSaveToAlbum">
            <text class="modal-btn-text">取消</text>
          </view>
          <view class="modal-btn primary" @click="saveToAlbum">
            <text class="modal-btn-text">保存到相册</text>
          </view>
        </view>
      </view>
    </view>
    
    <!-- 重命名弹窗 -->
    <view v-if="showRenameModal" class="modal-overlay" @click="showRenameModal = false">
      <view class="rename-modal" @click.stop>
        <view class="modal-header">
          <text class="modal-title">重命名画布</text>
          <view class="modal-close" @click="showRenameModal = false">
            <Icon name="close" :size="32" />
          </view>
        </view>
        
        <view class="modal-body">
          <view 
            class="name-input-wrapper"
            :class="{ 'focused': showRenameModal }"
          >
            <input 
              v-model="newProjectName"
              type="text"
              class="name-input"
              placeholder="输入新名称"
              cursor-color="#00f3ff"
              :focus="showRenameModal"
              confirm-type="done"
              @confirm="handleRename"
            />
          </view>
        </view>
        
        <view class="modal-actions">
          <view class="modal-btn cancel" @click="showRenameModal = false">
            <text class="modal-btn-text">取消</text>
          </view>
          <view class="modal-btn confirm" @click="handleRename">
            <text class="modal-btn-text">确认</text>
          </view>
        </view>
      </view>
    </view>
    
    <!-- 帮助弹窗 -->
    <HelpModal 
      :is-open="isHelpOpen"
      @close="isHelpOpen = false"
      title="编辑器操作指南"
      :items="helpItems"
    />
    
    <!-- 自定义 Toast 组件 -->
    <Toast ref="toastRef" @show="handleToastShow" @hide="handleToastHide" />
  </view>
</template>


<script>
import { useProjectStore } from '../../store/project.js'
import { useToast } from '../../composables/useToast.js'
import { getColorByCode } from '../../data/artkal-colors-full.js'
import statusBarMixin from '../../mixins/statusBar.js'
import PixelCanvas from '../../components/PixelCanvas.vue'
import ColorPalette from '../../components/ColorPalette.vue'
import HelpModal from '../../components/HelpModal.vue'
import Toast from '../../components/Toast.vue'
import Icon from "../../components/Icon.vue"
import PublishButton from '../../components/PublishButton.vue'
import { exportCanvasAsImage, saveImageToAlbum } from '../../utils/exportCanvas.js'
















export default {
  mixins: [statusBarMixin],
  components: {
    PixelCanvas,
    ColorPalette,
    HelpModal,
    Toast,
    Icon,
    PublishButton
  },
  
  data() {
    return {
      projectStore: null,
      toast: null,
      
      showRenameModal: false,
      newProjectName: '',
      projectId: '',
      boardId: '',
      project: null,
      
      // 像素数据
      pixels: new Map(),
      paletteColors: [],
      selectedColor: '',
      selectedColorCode: '',
      
      // 视图控制 - 初始值设为合理的默认值（52格子在400px容器中）
      zoom: 3,  // 初始缩放级别
      pan: { x: 50, y: 50 },  // 初始偏移，等待onReady重新计算
      tool: 'move',
      gridVisible: true,
      
      // 历史记录
      history: [],
      historyIndex: -1,
      
      // 容器尺寸
      containerSize: { width: 0, height: 0 },
      canvasReady: false,
      isCalculated: false,  // 标记是否已加载像素数据
      
      // 状态
      showSaveConfirm: false,
      showLeaveConfirm: false,
      showExportConfirm: false,
      showSaveToAlbumConfirm: false,
      showConnectModal: false,
      exportedImagePath: '',
      hasUnsavedChanges: false,
      savedPixelsSnapshot: '',
      isHelpOpen: false,
      toastVisible: false,  // Toast显示状态，用于隐藏canvas
      
      // 帮助内容
      helpItems: [
        { iconText: '👆', title: '基础操作', description: '单指在画布上拖动即可绘画。双指捏合可缩放画布，双指拖动可移动视角。' },
        { iconText: '✋', title: '拖动模式', description: '切换到底部的【拖动】工具后，单指即可随意拖动画布位置，避免误触绘画。' },
        { iconText: '🎨', title: '颜色选择', description: '点击底部色板选择当前颜色。' },
        { iconText: '💾', title: '保存与进度', description: '记得经常点击右上角的保存图标。系统也会尝试在退出时提醒保存。' }
      ]
    }
  },
  
  computed: {
    // 是否可编辑（草稿和已退回可编辑，已发布和审核中不可编辑）
    isEditable() {
      if (!this.project) return false
      const status = this.project.status || 'draft'
      return status === 'draft' || status === 'rejected'
    },

    deviceConnected() {
      return this.deviceStore?.connected || false
    },
    
    boardOffset() {
      if (!this.boardId || !this.project) {
        // 全画布模式：使用填充后的尺寸
        const paddedWidth = this.project?.paddedWidth || this.project?.width || 64
        const paddedHeight = this.project?.paddedHeight || this.project?.height || 64
        return { 
          x: 0, 
          y: 0, 
          w: paddedWidth, 
          h: paddedHeight
        }
      }
      // 单板模式：使用64x64
      const row = (this.boardId.charCodeAt(0) || 65) - 65
      const col = parseInt(this.boardId.slice(1) || '1') - 1
      return {
        x: col * 64,
        y: row * 64,
        w: 64,
        h: 64
      }
    },
    
    displayPixels() {
      if (!this.boardId) {
        return this.pixels
      }
      
      const local = new Map()
      this.pixels.forEach((color, key) => {
        const [px, py] = key.split(',').map(Number)
        if (px >= this.boardOffset.x && px < this.boardOffset.x + this.boardOffset.w && 
            py >= this.boardOffset.y && py < this.boardOffset.y + this.boardOffset.h) {
          local.set(`${px - this.boardOffset.x},${py - this.boardOffset.y}`, color)
        }
      })
      return local
    },
    
    // 判断是否有弹窗显示
    hasModalOpen() {
      return this.showSaveConfirm || 
             this.showLeaveConfirm || 
             this.showExportConfirm || 
             this.showSaveToAlbumConfirm || 
             this.showRenameModal || 
             this.isHelpOpen ||
             this.toastVisible
    }
  },
  
  watch: {
    'containerSize.width'(newVal, oldVal) {
      if (newVal > 0 && newVal !== oldVal) {
        this.handleFit()
      }
    },
    'containerSize.height'(newVal, oldVal) {
      if (newVal > 0 && newVal !== oldVal) {
        this.handleFit()
      }
    },
    'boardOffset.w'() {
      if (this.canvasReady) {
        this.handleFit()
      }
    },
    'boardOffset.h'() {
      if (this.canvasReady) {
        this.handleFit()
      }
    },
    pixels: {
      handler(newPixels) {
        const currentSnapshot = JSON.stringify(Array.from(newPixels.entries()))
        this.hasUnsavedChanges = currentSnapshot !== this.savedPixelsSnapshot
      },
      deep: true
    }
  },

  onLoad(options) {
    this.projectStore = useProjectStore()
    this.toast = useToast()
    
    this.projectId = options.id
    this.boardId = options.board || ''
    this.project = this.projectStore.getProject(this.projectId)
    
    if (!this.project) {
      this.toast.showError('项目不存在')
      setTimeout(() => {
        uni.switchTab({
          url: '/pages/workspace/workspace'
        })
      }, 1000)
      return
    }
    
    // 已发布/审核中项目提示只读
    if (this.project.status === 'published' || this.project.status === 'reviewing') {
      uni.showToast({
        title: '已发布作品仅可查看和拼豆',
        icon: 'none',
        duration: 2000
      })
    }

    this.loadPalette()
    this.loadPixels()

    // 首次使用帮助
    const hasSeenHelp = uni.getStorageSync('hasSeenEditorHelp')
    if (!hasSeenHelp) {
      this.isHelpOpen = true
      uni.setStorageSync('hasSeenEditorHelp', 'true')
    }
  },

  onShow() {
    // 隐藏 tabBar
    uni.hideTabBar({
      animation: false
    })
  },
  
  onReady() {
    // 注册自定义 Toast 实例
    if (this.$refs.toastRef) {
      this.toast.setToastInstance(this.$refs.toastRef)
    }
    
    setTimeout(() => {
      const query = uni.createSelectorQuery().in(this)
      query.select('.canvas-container').boundingClientRect(data => {
        if (data && data.width > 0 && data.height > 0) {
          this.containerSize = {
            width: data.width,
            height: data.height
          }
          
          const fitZoomW = data.width / this.boardOffset.w
          const fitZoomH = data.height / this.boardOffset.h
          const fitZoom = Math.min(fitZoomW, fitZoomH, 3) * 0.9  // 限制初始最大缩放为3，留10%边距
          
          const boardPixelWidth = this.boardOffset.w * fitZoom
          const boardPixelHeight = this.boardOffset.h * fitZoom
          
          this.zoom = fitZoom
          this.pan = {
            x: (data.width - boardPixelWidth) / 2,
            y: (data.height - boardPixelHeight) / 2
          }
          
          console.log('初始化:', { 
            containerWidth: data.width, 
            containerHeight: data.height,
            boardPixelWidth, 
            boardPixelHeight, 
            zoom: fitZoom,
            panX: this.pan.x,
            panY: this.pan.y
          })
          
          // 确保 pan 和 zoom 设置完成后再显示画布
          this.$nextTick(() => {
            this.canvasReady = true
            
            setTimeout(() => {
              this.isCalculated = true
            }, 10)
          })
        }
      }).exec()
    }, 100)
  },

  methods: {
    // ========== 设备同步方法 ==========
    toggleDeviceSync() {
      if (this.deviceConnected) {
        // 断开连接
        this.deviceStore.disconnect()
        this.toast.showInfo('设备已断开')
      } else {
        // 显示连接弹窗
        this.showConnectModal = true
      }
    },
    
    async handleConnectConfirm(ip) {
      try {
        const result = await this.deviceStore.connect(ip)
        if (result.success) {
          // 连接成功后立即同步当前画布
          setTimeout(() => {
            this.syncToDevice()
          }, 500)
          
          this.$refs.connectModal.onSuccess()
          this.toast.showSuccess('设备已连接')
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
    
    // 同步整个画布到设备
    async syncToDevice() {
      if (!this.deviceConnected) return
      
      try {
        const width = this.boardOffset.w
        const height = this.boardOffset.h
        const pixels = []
        
        // 转换画布数据为 RGB 数组
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            const key = `${x},${y}`
            const colorCode = this.pixels.get(key)
            
            if (colorCode) {
              const color = getColorByCode(colorCode)
              if (color) {
                pixels.push(color.r, color.g, color.b)
              } else {
                pixels.push(0, 0, 0)
              }
            } else {
              pixels.push(0, 0, 0)
            }
          }
        }
        
        await this.deviceStore.sendImage(pixels, width, height)
      } catch (err) {
        console.error('同步到设备失败:', err)
      }
    },
    
    // ========== 原有方法 ==========
    setTool(newTool) {
      if (this.tool !== newTool) {
        this.tool = newTool
      }
    },
    
    loadPalette() {
      if (this.project.palette && this.project.palette.length > 0) {
        this.paletteColors = this.project.palette
          .map(code => getColorByCode(code))
          .filter(color => color !== undefined)
      }
    },

    loadPixels() {
      const pixels = this.projectStore.getProjectPixels(this.projectId)
      this.pixels = pixels
      this.history = [new Map(pixels)]
      this.historyIndex = 0
      this.savedPixelsSnapshot = JSON.stringify(Array.from(pixels.entries()))
      this.hasUnsavedChanges = false
    },

    handlePixelClick(x, y) {
      if (!this.isEditable) {
        uni.showToast({ title: '已发布作品不可编辑', icon: 'none' })
        return
      }
      if (this.tool === 'move') return

      const globalX = x + this.boardOffset.x
      const globalY = y + this.boardOffset.y
      const key = `${globalX},${globalY}`
      
      const newPixels = new Map(this.pixels)
      
      if (this.tool === 'eraser') {
        if (!newPixels.has(key)) return
        newPixels.delete(key)
      } else if (this.tool === 'pencil') {
        if (newPixels.get(key) === this.selectedColor) return
        newPixels.set(key, this.selectedColor)
      }
      
      this.pixels = newPixels
      
      const newHistory = this.history.slice(0, this.historyIndex + 1)
      newHistory.push(new Map(newPixels))
      if (newHistory.length > 50) newHistory.shift()
      
      this.history = newHistory
      this.historyIndex = newHistory.length - 1
      
      // 同步到设备
      if (this.deviceConnected) {
        this.syncToDevice()
      }
    },

    handleUndo() {
      if (!this.isEditable) return
      if (this.historyIndex > 0) {
        this.historyIndex--
        this.pixels = new Map(this.history[this.historyIndex])
        
        // 同步到设备
        if (this.deviceConnected) {
          this.syncToDevice()
        }
      }
    },

    handleRedo() {
      if (!this.isEditable) return
      if (this.historyIndex < this.history.length - 1) {
        this.historyIndex++
        this.pixels = new Map(this.history[this.historyIndex])
        
        // 同步到设备
        if (this.deviceConnected) {
          this.syncToDevice()
        }
      }
    },

    handleZoom(delta) {
      const centerX = this.containerSize.width / 2
      const centerY = this.containerSize.height / 2
      
      const newZoom = Math.max(1, Math.min(50, this.zoom + delta))
      const scaleFactor = newZoom / this.zoom
      
      this.pan.x = centerX - (centerX - this.pan.x) * scaleFactor
      this.pan.y = centerY - (centerY - this.pan.y) * scaleFactor
      this.zoom = newZoom
    },

    handlePinchZoom(delta, centerX, centerY) {
      const newZoom = Math.max(1, Math.min(50, this.zoom + delta))
      const scaleFactor = newZoom / this.zoom

      this.pan = {
        x: centerX - (centerX - this.pan.x) * scaleFactor,
        y: centerY - (centerY - this.pan.y) * scaleFactor
      }
      this.zoom = newZoom
    },

    handlePan(dx, dy) {
      this.pan = { x: this.pan.x + dx, y: this.pan.y + dy }
    },

    handleFit() {
      if (this.containerSize.width > 0 && this.containerSize.height > 0 && this.boardOffset.w > 0) {
        const fitZoomW = this.containerSize.width / this.boardOffset.w
        const fitZoomH = this.containerSize.height / this.boardOffset.h
        const fitZoom = Math.min(fitZoomW, fitZoomH, 4.5) * 1.35  // 增加1.5倍缩放（0.9 * 1.5 = 1.35），最大缩放提升到4.5
        
        this.zoom = fitZoom
        
        const boardPixelWidth = this.boardOffset.w * fitZoom
        const boardPixelHeight = this.boardOffset.h * fitZoom
        
        this.pan = {
          x: (this.containerSize.width - boardPixelWidth) / 2,
          y: (this.containerSize.height - boardPixelHeight) / 2
        }
      }
    },

    setSelectedColor(colorHex) {
      if (!colorHex) {
        this.selectedColor = ''
        this.selectedColorCode = ''
        return
      }
      
      this.selectedColor = colorHex
      const colorObj = this.paletteColors.find(c => c.hex === colorHex)
      if (colorObj) {
        this.selectedColorCode = colorObj.code
      }
    },

    handleSave() {
      if (!this.isEditable) {
        uni.showToast({ title: '已发布作品不可编辑', icon: 'none' })
        return
      }
      this.showSaveConfirm = true
    },

    handleRename() {
      if (!this.isEditable) {
        uni.showToast({ title: '已发布作品不可重命名', icon: 'none' })
        return
      }
      if (!this.newProjectName.trim()) {
        this.toast.showError('名称不能为空')
        return
      }
      
      this.projectStore.updateProjectName(this.projectId, this.newProjectName.trim())
      
      // 重新获取项目数据
      const updatedProject = this.projectStore.getProject(this.projectId)
      if (updatedProject) {
        this.project = { ...updatedProject }
      }
      
      this.showRenameModal = false
      this.newProjectName = ''
      this.toast.showSuccess('重命名成功')
    },

    async confirmSave() {
      if (!this.isEditable) {
        uni.showToast({ title: '已发布作品不可编辑', icon: 'none' })
        return
      }
      // 保存像素数据
      this.projectStore.saveProjectPixels(this.projectId, this.pixels)
      this.savedPixelsSnapshot = JSON.stringify(Array.from(this.pixels.entries()))
      this.hasUnsavedChanges = false
      
      // 生成并保存缩略图
      await this.generateAndSaveThumbnail()
      
      this.toast.showSuccess('已保存')
      this.showSaveConfirm = false
    },

    handleBack() {
      if (this.hasUnsavedChanges) {
        this.showLeaveConfirm = true
      } else {
        uni.navigateBack()
      }
    },

    confirmLeave() {
      this.showLeaveConfirm = false
      uni.navigateBack()
    },

    async saveAndLeave() {
      if (!this.isEditable) {
        uni.navigateBack()
        return
      }
      // 保存像素数据
      this.projectStore.saveProjectPixels(this.projectId, this.pixels)
      
      // 生成并保存缩略图
      await this.generateAndSaveThumbnail()
      
      this.toast.showSuccess('已保存')
      this.showLeaveConfirm = false
      setTimeout(() => {
        uni.navigateBack()
      }, 500)
    },

    async handleExport() {
      // 检查是否有未保存的修改
      if (this.hasUnsavedChanges) {
        this.showExportConfirm = true
      } else {
        // 没有未保存的修改，直接导出
        await this.performExport()
      }
    },
    
    async saveAndExport() {
      this.showExportConfirm = false
      await this.confirmSave()
      await this.performExport()
    },
    
    async exportWithoutSave() {
      this.showExportConfirm = false
      await this.performExport()
    },
    
    async performExport() {
      this.toast.showLoading('正在生成图纸...')
      
      try {
        const tempFilePath = await exportCanvasAsImage({
          pixels: this.displayPixels,
          width: this.boardOffset.w,
          height: this.boardOffset.h,
          projectName: this.project.name + (this.boardId ? ` - ${this.boardId}` : ''),
          palette: this.project.palette,
          cellSize: 40,
          showGrid: true,
          showCoordinates: true,
          showColorCodes: true,
          showStatistics: true,
          showLogo: true,
          canvasId: 'exportCanvas'
        })
        
        this.toast.hideLoading()
        
        // 保存临时文件路径，用于后续保存到相册
        this.exportedImagePath = tempFilePath
        this.showSaveToAlbumConfirm = true
      } catch (error) {
        console.error('导出失败:', error)
        this.toast.hideLoading()
        this.toast.showError('导出失败')
      }
    },
    
    async saveToAlbum() {
      this.showSaveToAlbumConfirm = false
      try {
        await saveImageToAlbum(this.exportedImagePath)
        this.toast.showSuccess('已保存到相册')
      } catch (error) {
        console.error('保存失败:', error)
        this.toast.showError('保存失败')
      }
    },
    
    cancelSaveToAlbum() {
      this.showSaveToAlbumConfirm = false
      this.exportedImagePath = ''
    },
    
    // 生成并保存缩略图
    async generateAndSaveThumbnail() {
      try {
        // 使用隐藏的 canvas 生成缩略图
        const query = uni.createSelectorQuery().in(this)
        query.select('#thumbnailCanvas')
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              console.error('无法获取缩略图canvas')
              return
            }
            
            const canvas = res[0].node
            const ctx = canvas.getContext('2d')
            
            // 缩小到 100x100 节省存储空间
            const thumbnailSize = 100
            canvas.width = thumbnailSize
            canvas.height = thumbnailSize
            
            // 清空背景为透明
            ctx.clearRect(0, 0, thumbnailSize, thumbnailSize)
            
            // 计算缩放比例（保持宽高比，居中显示）
            const scale = Math.min(thumbnailSize / this.project.width, thumbnailSize / this.project.height)
            const scaledWidth = this.project.width * scale
            const scaledHeight = this.project.height * scale
            const offsetX = (thumbnailSize - scaledWidth) / 2
            const offsetY = (thumbnailSize - scaledHeight) / 2
            
            // 绘制像素
            this.pixels.forEach((color, key) => {
              const [x, y] = key.split(',').map(Number)
              ctx.fillStyle = color
              ctx.fillRect(
                offsetX + x * scale,
                offsetY + y * scale,
                Math.ceil(scale) + 1,
                Math.ceil(scale) + 1
              )
            })
            
            // 延迟一下确保绘制完成
            setTimeout(() => {
              // 转换为 Base64（永久保存）
              uni.canvasToTempFilePath({
                canvas: canvas,
                canvasId: 'thumbnailCanvas',
                fileType: 'png',
                quality: 0.8, // 压缩质量
                success: (res) => {
                  // 将临时文件转为 Base64
                  // #ifdef MP-WEIXIN
                  const fs = uni.getFileSystemManager()
                  fs.readFile({
                    filePath: res.tempFilePath,
                    encoding: 'base64',
                    success: (fileRes) => {
                      const base64 = 'data:image/png;base64,' + fileRes.data
                      console.log('缩略图生成成功 (Base64):', Math.round(base64.length / 1024), 'KB')
                      this.projectStore.updateProjectThumbnail(this.projectId, base64)
                    },
                    fail: (err) => {
                      console.error('读取缩略图文件失败:', err)
                      // 降级：保存临时文件路径
                      this.projectStore.updateProjectThumbnail(this.projectId, res.tempFilePath)
                    }
                  })
                  // #endif
                  
                  // #ifdef H5
                  // H5 直接使用 canvas.toDataURL
                  try {
                    const base64 = canvas.toDataURL('image/png', 0.8)
                    console.log('缩略图生成成功 (Base64):', Math.round(base64.length / 1024), 'KB')
                    this.projectStore.updateProjectThumbnail(this.projectId, base64)
                  } catch (err) {
                    console.error('生成 Base64 失败:', err)
                    // 降级：保存临时文件路径
                    this.projectStore.updateProjectThumbnail(this.projectId, res.tempFilePath)
                  }
                  // #endif
                },
                fail: (err) => {
                  console.error('生成缩略图失败:', err)
                }
              }, this)
            }, 100)
          })
        
        // 如果是看板模式，生成看板缩略图（继续用临时文件）
        if (this.boardId) {
          await this.generateBoardThumbnail()
        }
      } catch (error) {
        console.error('生成缩略图失败:', error)
      }
    },
    
    // 生成看板缩略图
    async generateBoardThumbnail() {
      try {
        const query = uni.createSelectorQuery().in(this)
        query.select('#boardThumbnailCanvas')
          .fields({ node: true, size: true })
          .exec((res) => {
            if (!res || !res[0]) {
              return
            }
            
            const canvas = res[0].node
            const ctx = canvas.getContext('2d')
            
            const thumbnailSize = 200
            canvas.width = thumbnailSize
            canvas.height = thumbnailSize
            
            // 清空背景
            ctx.clearRect(0, 0, thumbnailSize, thumbnailSize)
            
            // 看板固定 64x64，计算缩放
            const scale = thumbnailSize / 64
            
            // 绘制当前看板的像素
            this.displayPixels.forEach((color, key) => {
              const [x, y] = key.split(',').map(Number)
              ctx.fillStyle = color
              ctx.fillRect(
                x * scale,
                y * scale,
                Math.ceil(scale),
                Math.ceil(scale)
              )
            })
            
            // 转换为 base64
            uni.canvasToTempFilePath({
              canvas: canvas,
              success: (res) => {
                // 保存看板缩略图
                this.projectStore.saveBoardThumbnail(this.projectId, this.boardId, res.tempFilePath)
              },
              fail: (err) => {
                console.error('生成看板缩略图失败:', err)
              }
            }, this)
          })
      } catch (error) {
        console.error('生成看板缩略图失败:', error)
      }
    },
    
    handleToastShow() {
      this.toastVisible = true
    },
    
    handleToastHide() {
      this.toastVisible = false
    },
    
    // 处理发布按钮点击
    handlePublish() {
      // 检查画布是否为空
      if (this.pixels.size === 0) {
        this.toast.showError('画布为空，无法发布')
        return
      }
      
      // 检查是否有未保存的修改
      if (this.hasUnsavedChanges) {
        uni.showModal({
          title: '有未保存的修改',
          content: '检测到您有未保存的修改，是否先保存再发布？',
          confirmText: '保存并发布',
          cancelText: '直接发布',
          success: (res) => {
            if (res.confirm) {
              // 保存并发布
              this.saveAndPublish()
            } else if (res.cancel) {
              // 直接发布
              this.navigateToPublish()
            }
          }
        })
      } else {
        // 直接发布
        this.navigateToPublish()
      }
    },
    
    // 保存并发布
    async saveAndPublish() {
      if (!this.isEditable) {
        uni.showToast({ title: '已发布作品不可重复发布', icon: 'none' })
        return
      }
      try {
        // 保存像素数据
        this.projectStore.saveProjectPixels(this.projectId, this.pixels)
        this.savedPixelsSnapshot = JSON.stringify(Array.from(this.pixels.entries()))
        this.hasUnsavedChanges = false
        
        // 生成并保存缩略图
        await this.generateAndSaveThumbnail()
        
        this.toast.showSuccess('已保存')
        
        // 跳转到发布页面
        setTimeout(() => {
          this.navigateToPublish()
        }, 500)
      } catch (error) {
        console.error('保存失败:', error)
        this.toast.showError('保存失败，请重试')
      }
    },
    
    // 跳转到发布页面
    navigateToPublish() {
      // 传递当前画布数据到发布页面
      const canvasData = {
        projectId: this.projectId,
        boardId: this.boardId,
        pixels: Array.from(this.displayPixels.entries()),
        width: this.boardOffset.w,
        height: this.boardOffset.h,
        colors: this.paletteColors.map(c => c.code),
        projectName: this.project?.name || '未命名画布'
      }
      
      // 将数据存储到临时存储中
      uni.setStorageSync('temp_publish_data', canvasData)
      
      // 添加页面切换动画
      uni.navigateTo({
        url: '/pages/publish-project/publish-project',
        animationType: 'slide-in-right',
        animationDuration: 300
      })
    }
  }
}
</script>


<style scoped>
.editor-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background-color: var(--bg-primary);
}

/* 顶部栏 - 112rpx = 56px = h-14 */
.header {
  height: 112rpx;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  padding: 0 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  background-color: var(--bg-elevated);
  z-index: 10;
  flex-shrink: 0;
  box-sizing: border-box;
}

/* 小程序需要减小高度避免内容被挤压 */
/* #ifdef MP-WEIXIN */
.header {
  height: auto;
  min-height: 88rpx;
  padding-top: 16rpx;
  padding-bottom: 16rpx;
  padding-right: 200rpx;  /* 为胶囊按钮留出空间 */
}
/* #endif */

.header-left {
  display: flex;
  align-items: center;
  gap: 24rpx;
  flex: 1;
  overflow: hidden;
}

.back-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.back-btn:active {
  color: var(--text-primary);
}

.header-info {
  display: flex;
  flex-direction: column;
  gap: 4rpx;
  flex: 1;
  min-width: 0;
}

.project-name-wrapper {
  display: flex;
  align-items: center;
  gap: 8rpx;
}

.project-name {
  font-size: 26rpx;
  font-weight: 500;
  color: var(--text-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.board-id {
  font-weight: normal;
  color: var(--text-secondary);
}

.project-size {
  font-size: 20rpx;
  color: var(--text-secondary);
  font-family: monospace;
}

.board-mode {
  font-size: 18rpx;
}

.header-actions {
  display: flex;
  gap: 16rpx;
  flex-shrink: 0;
}

.icon-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
  transition: var(--transition-base);
}

.icon-btn:active {
  color: var(--accent-primary);
}

.device-btn.connected {
  color: var(--accent-primary);
  animation: pulse-glow 2s ease-in-out infinite;
}

@keyframes pulse-glow {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.6; }
}

/* Canvas 区域 */
.canvas-container {
  flex: 1;
  position: relative;
  overflow: hidden;
  background-color: var(--bg-primary);
  touch-action: none;
}

/* 底部工具栏 */
.toolbar {
  background-color: var(--bg-elevated);
  border-top: 2rpx solid var(--border-primary);
  padding-bottom: env(safe-area-inset-bottom);
  z-index: 20;
}

/* 操作栏 */
.action-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
  background-color: var(--bg-tertiary);
}

.action-group {
  display: flex;
  align-items: center;
  gap: 4rpx;
  padding: 4rpx;
  background-color: var(--bg-tertiary);
  border-radius: 12rpx;
}

.action-btn {
  padding: 12rpx;
  border-radius: 8rpx;
  color: var(--text-primary);
  transition: var(--transition-base);
}

.action-btn:active {
  background-color: var(--bg-elevated);
}

.action-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid var(--accent-primary);
  color: var(--accent-primary);
}

.action-btn.row-mode-btn.active {
  animation: pulse-glow 2s ease-in-out infinite;
}

.action-btn.disabled {
  opacity: 0.3;
  color: var(--text-tertiary);
}

.divider {
  width: 2rpx;
  height: 32rpx;
  background-color: var(--border-primary);
  margin: 0 4rpx;
}

/* 工具选择 */
.tools {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 16rpx;
  padding: 16rpx 32rpx;
}

.row-controls {
  display: flex;
  align-items: center;
  gap: 16rpx;
  padding: 12rpx 24rpx;
  background-color: var(--bg-secondary);
  border-radius: 16rpx;
  border: 2rpx solid var(--accent-primary);
}

.row-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 56rpx;
  height: 56rpx;
  border-radius: 12rpx;
  background-color: var(--bg-tertiary);
  color: var(--accent-primary);
  transition: var(--transition-base);
}

.row-btn:active {
  transform: scale(0.95);
  background-color: rgba(0, 243, 255, 0.2);
}

.row-text {
  font-size: 24rpx;
  font-weight: bold;
  color: var(--accent-primary);
  min-width: 120rpx;
  text-align: center;
}

.tool-btn {
  flex: 1;
  max-width: 200rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4rpx;
  padding: 16rpx 12rpx;
  border-radius: 16rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid transparent;
  transition: var(--transition-base);
}

.tool-btn:active {
  transform: scale(0.98);
}

.tool-btn.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
  color: var(--accent-primary);
}

.tool-btn.eraser.active {
  background-color: rgba(255, 51, 51, 0.1);
  border-color: var(--error-color);
  color: var(--error-color);
}

.tool-btn.move.active {
  background-color: rgba(0, 243, 255, 0.1);
  border-color: var(--accent-primary);
  color: var(--accent-primary);
}

.tool-label {
  font-size: 20rpx;
  font-weight: bold;
  color: currentColor;
  text-transform: uppercase;
  letter-spacing: 2rpx;
}

/* 颜色选择器 */
.color-palette {
  padding: 0 32rpx 16rpx;
}

/* 弹窗样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 99998;
  padding: 32rpx;
}

.modal {
  width: 100%;
  max-width: 600rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid rgba(0, 243, 255, 0.3);
  border-radius: 32rpx;
  padding: 48rpx 32rpx 32rpx;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24rpx;
  box-shadow: 0 0 60rpx rgba(0, 243, 255, 0.15);
}

.modal-icon {
  width: 80rpx;
  height: 80rpx;
  border-radius: 50%;
  background-color: rgba(0, 243, 255, 0.1);
  border: 2rpx solid rgba(0, 243, 255, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--accent-color);
}

.modal-icon.warning {
  background-color: rgba(255, 170, 0, 0.1);
  border-color: rgba(255, 170, 0, 0.2);
  color: #ff976a;
}

.modal-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
  text-align: center;
}

.modal-desc {
  font-size: 24rpx;
  color: var(--text-secondary);
  text-align: center;
  line-height: 1.6;
}

.modal-actions {
  display: flex;
  gap: 16rpx;
  width: 100%;
  margin-top: 8rpx;
}

.modal-actions.vertical {
  flex-direction: column;
  gap: 16rpx;
}

.modal-btn {
  flex: 1;
  padding: 24rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16rpx;
  transition: opacity 0.2s;
}

.modal-btn:active {
  opacity: 0.8;
}

.modal-btn.primary {
  background-color: var(--accent-color);
  box-shadow: 0 8rpx 32rpx rgba(0, 243, 255, 0.2);
}

.modal-btn.secondary {
  background-color: rgba(255, 255, 255, 0.05);
  border: 2rpx solid var(--border-color);
}

.modal-btn.text {
  background-color: transparent;
}

.modal-btn-text {
  font-size: 28rpx;
  font-weight: bold;
  color: inherit;
}

.modal-btn.primary .modal-btn-text {
  color: #000000;
}

.modal-btn.secondary .modal-btn-text,
.modal-btn.text .modal-btn-text {
  color: var(--text-primary);
}

/* 浅色主题适配 */
.light-theme .tool-btn.move.active {
  background-color: rgba(0, 153, 204, 0.1);
  border-color: var(--accent-primary);
  color: var(--accent-primary);
}

/* 重命名弹窗 */
.rename-modal {
  right: 0;
  bottom: 0;
  background-color: rgba(0, 0, 0, 0.8);
  backdrop-filter: blur(20rpx);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 48rpx;
}

.rename-modal {
  width: 100%;
  max-width: 500rpx;
  background-color: var(--bg-tertiary);
  border: 2rpx solid var(--accent-primary);
  border-radius: 24rpx;
  overflow: hidden;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 24rpx 32rpx;
  border-bottom: 2rpx solid var(--border-primary);
}

.modal-title {
  font-size: 32rpx;
  font-weight: bold;
  color: var(--text-primary);
}

.modal-close {
  width: 48rpx;
  height: 48rpx;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--text-secondary);
}

.modal-body {
  padding: 32rpx;
}

.rename-input {
  width: 100%;
  padding: 20rpx 24rpx;
  font-size: 28rpx;
  color: var(--text-primary);
  background-color: var(--bg-primary);
  border: 2rpx solid var(--border-primary);
  border-radius: 12rpx;
  transition: all 0.2s;
  box-sizing: border-box;
}

.rename-input::placeholder {
  color: var(--text-tertiary);
  opacity: 0.6;
}

.name-input-wrapper {
  position: relative;
  padding: 24rpx 0;
  border-bottom: 3rpx solid var(--border-primary);
  transition: var(--transition-base);
  min-height: 80rpx;
  display: flex;
  align-items: center;
  width: 100%;
}

.name-input-wrapper.focused {
  border-bottom-color: #00f3ff;
  box-shadow: 0 3rpx 0 rgba(0, 243, 255, 0.3);
}

.name-input {
  width: 100%;
  font-size: 32rpx;
  color: var(--text-primary);
  background-color: transparent;
  border: none;
  outline: none;
  line-height: 1.2;
}

.name-input::placeholder {
  color: rgba(136, 136, 136, 0.5);
}

.modal-actions {
  display: flex;
  gap: 16rpx;
  padding: 24rpx 32rpx;
  border-top: 2rpx solid var(--border-primary);
}

.modal-btn {
  flex: 1;
  padding: 24rpx;
  border-radius: 12rpx;
  text-align: center;
  transition: all 0.2s;
}

.modal-btn.cancel {
  background-color: var(--bg-primary);
  border: 2rpx solid var(--border-primary);
}

.modal-btn.confirm {
  background-color: #00f3ff;
}

.modal-btn-text {
  font-size: 28rpx;
  font-weight: bold;
}

.modal-btn.cancel .modal-btn-text {
  color: var(--text-secondary);
}

.modal-btn.confirm .modal-btn-text {
  color: #000000;
}

.project-name-wrapper {
  display: flex;
  align-items: center;
  gap: 12rpx;
}

.edit-icon {
  color: var(--text-tertiary);
  opacity: 0.6;
}
</style>

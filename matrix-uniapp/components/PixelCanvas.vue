<template>
  <canvas 
    :canvas-id="canvasId"
    :id="canvasId"
    type="2d"
    class="pixel-canvas"
    @touchstart="handleTouchStart"
    @touchmove.stop.prevent="handleTouchMove"
    @touchend="handleTouchEnd"
    @touchcancel="handleTouchEnd"
  ></canvas>
</template>

<script>
export default {
  props: {
    width: {
      type: Number,
      required: true
    },
    height: {
      type: Number,
      required: true
    },
    pixels: {
      type: Map,
      required: true
    },
    zoom: {
      type: Number,
      default: 10
    },
    offsetX: {
      type: Number,
      default: 0
    },
    offsetY: {
      type: Number,
      default: 0
    },
    canvasWidth: {
      type: Number,
      default: 0
    },
    canvasHeight: {
      type: Number,
      default: 0
    },
    gridVisible: {
      type: Boolean,
      default: true
    },
    highlightColor: {
      type: String,
      default: null
    },
    highlightRow: {
      type: Number,
      default: null
    },
    allowSingleTouchPan: {
      type: Boolean,
      default: false
    },
    isDarkMode: {
      type: Boolean,
      default: false
    },
    canvasId: {
      type: String,
      default: 'pixelCanvas'
    }
  },
  
  data() {
    return {
      canvas: null,
      ctx: null,
      canvasRect: null,  // 缓存 canvas 位置
      activePointers: new Map(),
      lastCenter: null,
      lastDist: null,
      lastDrawPos: null
    }
  },
  
  watch: {
    width() { this.drawCanvas() },
    height() { this.drawCanvas() },
    pixels: {
      handler() { this.drawCanvas() },
      deep: true
    },
    zoom(newVal, oldVal) { 
      this.drawCanvas()
    },
    offsetX(newVal, oldVal) { 
      this.drawCanvas()
    },
    offsetY(newVal, oldVal) { 
      this.drawCanvas()
    },
    gridVisible() { this.drawCanvas() },
    highlightColor() { this.drawCanvas() },
    highlightRow() { this.drawCanvas() },
    isDarkMode() { this.drawCanvas() },
    canvasWidth() { this.updateCanvasSize() },
    canvasHeight() { this.updateCanvasSize() }
  },
  
  mounted() {
    this.initCanvas()
  },
  
  methods: {
    initCanvas() {
      const query = uni.createSelectorQuery().in(this)
      query.select(`#${this.canvasId}`)
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0]) return
          
          const canvas = res[0].node
          this.canvas = canvas
          this.ctx = canvas.getContext('2d')
          
          // 获取设备像素比
          const dpr = uni.getSystemInfoSync().pixelRatio || 1
          
          // 设置 canvas 尺寸
          const width = this.canvasWidth || 375
          const height = this.canvasHeight || 468
          
          // 设置实际渲染尺寸（物理像素）
          canvas.width = width * dpr
          canvas.height = height * dpr
          
          // 缩放上下文以匹配 dpr
          this.ctx.scale(dpr, dpr)
          
          this.updateCanvasRect()
          
          // Canvas 初始化完成后立即绘制
          this.drawCanvas()
        })
    },
    
    updateCanvasSize() {
      if (!this.canvas || !this.ctx) return
      
      const width = this.canvasWidth || 375
      const height = this.canvasHeight || 468
      
      const dpr = uni.getSystemInfoSync().pixelRatio || 1
      
      // 重置变换矩阵
      this.ctx.setTransform(1, 0, 0, 1, 0, 0)
      
      // 设置 canvas 实际绘制尺寸（考虑设备像素比）
      this.canvas.width = width * dpr
      this.canvas.height = height * dpr
      
      // 重新缩放上下文
      this.ctx.scale(dpr, dpr)
      
      this.updateCanvasRect()
      
      // 重新绘制
      this.$nextTick(() => {
        this.drawCanvas()
      })
    },
    
    updateCanvasRect() {
      const query = uni.createSelectorQuery().in(this)
      query.select(`#${this.canvasId}`).boundingClientRect(rect => {
        if (rect) {
          this.canvasRect = rect
        }
      }).exec()
    },
    
    drawCanvas() {
      if (!this.canvas || !this.ctx) {
        return
      }
      
      const ctx = this.ctx
      const width = this.canvasWidth || 375
      const height = this.canvasHeight || 667
      
      // 清空画布（使用逻辑像素，因为 ctx 已经缩放过了）
      ctx.clearRect(0, 0, width, height)
      
      // 背景色 - 根据主题模式设置
      ctx.fillStyle = this.isDarkMode ? '#1a1a1a' : '#FFFFFF'
      ctx.fillRect(0, 0, width, height)
      
      ctx.save()
      ctx.translate(this.offsetX, this.offsetY)
      ctx.scale(this.zoom, this.zoom)
      
      // 绘制像素
      this.pixels.forEach((color, key) => {
        const [x, y] = key.split(',').map(Number)
        
        let alpha = 1
        let shouldGlow = false
        
        // 高亮逻辑
        if (this.highlightRow !== null) {
          // 逐行模式
          if (this.highlightColor) {
            // 选择了颜色：只显示当前行的该颜色
            if (y !== this.highlightRow || color !== this.highlightColor) {
              alpha = 0.15  // 其他都变得很暗
            } else {
              // 当前行且是高亮颜色，添加发光效果
              shouldGlow = true
            }
          } else {
            // 没选择颜色：只高亮当前行
            if (y !== this.highlightRow) {
              alpha = 0.25  // 不是当前行，变暗
            }
          }
        } else {
          // 颜色模式
          if (this.highlightColor) {
            if (color !== this.highlightColor) {
              alpha = 0.25
            } else {
              shouldGlow = true
            }
          }
        }
        
        // 绘制像素
        ctx.globalAlpha = alpha
        ctx.fillStyle = color
        
        // 只对高亮的像素添加轻微发光
        if (shouldGlow) {
          ctx.shadowColor = color
          ctx.shadowBlur = 2  // 减小发光范围
        }
        
        ctx.fillRect(x, y, 1, 1)
        
        // 重置
        ctx.globalAlpha = 1
        ctx.shadowBlur = 0
      })
      
      // 绘制网格
      if (this.gridVisible && this.zoom > 1.5) {
        const gridColor = this.isDarkMode ? '#ffffff' : '#000000'
        
        // 细网格线
        ctx.lineWidth = 0.05
        ctx.strokeStyle = gridColor
        ctx.globalAlpha = this.isDarkMode ? 0.1 : 0.15
        
        ctx.beginPath()
        for (let x = 0; x <= this.width; x++) {
          ctx.moveTo(x, 0)
          ctx.lineTo(x, this.height)
        }
        for (let y = 0; y <= this.height; y++) {
          ctx.moveTo(0, y)
          ctx.lineTo(this.width, y)
        }
        ctx.stroke()
        
        // 粗网格线（52x52板子边界）
        ctx.lineWidth = 0.15
        ctx.strokeStyle = gridColor
        ctx.globalAlpha = this.isDarkMode ? 0.2 : 0.3
        ctx.beginPath()
        for (let x = 0; x <= this.width; x += 52) {
          ctx.moveTo(x, 0)
          ctx.lineTo(x, this.height)
        }
        for (let y = 0; y <= this.height; y += 52) {
          ctx.moveTo(0, y)
          ctx.lineTo(this.width, y)
        }
        ctx.stroke()
        
        ctx.globalAlpha = 1
      }
      
      ctx.restore()
    },
    
    handleDraw(clientX, clientY) {
      if (!this.canvasRect) {
        // 如果还没有缓存位置，先获取一次
        this.updateCanvasRect()
        return
      }
      
      // 计算相对于 canvas 的坐标
      const x = (clientX - this.canvasRect.left - this.offsetX) / this.zoom
      const y = (clientY - this.canvasRect.top - this.offsetY) / this.zoom
      
      const pixelX = Math.floor(x)
      const pixelY = Math.floor(y)
      
      if (this.lastDrawPos?.x === pixelX && this.lastDrawPos?.y === pixelY) return
      
      if (pixelX >= 0 && pixelX < this.width && pixelY >= 0 && pixelY < this.height) {
        this.$emit('pixel-click', pixelX, pixelY)
        this.lastDrawPos = { x: pixelX, y: pixelY }
      }
    },
    
    handleTouchStart(e) {
      const touches = e.touches
      this.activePointers = new Map()
      
      Array.from(touches).forEach((t, index) => {
        this.activePointers.set(index, { x: t.clientX, y: t.clientY })
      })
      
      if (this.activePointers.size === 1) {
        const touch = touches[0]
        if (this.allowSingleTouchPan) {
          this.lastCenter = { x: touch.clientX, y: touch.clientY }
          this.lastDist = 0
        } else {
          this.handleDraw(touch.clientX, touch.clientY)
        }
      }
    },
    
    handleTouchMove(e) {
      const touches = e.touches
      this.activePointers = new Map()
      
      Array.from(touches).forEach((t, index) => {
        this.activePointers.set(index, { x: t.clientX, y: t.clientY })
      })
      
      const pointers = Array.from(this.activePointers.values())
      
      if (pointers.length === 1) {
        const touch = touches[0]
        if (this.allowSingleTouchPan) {
          if (this.lastCenter) {
            const dx = touch.clientX - this.lastCenter.x
            const dy = touch.clientY - this.lastCenter.y
            this.$emit('pan', dx, dy)
            this.lastCenter = { x: touch.clientX, y: touch.clientY }
          }
        } else {
          this.handleDraw(touch.clientX, touch.clientY)
        }
      } else if (pointers.length === 2) {
        const p1 = pointers[0]
        const p2 = pointers[1]
        
        const clientCenterX = (p1.x + p2.x) / 2
        const clientCenterY = (p1.y + p2.y) / 2
        
        const dist = Math.hypot(p1.x - p2.x, p1.y - p2.y)
        
        if (this.lastCenter && this.lastDist) {
          const dx = clientCenterX - this.lastCenter.x
          const dy = clientCenterY - this.lastCenter.y
          this.$emit('pan', dx, dy)
          
          const zoomDelta = (dist - this.lastDist) * 0.05
          if (Math.abs(zoomDelta) > 0.05) {
            this.$emit('zoom', zoomDelta, clientCenterX, clientCenterY)
          }
        }
        
        this.lastCenter = { x: clientCenterX, y: clientCenterY }
        this.lastDist = dist
      }
    },
    
    handleTouchEnd(e) {
      this.activePointers = new Map()
      
      if (e.touches.length < 2) {
        this.lastDist = null
      }
      
      if (e.touches.length === 0) {
        this.lastDrawPos = null
        this.lastCenter = null
      } else if (e.touches.length === 1) {
        const p = e.touches[0]
        this.lastCenter = { x: p.clientX, y: p.clientY }
      }
    }
  }
}
</script>

<style scoped>
.pixel-canvas {
  width: 100%;
  height: 100%;
  display: block;
}
</style>

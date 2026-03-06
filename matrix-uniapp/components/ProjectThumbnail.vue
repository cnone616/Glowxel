<template>
  <!-- #ifdef H5 -->
  <!-- H5：优先显示缓存的缩略图 -->
  <image 
    v-if="cachedThumbnail"
    :src="cachedThumbnail"
    class="thumbnail-image"
    mode="aspectFit"
  />
  <canvas 
    v-else
    :canvas-id="canvasId"
    :id="canvasId"
    type="2d"
    class="thumbnail-canvas"
  ></canvas>
  <!-- #endif -->
  
  <!-- #ifdef MP-WEIXIN -->
  <!-- 小程序：直接显示canvas -->
  <canvas 
    :canvas-id="canvasId"
    :id="canvasId"
    type="2d"
    class="thumbnail-canvas"
  ></canvas>
  <!-- #endif -->
</template>

<script>
import { useProjectStore } from '../store/project.js'

export default {
  props: {
    projectId: {
      type: String,
      required: true
    },
    width: {
      type: Number,
      required: true
    },
    height: {
      type: Number,
      required: true
    },
    thumbnail: {
      type: String,
      default: ''
    }
  },
  
  data() {
    return {
      canvas: null,
      ctx: null,
      pixels: new Map(),
      cachedThumbnail: '',
      projectStore: null
    }
  },
  
  computed: {
    canvasId() {
      return `thumbnail-${this.projectId}`
    }
  },
  
  mounted() {
    this.projectStore = useProjectStore()
    
    // #ifdef H5
    // H5：优先使用传入的缩略图（支持base64和临时文件路径）
    if (this.thumbnail) {
      this.cachedThumbnail = this.thumbnail
      return
    }
    // #endif
    
    // 小程序或H5无缓存：从像素数据生成
    this.loadPixels()
    this.$nextTick(() => {
      // #ifdef MP-WEIXIN
      // 小程序需要更长的延迟等待DOM渲染
      setTimeout(() => {
        this.initCanvas()
      }, 500)
      // #endif
      
      // #ifdef H5
      setTimeout(() => {
        this.initCanvas()
      }, 100)
      // #endif
    })
  },
  
  methods: {
    loadPixels() {
      try {
        const savedPixels = uni.getStorageSync(`pixels-${this.projectId}`)
        if (savedPixels) {
          const parsed = JSON.parse(savedPixels)
          this.pixels = new Map(parsed)
        }
      } catch (e) {
        console.error('Failed to load pixels for thumbnail', e)
      }
    },
    
    initCanvas() {
      // 先获取外层thumbnail-wrapper的尺寸
      const wrapperQuery = uni.createSelectorQuery().in(this.$parent)
      wrapperQuery.select('.thumbnail-wrapper')
        .boundingClientRect()
        .exec((wrapperRes) => {
          let wrapperWidth = 260
          let wrapperHeight = 260
          
          if (wrapperRes && wrapperRes[0]) {
            wrapperWidth = wrapperRes[0].width || 260
            wrapperHeight = wrapperRes[0].height || 260
          }
          
          // 再获取canvas节点
          const query = uni.createSelectorQuery().in(this)
          query.select(`#${this.canvasId}`)
            .fields({ node: true, size: true })
            .exec((res) => {
              if (!res || !res[0]) {
                return
              }
              
              const canvas = res[0].node
              this.canvas = canvas
              this.ctx = canvas.getContext('2d')
              
              // #ifdef MP-WEIXIN
              // 小程序：使用wrapper的实际尺寸
              const dpr = uni.getWindowInfo().pixelRatio || 1
              canvas.width = wrapperWidth * dpr
              canvas.height = wrapperHeight * dpr
              this.ctx.scale(dpr, dpr)
              this.drawCanvas(wrapperWidth, wrapperHeight)
              // #endif
              
              // #ifdef H5
              // H5：使用容器尺寸
              const containerWidth = res[0].width
              const containerHeight = res[0].height
              canvas.width = containerWidth
              canvas.height = containerHeight
              this.drawCanvas(containerWidth, containerHeight)
              // #endif
            })
        })
    },
    
    drawCanvas(containerWidth, containerHeight) {
      this.ctx.clearRect(0, 0, containerWidth, containerHeight)
      
      const maxDimension = Math.max(this.width || 52, this.height || 52)
      
      // #ifdef H5
      const multiplier = (maxDimension > 52) ? 3 : 2
      // #endif
      
      // #ifdef MP-WEIXIN
      const multiplier = (maxDimension > 52) ? 1 : 1
      console.log('123')
      // #endif
      
      let scale = Math.min(containerWidth / (maxDimension * multiplier), containerHeight / (maxDimension * multiplier))
      
      if (scale >= 1) {
        // #ifdef H5
        scale = Math.floor(scale)
        // #endif
        
        // #ifdef MP-WEIXIN
        scale = 3.3
        // #endif
      } else {
        scale = Math.round(scale * 100) / 100
      }
      
      const centerOffsetX = (maxDimension - (this.width || 52)) / 2
      const centerOffsetY = (maxDimension - (this.height || 52)) / 2
      
      this.pixels.forEach((color, key) => {
        const [x, y] = key.split(',').map(Number)
        this.ctx.fillStyle = color
        this.ctx.fillRect(
          (x + centerOffsetX) * scale, 
          (y + centerOffsetY) * scale, 
          scale, 
          scale
        )
      })
    },
    
    saveThumbnail(canvas) {
      // 暂时不保存缩略图
      return
      
      const thumbWidth = 520
      const thumbHeight = 520
      
      // 创建一个新的离屏 canvas（如果支持）
      let offscreenCanvas = canvas
      let offscreenCtx = this.ctx
      
      // 小程序不支持createOffscreenCanvas，直接使用当前canvas
      offscreenCanvas.width = thumbWidth
      offscreenCanvas.height = thumbHeight
      
      offscreenCtx = offscreenCanvas.getContext('2d')
      offscreenCtx.clearRect(0, 0, thumbWidth, thumbHeight)
      
      // 使用最大边作为正方形基准
      const maxDimension = Math.max(this.width, this.height)
      const pixelSize = 3 / (maxDimension / 52)
      
      // 计算居中偏移量
      const centerOffsetX = (maxDimension - this.width) / 2
      const centerOffsetY = (maxDimension - this.height) / 2
      
      // 绘制像素（加上居中偏移）
      this.pixels.forEach((color, key) => {
        const [x, y] = key.split(',').map(Number)
        offscreenCtx.fillStyle = color
        offscreenCtx.fillRect(
          (x + centerOffsetX) * pixelSize, 
          (y + centerOffsetY) * pixelSize, 
          pixelSize, 
          pixelSize
        )
      })
      
      // 等待canvas绘制完成后再转换为图片
      setTimeout(() => {
        // 将 canvas 转换为图片并保存
        uni.canvasToTempFilePath({
          canvas: offscreenCanvas,
          x: 0,
          y: 0,
          width: thumbWidth,
          height: thumbHeight,
          destWidth: thumbWidth,
          destHeight: thumbHeight,
          fileType: 'png',
          quality: 1,
          success: (res) => {
            // #ifdef MP-WEIXIN
            // 小程序：将临时文件保存到本地永久存储
            uni.getFileSystemManager().saveFile({
              tempFilePath: res.tempFilePath,
              success: (saveRes) => {
                const savedPath = saveRes.savedFilePath
                this.projectStore.updateProjectThumbnail(this.projectId, savedPath)
                this.cachedThumbnail = savedPath
                console.log('缩略图已保存到永久存储:', savedPath)
              },
              fail: (err) => {
                console.error('保存到永久存储失败，使用临时路径:', err)
                // 失败时仍使用临时路径
                this.projectStore.updateProjectThumbnail(this.projectId, res.tempFilePath)
                this.cachedThumbnail = res.tempFilePath
              }
            })
            // #endif
            
            // #ifdef H5
            // H5：直接使用临时路径（base64）
            this.projectStore.updateProjectThumbnail(this.projectId, res.tempFilePath)
            this.cachedThumbnail = res.tempFilePath
            // #endif
          },
          fail: (err) => {
            console.error('生成缩略图失败:', err)
          }
        }, this)
      }, 300) // 延迟300ms等待canvas绘制完成
    },
    
    regenerate() {
      // 清空缓存的缩略图，重新生成
      this.cachedThumbnail = ''
      this.loadPixels()
      this.$nextTick(() => {
        setTimeout(() => {
          this.initCanvas()
        }, 100)
      })
    }
  }
}
</script>

<style scoped>
.thumbnail-image {
  width: 100%;
  height: 100%;
  display: block;
  object-fit: contain;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}

.thumbnail-canvas {
  display: block;
  object-fit: contain;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
  width: 100%;
  height: 100%;
  position: absolute;
  left: 0;
  top: 0;
  padding: 20rpx;
  box-sizing: border-box;
}
</style>

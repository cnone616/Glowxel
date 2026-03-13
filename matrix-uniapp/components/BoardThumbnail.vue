<template>
  <!-- 优先显示缓存的缩略图 -->
  <image 
    v-if="cachedThumbnail"
    :src="cachedThumbnail"
    class="thumbnail-image"
    mode="aspectFit"
  />
  <!-- 如果没有缓存，使用 canvas 绘制 -->
  <canvas 
    v-else
    :canvas-id="canvasId"
    :id="canvasId"
    type="2d"
    class="thumbnail-canvas"
  ></canvas>
</template>

<script>
import { useProjectStore } from '../store/project.js'

export default {
  props: {
    boardId: {
      type: String,
      required: true
    },
    pixels: {
      type: Map,
      required: true
    },
    projectId: {
      type: String,
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
      cachedThumbnail: '',
      projectStore: null
    }
  },
  
  computed: {
    canvasId() {
      return `board-thumb-${this.boardId}`
    }
  },
  
  mounted() {
    this.projectStore = useProjectStore()
    
    // 不使用缓存，每次动态生成
    this.$nextTick(() => {
      setTimeout(() => {
        this.initCanvas()
      }, 100)
    })
  },
  
  methods: {
    initCanvas() {
      const query = uni.createSelectorQuery().in(this)
      query.select(`#${this.canvasId}`)
        .fields({ node: true, size: true })
        .exec((res) => {
          if (!res || !res[0]) {
            return
          }
          
          const canvas = res[0].node
          const containerWidth = res[0].width
          const containerHeight = res[0].height
          
          this.canvas = canvas
          this.ctx = canvas.getContext('2d', { willReadFrequently: true })
          
          // 取宽高中的较小值，确保是正方形
          const size = Math.min(containerWidth, containerHeight)
          
          // 设置 canvas 物理尺寸为正方形
          canvas.width = size
          canvas.height = size
          
          // 清空
          this.ctx.clearRect(0, 0, size, size)
          
          // 画布总览：固定 64x64，乘2后是 128x128 的坐标系统
          const boardSize = 64
          const multiplier = 3
          const coordinateSize = boardSize * multiplier // 192
          
          // 计算缩放比例
          let scale = size / coordinateSize
          
          // 保留两位小数
          scale = Math.round(scale * 100) / 100
          
          // 绘制像素（在 104x104 的坐标系统中绘制）
          this.pixels.forEach((color, key) => {
            const [x, y] = key.split(',').map(Number)
            this.ctx.fillStyle = color
            this.ctx.fillRect(x * scale, y * scale, scale, scale)
          })
          
          // 不保存缩略图
        })
    },
    
    saveThumbnail(canvas, size) {
      // 不保存board缩略图，节省存储空间
    }
  }
}
</script>

<style scoped>
.thumbnail-image {
  width: 100%;
  height: 100%;
  display: block;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}

.thumbnail-canvas {
  width: 100%;
  height: 100%;
  display: block;
  image-rendering: pixelated;
  image-rendering: -moz-crisp-edges;
  image-rendering: crisp-edges;
}
</style>

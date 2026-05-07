/**
 * 图片导入工具
 * 将图片转换为像素画
 */

import { ARTKAL_COLORS_FULL } from '../data/artkal-colors-full.js'

/**
 * 计算两个颜色的距离（欧几里得距离）
 */
function colorDistance(color1, color2) {
  const r1 = (color1 >> 16) & 0xff
  const g1 = (color1 >> 8) & 0xff
  const b1 = color1 & 0xff
  
  const r2 = (color2 >> 16) & 0xff
  const g2 = (color2 >> 8) & 0xff
  const b2 = color2 & 0xff
  
  return Math.sqrt(
    Math.pow(r1 - r2, 2) +
    Math.pow(g1 - g2, 2) +
    Math.pow(b1 - b2, 2)
  )
}

/**
 * 找到最接近的 Artkal 颜色
 */
function findClosestColor(rgb, palette) {
  const colors = palette && palette.length > 0
    ? ARTKAL_COLORS_FULL.filter(c => palette.includes(c.code))
    : ARTKAL_COLORS_FULL
  
  let minDistance = Infinity
  let closestColor = colors[0]
  
  for (const color of colors) {
    const colorRgb = parseInt(color.hex.slice(1), 16)
    const distance = colorDistance(rgb, colorRgb)
    
    if (distance < minDistance) {
      minDistance = distance
      closestColor = color
    }
  }
  
  return closestColor
}

/**
 * 检测图片是否可能是像素画
 * 通过分析颜色数量和边缘特征判断
 */
function detectPixelArt(imageData, width, height) {
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
}

/**
 * 从图片路径导入像素数据
 */
export async function importImageAsPixels(options) {
  const {
    imagePath,
    targetWidth,
    targetHeight,
    palette = null,
    pixelPerfect = 'auto' // 'auto' | true | false
  } = options
  
  return new Promise((resolve, reject) => {
    // 创建临时 Canvas 用于读取图片
    const query = uni.createSelectorQuery()
    query.select('#importCanvas')
      .fields({ node: true })
      .exec((res) => {
        if (!res || !res[0]) {
          reject(new Error('无法获取导入 Canvas'))
          return
        }
        
        const canvas = res[0].node
        const ctx = canvas.getContext('2d')
        
        // 设置 Canvas 尺寸
        canvas.width = targetWidth
        canvas.height = targetHeight
        
        // 加载图片 - 区分平台
        let image
        // #ifdef H5
        image = new Image()
        image.crossOrigin = 'anonymous'
        // #endif
        
        // #ifndef H5
        image = canvas.createImage()
        // #endif
        
        image.onload = () => {
          // 先绘制原始尺寸以检测是否为像素画
          const tempCanvas = canvas
          const tempCtx = ctx
          tempCanvas.width = image.width
          tempCanvas.height = image.height
          
          // 第一次绘制：用于检测
          tempCtx.drawImage(image, 0, 0)
          const detectData = tempCtx.getImageData(0, 0, image.width, image.height)
          
          // 判断是否使用像素完美模式
          let usePixelPerfect = pixelPerfect === true
          if (pixelPerfect === 'auto') {
            usePixelPerfect = detectPixelArt(detectData, image.width, image.height)
          }
          
          // 重新设置 Canvas 尺寸为目标尺寸
          canvas.width = targetWidth
          canvas.height = targetHeight
          
          // 设置图像平滑选项
          if (usePixelPerfect) {
            // 禁用所有平滑算法，使用最近邻采样
            ctx.imageSmoothingEnabled = false
            // 兼容性写法
            ctx.mozImageSmoothingEnabled = false
            ctx.webkitImageSmoothingEnabled = false
            ctx.msImageSmoothingEnabled = false
          } else {
            ctx.imageSmoothingEnabled = true
            ctx.imageSmoothingQuality = 'high'
          }
          
          // 绘制图片到目标尺寸
          ctx.drawImage(image, 0, 0, targetWidth, targetHeight)
          
          // 读取像素数据
          const imageData = ctx.getImageData(0, 0, targetWidth, targetHeight)
          const data = imageData.data
          
          // 转换为像素 Map
          const pixels = new Map()
          const usedColors = new Set()
          
          for (let y = 0; y < targetHeight; y++) {
            for (let x = 0; x < targetWidth; x++) {
              const index = (y * targetWidth + x) * 4
              const r = data[index]
              const g = data[index + 1]
              const b = data[index + 2]
              const a = data[index + 3]
              
              // 跳过透明像素
              if (a < 128) continue
              
              // 转换为 RGB 整数
              const rgb = (r << 16) | (g << 8) | b
              
              // 找到最接近的 Artkal 颜色
              const closestColor = findClosestColor(rgb, palette)
              
              // 保存像素
              const key = `${x},${y}`
              pixels.set(key, closestColor.hex)
              usedColors.add(closestColor.code)
            }
          }
          
          resolve({
            pixels,
            usedColors: Array.from(usedColors),
            isPixelArt: usePixelPerfect
          })
        }
        
        image.onerror = (err) => {
          reject(new Error('图片加载失败'))
        }
        
        image.src = imagePath
      })
  })
}

/**
 * 选择图片并导入（已废弃，请直接使用 importImageAsPixels）
 */
export async function chooseAndImportImage(options) {
  return importImageAsPixels(options)
}

import { findClosestColor, getColorByCode } from '@/data/artkal-colors'

/**
 * 将图片转换为像素数据
 * @param {HTMLImageElement} image - 图片对象
 * @param {number} targetWidth - 目标宽度
 * @param {number} targetHeight - 目标高度
 * @param {Array} colorCodes - 颜色代码数组
 * @returns {Map} 像素数据 Map
 */
export function imageToPixels(image, targetWidth, targetHeight, colorCodes) {
  const canvas = document.createElement('canvas')
  canvas.width = targetWidth
  canvas.height = targetHeight
  const ctx = canvas.getContext('2d')
  
  // 禁用图像平滑以保持像素艺术风格
  ctx.imageSmoothingEnabled = false
  
  // 绘制缩放后的图片
  ctx.drawImage(image, 0, 0, targetWidth, targetHeight)
  
  // 获取像素数据
  const imageData = ctx.getImageData(0, 0, targetWidth, targetHeight)
  const pixels = imageData.data
  
  const pixelMap = new Map()
  
  // 转换每个像素
  for (let y = 0; y < targetHeight; y++) {
    for (let x = 0; x < targetWidth; x++) {
      const idx = (y * targetWidth + x) * 4
      const r = pixels[idx]
      const g = pixels[idx + 1]
      const b = pixels[idx + 2]
      const a = pixels[idx + 3]
      
      // 跳过透明像素
      if (a < 128) continue
      
      // 找到最接近的颜色
      const closestColor = findClosestColor(r, g, b, colorCodes)
      if (closestColor) {
        pixelMap.set(`${x},${y}`, closestColor.code)
      }
    }
  }
  
  return pixelMap
}

/**
 * 统计使用的颜色
 * @param {Map} pixels - 像素数据
 * @returns {Map} 使用的颜色及数量的 Map
 */
export function getUsedColors(pixels) {
  const colorCount = new Map()
  
  pixels.forEach(colorCode => {
    colorCount.set(colorCode, (colorCount.get(colorCode) || 0) + 1)
  })
  
  return colorCount
}

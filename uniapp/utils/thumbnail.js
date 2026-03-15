/**
 * 缩略图生成和管理工具
 */

/**
 * 从像素数据生成缩略图 base64
 * @param {Map} pixels - 像素数据 Map
 * @param {number} width - 画布宽度
 * @param {number} height - 画布高度
 * @param {number} thumbnailSize - 缩略图尺寸（默认 200）
 * @returns {Promise<string>} base64 字符串
 */
export function generateThumbnail(pixels, width, height, thumbnailSize = 200) {
  return new Promise((resolve, reject) => {
    try {
      // 创建离屏 canvas
      const canvas = document.createElement('canvas')
      const ctx = canvas.getContext('2d')
      
      // 设置缩略图尺寸
      canvas.width = thumbnailSize
      canvas.height = thumbnailSize
      
      // 清空背景
      ctx.clearRect(0, 0, thumbnailSize, thumbnailSize)
      
      // 计算缩放比例（保持宽高比，居中显示）
      const scale = Math.min(thumbnailSize / width, thumbnailSize / height)
      const scaledWidth = width * scale
      const scaledHeight = height * scale
      const offsetX = (thumbnailSize - scaledWidth) / 2
      const offsetY = (thumbnailSize - scaledHeight) / 2
      
      // 绘制像素
      pixels.forEach((color, key) => {
        const [x, y] = key.split(',').map(Number)
        ctx.fillStyle = color
        ctx.fillRect(
          offsetX + x * scale,
          offsetY + y * scale,
          Math.ceil(scale),
          Math.ceil(scale)
        )
      })
      
      // 转换为 base64
      const base64 = canvas.toDataURL('image/png', 0.8)
      resolve(base64)
    } catch (error) {
      console.error('生成缩略图失败:', error)
      reject(error)
    }
  })
}

/**
 * 从像素数据生成缩略图 base64 (uni-app 版本)
 * @param {Map} pixels - 像素数据 Map
 * @param {number} width - 画布宽度
 * @param {number} height - 画布高度
 * @param {number} thumbnailSize - 缩略图尺寸（默认 200）
 * @returns {Promise<string>} base64 字符串
 */
export function generateThumbnailUniapp(pixels, width, height, thumbnailSize = 200) {
  return new Promise((resolve, reject) => {
    try {
      // 在 uni-app 中需要使用页面上的 canvas
      // 这里返回一个标记，让调用方知道需要使用 canvas 组件
      resolve(null)
    } catch (error) {
      console.error('生成缩略图失败:', error)
      reject(error)
    }
  })
}

/**
 * 检查缩略图是否有效
 * @param {string} thumbnail - base64 字符串
 * @returns {boolean}
 */
export function isValidThumbnail(thumbnail) {
  return thumbnail && typeof thumbnail === 'string' && thumbnail.startsWith('data:image/')
}

/**
 * 获取缩略图大小（KB）
 * @param {string} thumbnail - base64 字符串
 * @returns {number}
 */
export function getThumbnailSize(thumbnail) {
  if (!thumbnail) return 0
  // base64 字符串长度约等于字节数 * 1.33
  return (thumbnail.length * 0.75) / 1024
}

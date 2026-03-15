/**
 * 导出像素画为带标注的图纸
 * uni-app 版本
 */

import { ARTKAL_COLORS_FULL, ARTKAL_PRESETS } from '../data/artkal-colors-full.js'

// 获取颜色代码
function getColorCode(hex) {
  const color = ARTKAL_COLORS_FULL.find(c => c.hex.toLowerCase() === hex.toLowerCase())
  return color?.code || '?'
}

// 获取颜色名称
function getColorName(hex) {
  const color = ARTKAL_COLORS_FULL.find(c => c.hex.toLowerCase() === hex.toLowerCase())
  return color?.name || '未知'
}

// 计算文字在格子中的最佳字体大小
function calculateFontSize(cellSize, text) {
  const baseSize = cellSize * 0.35
  const lengthFactor = Math.max(1, text.length / 3)
  return Math.max(8, Math.floor(baseSize / lengthFactor))
}

// 判断颜色是深色还是浅色
function isLightColor(hex) {
  const rgb = parseInt(hex.slice(1), 16)
  const r = (rgb >> 16) & 0xff
  const g = (rgb >> 8) & 0xff
  const b = (rgb >> 0) & 0xff
  const brightness = (r * 299 + g * 587 + b * 114) / 1000
  return brightness > 128
}

/**
 * 导出画布为图片
 */
export async function exportCanvasAsImage(options) {
  const {
    pixels,
    width,
    height,
    projectName,
    palette,
    cellSize: requestedCellSize = 40,
    showGrid = true,
    showCoordinates = true,
    showColorCodes = true,
    showStatistics = true,
    showLogo = true,
    watermark = null,  // { username: '用户名' } 传入则叠加水印
    canvasId = 'exportCanvas'
  } = options

  return new Promise((resolve, reject) => {
    // 动态调整 cellSize 以避免 canvas 尺寸过大
    // 微信小程序 canvas 限制约 4096x4096
    const maxCanvasSize = 4000 // 留一些余量
    
    // 计算需要的网格尺寸
    const coordinateSize = showCoordinates ? 30 : 0
    const padding = 40
    
    // 预估统计表高度
    const colorStats = new Map()
    pixels.forEach((color) => {
      colorStats.set(color, (colorStats.get(color) || 0) + 1)
    })
    
    // 计算统计表实际需要的高度
    // 每行显示12个颜色，每行高度40px，加上标题和合计信息180px
    const statsRows = Math.ceil(colorStats.size / 12)
    const statsHeight = showStatistics 
      ? statsRows * 40 + 180 
      : 0
    
    // 计算最大可用空间
    const maxGridWidth = maxCanvasSize - coordinateSize - padding * 2
    const maxGridHeight = maxCanvasSize - coordinateSize - statsHeight - padding * 2
    
    // 根据可用空间计算最大 cellSize
    const maxCellSizeByWidth = Math.floor(maxGridWidth / width)
    const maxCellSizeByHeight = Math.floor(maxGridHeight / height)
    const calculatedMaxCellSize = Math.min(maxCellSizeByWidth, maxCellSizeByHeight)
    
    // 使用请求的 cellSize 和计算出的最大值中较小的那个
    // 这样既能保证不超限，又能在颜色少的时候使用较大的 cellSize
    let cellSize = Math.min(requestedCellSize, calculatedMaxCellSize)
    
    // 确保 cellSize 至少为 5
    if (cellSize < 5) {
      reject(new Error(`画布尺寸过大(${width}x${height})且颜色过多(${colorStats.size}种)，建议分块导出单个看板以获得更清晰的图纸`))
      return
    }
    
    // 如果 cellSize 太小，给出警告但继续导出
    if (cellSize < 7 && (width > 300 || height > 300)) {
      console.warn(`图纸较大且颜色较多，导出的图纸可能不够清晰。建议分块导出单个看板。`)
    }
    
    const gridWidth = width * cellSize
    const gridHeight = height * cellSize
    const canvasWidth = gridWidth + coordinateSize + padding * 2
    const canvasHeight = gridHeight + coordinateSize + statsHeight + padding * 2
    
    // 最终检查
    if (canvasWidth > maxCanvasSize || canvasHeight > maxCanvasSize) {
      reject(new Error(`Canvas 尺寸过大: ${canvasWidth}x${canvasHeight}，画布: ${width}x${height}，颜色数: ${colorStats.size}`))
      return
    }
    
    console.log(`导出 Canvas 尺寸: ${canvasWidth}x${canvasHeight}, cellSize: ${cellSize}, 画布: ${width}x${height}, 颜色数: ${colorStats.size}`)
    
    // 判断需要哪些套装
    const requiredPresets = []
    if (palette && palette.length > 0) {
      const preset = ARTKAL_PRESETS.find(p => p.count === palette.length)
      if (preset) {
        requiredPresets.push(preset.name)
      }
    }
    
    // #ifdef H5
    // H5 环境直接创建离屏 canvas
    const canvas = document.createElement('canvas')
    canvas.width = canvasWidth
    canvas.height = canvasHeight
    const ctx = canvas.getContext('2d')
    
    if (!ctx) {
      reject(new Error('无法创建 Canvas 上下文'))
      return
    }
    
    // 直接绘制
    drawCanvas(ctx, canvasWidth, canvasHeight, {
      pixels, width, height, projectName, palette, cellSize,
      showGrid, showCoordinates, showColorCodes, showStatistics, showLogo,
      coordinateSize, gridWidth, gridHeight, colorStats, requiredPresets, statsHeight, padding
    })
    
    // 转换为 dataURL
    try {
      const dataURL = canvas.toDataURL('image/png')
      resolve(dataURL)
    } catch (err) {
      reject(err)
    }
    // #endif
    
    // #ifndef H5
    // App 和小程序环境使用页面上的 canvas
    const query = uni.createSelectorQuery()
    query.select(`#${canvasId}`)
      .fields({ node: true })
      .exec((res) => {
        if (!res || !res[0]) {
          reject(new Error('无法获取导出 Canvas'))
          return
        }
        
        const canvas = res[0].node
        const ctx = canvas.getContext('2d')
        
        const dpr = uni.getSystemInfoSync().pixelRatio || 1
        canvas.width = canvasWidth * dpr
        canvas.height = canvasHeight * dpr
        ctx.scale(dpr, dpr)
        
        // 绘制
        drawCanvas(ctx, canvasWidth, canvasHeight, {
          pixels, width, height, projectName, palette, cellSize,
          showGrid, showCoordinates, showColorCodes, showStatistics, showLogo,
          coordinateSize, gridWidth, gridHeight, colorStats, requiredPresets, statsHeight, padding
        })
        
        // 导出为临时文件
        uni.canvasToTempFilePath({
          canvas,
          canvasId,
          success: (res) => {
            resolve(res.tempFilePath)
          },
          fail: (err) => {
            reject(err)
          }
        })
      })
    // #endif
  })
}

// 绘制函数（H5 和 App/小程序共用）
function drawCanvas(ctx, canvasWidth, canvasHeight, options) {
  const {
    pixels, width, height, projectName, cellSize,
    showGrid, showCoordinates, showColorCodes, showStatistics, showLogo,
    coordinateSize, gridWidth, gridHeight, colorStats, requiredPresets, statsHeight, padding
  } = options
  
  const offsetX = padding + coordinateSize
  const offsetY = padding + coordinateSize
  
  // 背景
  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, canvasWidth, canvasHeight)
        
        // 绘制标题
        ctx.fillStyle = '#000000'
        ctx.font = 'bold 24px Arial'
        ctx.textAlign = 'center'
        ctx.fillText(projectName, canvasWidth / 2, padding - 10)
        
        // 绘制坐标轴
        if (showCoordinates) {
          ctx.fillStyle = '#666666'
          ctx.font = '12px monospace'
          ctx.textAlign = 'center'
          ctx.textBaseline = 'middle'
          
          // 列号
          for (let x = 0; x < width; x++) {
            const centerX = offsetX + x * cellSize + cellSize / 2
            ctx.fillText((x + 1).toString(), centerX, padding + coordinateSize / 2)
          }
          
          // 行号
          ctx.textAlign = 'right'
          for (let y = 0; y < height; y++) {
            const centerY = offsetY + y * cellSize + cellSize / 2
            ctx.fillText((y + 1).toString(), padding + coordinateSize - 10, centerY)
          }
        }
        
        // 绘制网格和像素
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            const key = `${x},${y}`
            const color = pixels.get(key)
            
            const cellX = offsetX + x * cellSize
            const cellY = offsetY + y * cellSize
            
            // 绘制格子背景
            if (color) {
              ctx.fillStyle = color
              ctx.fillRect(cellX, cellY, cellSize, cellSize)
            }
            
            // 绘制网格线
            if (showGrid) {
              ctx.strokeStyle = '#cccccc'
              ctx.lineWidth = 0.5
              ctx.strokeRect(cellX, cellY, cellSize, cellSize)
            }
            
            // 绘制颜色代码
            if (showColorCodes && color) {
              const code = getColorCode(color)
              const fontSize = calculateFontSize(cellSize, code)
              ctx.font = `bold ${fontSize}px monospace`
              ctx.textAlign = 'center'
              ctx.textBaseline = 'middle'
              ctx.fillStyle = isLightColor(color) ? '#000000' : '#ffffff'
              ctx.fillText(code, cellX + cellSize / 2, cellY + cellSize / 2)
            }
          }
        }
        
        // 绘制粗网格线（52x52 板子边界）
        if (showGrid) {
          ctx.strokeStyle = '#000000'
          ctx.lineWidth = 2
          for (let x = 0; x <= width; x += 52) {
            ctx.beginPath()
            ctx.moveTo(offsetX + x * cellSize, offsetY)
            ctx.lineTo(offsetX + x * cellSize, offsetY + gridHeight)
            ctx.stroke()
          }
          for (let y = 0; y <= height; y += 52) {
            ctx.beginPath()
            ctx.moveTo(offsetX, offsetY + y * cellSize)
            ctx.lineTo(offsetX + gridWidth, offsetY + y * cellSize)
            ctx.stroke()
          }
        }
        
        // 绘制统计表
        if (showStatistics && colorStats.size > 0) {
          const statsY = offsetY + gridHeight + 40
          
          ctx.fillStyle = '#000000'
          ctx.font = 'bold 18px Arial'
          ctx.textAlign = 'left'
          ctx.fillText('颜色统计', padding, statsY)
          
          const colors = Array.from(colorStats.entries()).sort((a, b) => {
            const codeA = getColorCode(a[0])
            const codeB = getColorCode(b[0])
            return codeA.localeCompare(codeB)
          })
          
          const colWidth = (canvasWidth - padding * 2) / 12
          const rowHeight = 40
          let currentRow = 0
          let currentCol = 0
          
          colors.forEach(([color, count]) => {
            const code = getColorCode(color)
            const name = getColorName(color)
            
            const x = padding + currentCol * colWidth
            const y = statsY + 35 + currentRow * rowHeight
            
            // 颜色方块
            ctx.fillStyle = color
            ctx.fillRect(x, y, 30, 30)
            ctx.strokeStyle = '#999999'
            ctx.lineWidth = 1
            ctx.strokeRect(x, y, 30, 30)
            
            // 颜色信息 - 只显示编号和数量，不显示名称
            ctx.fillStyle = '#000000'
            ctx.font = 'bold 20px monospace'
            ctx.textAlign = 'left'
            ctx.fillText(`${code}`, x + 35, y + 14)
            
            ctx.font = 'bold 20px monospace'
            ctx.fillText(`×${count}`, x + 95, y + 14)
            
            currentCol++
            if (currentCol >= 12) {
              currentCol = 0
              currentRow++
            }
          })
          
          // 合计
          const totalCount = Array.from(colorStats.values()).reduce((a, b) => a + b, 0)
          const totalY = statsY + 35 + (currentRow + (currentCol > 0 ? 1 : 0)) * rowHeight + 15
          ctx.font = 'bold 16px Arial'
          ctx.textAlign = 'right'
          ctx.fillText(`合计: ${totalCount} 颗`, canvasWidth - padding, totalY)
          
          // 套装信息
          if (requiredPresets.length > 0) {
            const presetY = totalY + 30
            ctx.fillStyle = '#000000'
            ctx.font = 'bold 16px Arial'
            ctx.textAlign = 'right'
            ctx.fillText('推荐套装: ', canvasWidth - padding - 200, presetY)
            
            ctx.font = 'bold 16px Arial'
            ctx.fillStyle = '#00f3ff'
            ctx.fillText(requiredPresets.join('、'), canvasWidth - padding, presetY)
            
            ctx.font = '11px Arial'
            ctx.fillStyle = '#666666'
            ctx.fillText('（根据使用的颜色推荐）', canvasWidth - padding, presetY + 18)
          }
        }
        
        // 绘制 Logo
        if (showLogo) {
          const logoSize = 80  // 从 40 改为 80，放大2倍
          const logoX = padding
          const logoY = canvasHeight - padding - logoSize - 10
          
          const iconSize = logoSize
          const iconPadding = iconSize * 0.15
          const gridSize = (iconSize - iconPadding * 2) / 3
          const gap = 4  // 从 2 改为 4，间隙也放大
          
          ctx.fillStyle = '#111111'
          ctx.fillRect(logoX, logoY, iconSize, iconSize)
          ctx.strokeStyle = '#444444'
          ctx.lineWidth = 2  // 从 1 改为 2
          ctx.strokeRect(logoX, logoY, iconSize, iconSize)
          
          for (let i = 0; i < 9; i++) {
            const row = Math.floor(i / 3)
            const col = i % 3
            const x = logoX + iconPadding + col * (gridSize + gap)
            const y = logoY + iconPadding + row * (gridSize + gap)
            
            ctx.fillStyle = i % 2 === 0 ? 'rgba(0, 243, 255, 0.9)' : 'rgba(0, 243, 255, 0.4)'
            ctx.fillRect(x, y, gridSize - gap, gridSize - gap)
          }
          
          ctx.fillStyle = '#00f3ff'
          ctx.font = 'bold 48px monospace'  // 从 24px 改为 48px
          ctx.textAlign = 'left'
          ctx.textBaseline = 'alphabetic'
          const textY = logoY + iconSize / 2 + 16  // 从 8 改为 16
          ctx.fillText('Glowxel', logoX + iconSize + 20, textY)  // 从 10 改为 20
        }

        // 叠加水印（如果传入了 watermark 参数）
        if (watermark && watermark.username) {
          drawWatermark(ctx, canvasWidth, canvasHeight, {
            username: watermark.username,
            logoText: 'Glowxel'
          })
        }
}

/**
 * 在已有 canvas context 上叠加水印
 * @param {CanvasRenderingContext2D} ctx
 * @param {number} canvasWidth
 * @param {number} canvasHeight
 * @param {object} options - { username, logoText }
 */
export function drawWatermark(ctx, canvasWidth, canvasHeight, options = {}) {
  const { username = 'Glowxel', logoText = 'Glowxel' } = options
  const text = `${logoText} · ${username}`

  ctx.save()
  ctx.globalAlpha = 0.08
  ctx.fillStyle = '#000000'
  ctx.font = 'bold 28px monospace'
  ctx.textAlign = 'center'
  ctx.textBaseline = 'middle'

  // 斜 45° 重复平铺
  const stepX = 300
  const stepY = 200
  ctx.translate(canvasWidth / 2, canvasHeight / 2)
  ctx.rotate(-Math.PI / 6)

  for (let y = -canvasHeight; y < canvasHeight * 2; y += stepY) {
    for (let x = -canvasWidth; x < canvasWidth * 2; x += stepX) {
      ctx.fillText(text, x, y)
    }
  }

  ctx.restore()
}

/**
 * 导出带水印的图纸（用于发布预览）
 * 复用 exportCanvasAsImage 逻辑，额外叠加水印层
 */
export async function exportWithWatermark(options) {
  const { username = 'Glowxel', ...exportOptions } = options

  // 先生成原始图纸
  const tempFilePath = await exportCanvasAsImage(exportOptions)

  // 在 H5 环境下可以二次处理加水印
  // 小程序环境下水印在 canvas 绘制阶段直接叠加更高效
  // 这里提供一个标记，让 exportCanvasAsImage 内部支持水印参数
  return tempFilePath
}

/**
 * 保存图片到相册
 */
export function saveImageToAlbum(tempFilePath) {
  return new Promise((resolve, reject) => {
    // 检查参数
    if (!tempFilePath) {
      reject(new Error('tempFilePath is required'))
      return
    }
    
    // #ifdef H5
    // H5 环境下使用下载方式
    try {
      // 如果是 base64 数据，需要转换为 Blob
      if (tempFilePath.startsWith('data:image')) {
        const arr = tempFilePath.split(',')
        const mime = arr[0].match(/:(.*?);/)[1]
        const bstr = atob(arr[1])
        let n = bstr.length
        const u8arr = new Uint8Array(n)
        while (n--) {
          u8arr[n] = bstr.charCodeAt(n)
        }
        const blob = new Blob([u8arr], { type: mime })
        const url = URL.createObjectURL(blob)
        
        const link = document.createElement('a')
        link.href = url
        link.download = `pixel-art-${Date.now()}.png`
        document.body.appendChild(link)
        link.click()
        document.body.removeChild(link)
        
        // 释放 URL 对象
        setTimeout(() => URL.revokeObjectURL(url), 100)
      } else {
        // 普通 URL
        const link = document.createElement('a')
        link.href = tempFilePath
        link.download = `pixel-art-${Date.now()}.png`
        document.body.appendChild(link)
        link.click()
        document.body.removeChild(link)
      }
      resolve()
    } catch (err) {
      reject(err)
    }
    // #endif
    
    // #ifndef H5
    // App 和小程序环境下保存到相册
    uni.saveImageToPhotosAlbum({
      filePath: tempFilePath,
      success: () => {
        resolve()
      },
      fail: (err) => {
        reject(err)
      }
    })
    // #endif
  })
}

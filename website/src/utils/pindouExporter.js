import jsPDF from 'jspdf'
import html2canvas from 'html2canvas'
import { getColorByCode } from '@/data/artkal-colors.js'
import logoUrl from '@/assets/images/RenLight-logo.png'

/**
 * 导出拼豆图纸为 PDF
 */
export async function exportToPDF(pixels, width, height, options = {}) {
  const {
    format = 'pdf',
    showNumbers = true,
    showStats = true,
    style = 'grid-circle',
    spacing = 'small',
    colors = [],
    usedColors = new Map()
  } = options

  if (format === 'png') {
    return exportToPNG(pixels, width, height, options)
  }

  // 创建 PDF
  const pdf = new jsPDF({
    orientation: width > height ? 'landscape' : 'portrait',
    unit: 'mm',
    format: 'a4'
  })

  const pageWidth = pdf.internal.pageSize.getWidth()
  const pageHeight = pdf.internal.pageSize.getHeight()
  const margin = 20

  // 添加标题
  pdf.setFontSize(20)
  pdf.setFont('helvetica', 'bold')
  pdf.text('拼豆图纸', pageWidth / 2, margin, { align: 'center' })

  // 添加基本信息
  pdf.setFontSize(12)
  pdf.setFont('helvetica', 'normal')
  let yPos = margin + 15
  
  pdf.text(`尺寸: ${width} × ${height}`, margin, yPos)
  yPos += 8
  pdf.text(`颜色数: ${usedColors.size} 种`, margin, yPos)
  yPos += 8
  pdf.text(`拼豆数: ${getTotalBeads(usedColors)} 颗`, margin, yPos)
  yPos += 8
  pdf.text(`生成时间: ${new Date().toLocaleString()}`, margin, yPos)
  yPos += 15

  // 计算网格尺寸
  const availableWidth = pageWidth - margin * 2
  const availableHeight = pageHeight - yPos - margin - (showStats ? 60 : 20)
  
  const cellSize = Math.min(
    availableWidth / width,
    availableHeight / height,
    8 // 最大单元格尺寸
  )

  const gridWidth = width * cellSize
  const gridHeight = height * cellSize
  const gridX = (pageWidth - gridWidth) / 2
  const gridY = yPos

  // 绘制网格和拼豆
  drawPindouGrid(pdf, pixels, width, height, gridX, gridY, cellSize, {
    showNumbers,
    style,
    spacing
  })

  // 添加颜色统计表
  if (showStats && usedColors.size > 0) {
    addColorStatsTable(pdf, usedColors, colors, gridY + gridHeight + 10)
  }

  // 底部 logo + 公司名
  const pageWidth = pdf.internal.pageSize.getWidth()
  const pageHeight = pdf.internal.pageSize.getHeight()
  pdf.setFontSize(9)
  pdf.setTextColor(150, 150, 150)
  pdf.text('RenLight', 20, pageHeight - 8)
  pdf.text(new Date().toLocaleDateString('zh-CN'), 20, pageHeight - 4)
  try {
    const logoImg = await loadLogoBase64()
    if (logoImg) pdf.addImage(logoImg, 'PNG', pageWidth - 35, pageHeight - 14, 25, 8)
  } catch {}

  // 保存 PDF
  const filename = `pindou_pattern_${width}x${height}_${Date.now()}.pdf`
  pdf.save(filename)
}

/**
 * 导出为 PNG 图片
 */
export async function exportToPNG(pixels, width, height, options = {}) {
  const {
    showNumbers = true,
    showStats = true,
    style = 'grid-circle',
    spacing = 'small',
    colors = [],
    usedColors = new Map()
  } = options

  // 创建临时 canvas
  const canvas = document.createElement('canvas')
  const ctx = canvas.getContext('2d')
  
  const cellSize = 20
  const gridSpacing = getSpacingValue(spacing)
  const statsHeight = showStats ? 200 : 0
  
  canvas.width = width * (cellSize + gridSpacing) + 40
  canvas.height = height * (cellSize + gridSpacing) + 100 + statsHeight

  // 绘制背景
  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, canvas.width, canvas.height)

  // 绘制标题
  ctx.fillStyle = '#2d3748'
  ctx.font = 'bold 24px Arial'
  ctx.textAlign = 'center'
  ctx.fillText('拼豆图纸', canvas.width / 2, 30)

  // 绘制基本信息
  ctx.font = '14px Arial'
  ctx.textAlign = 'left'
  ctx.fillText(`尺寸: ${width} × ${height}`, 20, 60)
  ctx.fillText(`颜色数: ${usedColors.size} 种`, 200, 60)
  ctx.fillText(`拼豆数: ${getTotalBeads(usedColors)} 颗`, 350, 60)

  // 绘制网格
  const gridX = 20
  const gridY = 80
  
  drawPindouGridCanvas(ctx, pixels, width, height, gridX, gridY, cellSize, {
    showNumbers,
    style,
    spacing: gridSpacing
  })

  // 绘制统计信息
  if (showStats && usedColors.size > 0) {
    drawColorStatsCanvas(ctx, usedColors, colors, gridY + height * (cellSize + gridSpacing) + 20)
  }

  // 绘制 logo 和公司名（底部）
  try {
    const logo = await loadImage(logoUrl)
    const lh = 28
    const lw = logo.width * (lh / logo.height)
    ctx.drawImage(logo, canvas.width - lw - 20, canvas.height - lh - 10, lw, lh)
  } catch {}
  ctx.fillStyle = '#999'
  ctx.font = '13px Arial'
  ctx.textAlign = 'left'
  ctx.textBaseline = 'alphabetic'
  ctx.fillText('RenLight', 20, canvas.height - 12)
  ctx.fillText(new Date().toLocaleDateString('zh-CN'), 20, canvas.height - 28)

  // 转换为 blob 并下载
  return new Promise((resolve) => {
    canvas.toBlob((blob) => {
      const url = URL.createObjectURL(blob)
      const link = document.createElement('a')
      link.href = url
      link.download = `pindou_pattern_${width}x${height}_${Date.now()}.png`
      link.click()
      URL.revokeObjectURL(url)
      resolve(url)
    })
  })
}

/**
 * 在 PDF 中绘制拼豆网格
 */
function drawPindouGrid(pdf, pixels, width, height, startX, startY, cellSize, options) {
  const { showNumbers, style, spacing } = options
  const spacingValue = getSpacingValue(spacing) * 0.1 // PDF 中的间距更小

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const key = `${x},${y}`
      const colorCode = pixels.get(key)
      
      if (colorCode) {
        const color = getColorByCode(colorCode)
        if (color) {
          const cellX = startX + x * (cellSize + spacingValue)
          const cellY = startY + y * (cellSize + spacingValue)
          
          // 设置颜色
          pdf.setFillColor(color.hex)
          
          // 绘制拼豆形状
          if (style === 'grid-circle') {
            pdf.circle(cellX + cellSize / 2, cellY + cellSize / 2, cellSize / 2, 'F')
          } else if (style === 'grid-hollow') {
            pdf.circle(cellX + cellSize / 2, cellY + cellSize / 2, cellSize / 2, 'S')
            pdf.circle(cellX + cellSize / 2, cellY + cellSize / 2, cellSize / 4, 'S')
          } else {
            pdf.rect(cellX, cellY, cellSize, cellSize, 'F')
          }
          
          // 绘制编号
          if (showNumbers && cellSize > 3) {
            pdf.setTextColor(0, 0, 0)
            pdf.setFontSize(Math.min(cellSize * 0.3, 8))
            pdf.text(colorCode, cellX + cellSize / 2, cellY + cellSize / 2, { align: 'center' })
          }
        }
      }
      
      // 绘制网格线
      pdf.setDrawColor(200, 200, 200)
      pdf.setLineWidth(0.1)
      const cellX = startX + x * (cellSize + spacingValue)
      const cellY = startY + y * (cellSize + spacingValue)
      pdf.rect(cellX, cellY, cellSize, cellSize, 'S')
    }
  }
}

/**
 * 在 Canvas 中绘制拼豆网格
 */
function drawPindouGridCanvas(ctx, pixels, width, height, startX, startY, cellSize, options) {
  const { showNumbers, style, spacing } = options

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const key = `${x},${y}`
      const colorCode = pixels.get(key)
      
      const cellX = startX + x * (cellSize + spacing)
      const cellY = startY + y * (cellSize + spacing)
      
      // 绘制网格背景
      ctx.strokeStyle = '#e2e8f0'
      ctx.lineWidth = 1
      ctx.strokeRect(cellX, cellY, cellSize, cellSize)
      
      if (colorCode) {
        const color = getColorByCode(colorCode)
        if (color) {
          ctx.fillStyle = color.hex
          
          // 绘制拼豆形状
          if (style === 'grid-circle') {
            ctx.beginPath()
            ctx.arc(cellX + cellSize / 2, cellY + cellSize / 2, cellSize * 0.4, 0, Math.PI * 2)
            ctx.fill()
          } else if (style === 'grid-hollow') {
            ctx.beginPath()
            ctx.arc(cellX + cellSize / 2, cellY + cellSize / 2, cellSize * 0.4, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(cellX + cellSize / 2, cellY + cellSize / 2, cellSize * 0.2, 0, Math.PI * 2)
            ctx.stroke()
          } else {
            ctx.fillRect(cellX + 1, cellY + 1, cellSize - 2, cellSize - 2)
          }
          
          // 绘制编号
          if (showNumbers && cellSize > 15) {
            ctx.fillStyle = '#2d3748'
            ctx.font = `${Math.min(cellSize * 0.3, 10)}px Arial`
            ctx.textAlign = 'center'
            ctx.textBaseline = 'middle'
            ctx.fillText(colorCode, cellX + cellSize / 2, cellY + cellSize / 2)
          }
        }
      }
    }
  }
}

/**
 * 添加颜色统计表到 PDF
 */
function addColorStatsTable(pdf, usedColors, colors, startY) {
  pdf.setFontSize(14)
  pdf.setFont('helvetica', 'bold')
  pdf.text('颜色统计表', 20, startY)
  
  let yPos = startY + 10
  const colWidth = 40
  const rowHeight = 6
  
  // 表头
  pdf.setFontSize(10)
  pdf.setFont('helvetica', 'bold')
  pdf.text('编号', 20, yPos)
  pdf.text('颜色', 20 + colWidth, yPos)
  pdf.text('名称', 20 + colWidth * 2, yPos)
  pdf.text('数量', 20 + colWidth * 3, yPos)
  
  yPos += rowHeight
  
  // 绘制表格线
  pdf.setDrawColor(0, 0, 0)
  pdf.setLineWidth(0.2)
  pdf.line(20, yPos - 2, 20 + colWidth * 4, yPos - 2)
  
  // 表格内容
  pdf.setFont('helvetica', 'normal')
  
  Array.from(usedColors.entries())
    .sort((a, b) => b[1] - a[1]) // 按数量排序
    .forEach(([colorCode, count]) => {
      const color = getColorByCode(colorCode)
      if (color) {
        pdf.text(colorCode, 20, yPos)
        
        // 绘制颜色方块
        pdf.setFillColor(color.hex)
        pdf.rect(20 + colWidth - 5, yPos - 3, 4, 4, 'F')
        
        pdf.setFillColor(0, 0, 0)
        pdf.text(color.name, 20 + colWidth * 2, yPos)
        pdf.text(count.toString(), 20 + colWidth * 3, yPos)
        
        yPos += rowHeight
      }
    })
}

/**
 * 在 Canvas 中绘制颜色统计
 */
function drawColorStatsCanvas(ctx, usedColors, colors, startY) {
  ctx.fillStyle = '#2d3748'
  ctx.font = 'bold 16px Arial'
  ctx.textAlign = 'left'
  ctx.fillText('颜色统计表', 20, startY)
  
  let yPos = startY + 30
  const colWidth = 120
  const rowHeight = 25
  
  // 表头
  ctx.font = 'bold 12px Arial'
  ctx.fillText('编号', 20, yPos)
  ctx.fillText('颜色', 20 + colWidth, yPos)
  ctx.fillText('名称', 20 + colWidth * 2, yPos)
  ctx.fillText('数量', 20 + colWidth * 3, yPos)
  
  yPos += rowHeight
  
  // 表格内容
  ctx.font = '11px Arial'
  
  Array.from(usedColors.entries())
    .sort((a, b) => b[1] - a[1])
    .slice(0, 6) // 只显示前6种颜色
    .forEach(([colorCode, count]) => {
      const color = getColorByCode(colorCode)
      if (color) {
        ctx.fillText(colorCode, 20, yPos)
        
        // 绘制颜色方块
        ctx.fillStyle = color.hex
        ctx.fillRect(20 + colWidth - 10, yPos - 10, 15, 15)
        ctx.strokeStyle = '#ccc'
        ctx.strokeRect(20 + colWidth - 10, yPos - 10, 15, 15)
        
        ctx.fillStyle = '#2d3748'
        ctx.fillText(color.name, 20 + colWidth * 2, yPos)
        ctx.fillText(count.toString(), 20 + colWidth * 3, yPos)
        
        yPos += rowHeight
      }
    })
}

/**
 * 获取间距数值
 */
function getSpacingValue(spacing) {
  switch (spacing) {
    case 'none': return 0
    case 'small': return 1
    case 'large': return 3
    default: return 1
  }
}

/**
 * 计算总拼豆数
 */
function getTotalBeads(usedColors) {
  let total = 0
  usedColors.forEach(count => {
    total += count
  })
}

function loadImage(src) {
  return new Promise((resolve, reject) => {
    const img = new Image()
    img.onload = () => resolve(img)
    img.onerror = reject
    img.src = src
  })
}

async function loadLogoBase64() {
  try {
    const img = await loadImage(logoUrl)
    const c = document.createElement('canvas')
    c.width = img.width; c.height = img.height
    c.getContext('2d').drawImage(img, 0, 0)
    return c.toDataURL('image/png')
  } catch { return null }
}
  return total
}

/**
 * 下载图片
 */
export function downloadImage(dataUrl, filename) {
  const link = document.createElement('a')
  link.href = dataUrl
  link.download = filename
  link.click()
}
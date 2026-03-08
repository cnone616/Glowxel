// 闹钟 Canvas 绘制工具
// 5x7 点阵字体数据
export const font5x7 = {
  '0': [0x3E, 0x51, 0x49, 0x45, 0x3E],
  '1': [0x00, 0x42, 0x7F, 0x40, 0x00],
  '2': [0x42, 0x61, 0x51, 0x49, 0x46],
  '3': [0x21, 0x41, 0x45, 0x4B, 0x31],
  '4': [0x18, 0x14, 0x12, 0x7F, 0x10],
  '5': [0x27, 0x45, 0x45, 0x45, 0x39],
  '6': [0x3C, 0x4A, 0x49, 0x49, 0x30],
  '7': [0x01, 0x71, 0x09, 0x05, 0x03],
  '8': [0x36, 0x49, 0x49, 0x49, 0x36],
  '9': [0x06, 0x49, 0x49, 0x29, 0x1E],
  ':': [0x00, 0x36, 0x36, 0x00, 0x00],
  '-': [0x08, 0x08, 0x08, 0x08, 0x08],
  '/': [0x20, 0x10, 0x08, 0x04, 0x02],
  ' ': [0x00, 0x00, 0x00, 0x00, 0x00],
  
  // 中文字符（简化版）
  '月': [0x7F, 0x49, 0x49, 0x49, 0x7F],
  '日': [0x7F, 0x41, 0x41, 0x41, 0x7F],
  '一': [0x00, 0x00, 0x7F, 0x00, 0x00],
  '二': [0x00, 0x7F, 0x00, 0x7F, 0x00],
  '三': [0x7F, 0x00, 0x7F, 0x00, 0x7F],
  '四': [0x7F, 0x49, 0x49, 0x49, 0x7F],
  '五': [0x7F, 0x09, 0x09, 0x09, 0x7F],
  '六': [0x7F, 0x09, 0x7F, 0x49, 0x7F],
  '周': [0x7F, 0x49, 0x49, 0x49, 0x7F]
}

// Hex 转 RGB
export function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : { r: 255, g: 255, b: 255 }
}

// 绘制单个字符到像素 Map
export function drawCharToPixels(char, x, y, color, size, pixelMap) {
  const fontData = font5x7[char]
  if (!fontData) return 5 * size
  
  for (let col = 0; col < 5; col++) {
    for (let row = 0; row < 8; row++) {
      if (fontData[col] & (1 << row)) {
        for (let sy = 0; sy < size; sy++) {
          for (let sx = 0; sx < size; sx++) {
            const px = x + col * size + sx
            const py = y + row * size + sy
            if (px >= 0 && px < 64 && py >= 0 && py < 64) {
              pixelMap.set(`${px},${py}`, color)
            }
          }
        }
      }
    }
  }
  
  return (5 + 1) * size
}

// 计算文本宽度（像素数）
export function getTextWidth(text, size) {
  let width = 0
  for (let i = 0; i < text.length; i++) {
    const fontData = font5x7[text[i]]
    if (fontData) {
      width += (5 + 1) * size // 每个字符宽度 5 + 间距 1
    }
  }
  return width
}

// 绘制文本到像素 Map（支持对齐）
export function drawTextToPixels(text, x, y, color, size, pixelMap, align = 'left') {
  // 计算文本总宽度
  const totalWidth = getTextWidth(text, size)
  
  // 根据对齐方式调整起始 x 坐标
  let startX = x
  if (align === 'center') {
    // 居中：x 是中心点，向左偏移一半宽度
    startX = x - Math.floor(totalWidth / 2)
  } else if (align === 'right') {
    // 右对齐：x 是右边界，向左偏移整个宽度
    startX = x - totalWidth
  }
  // 左对齐：x 就是起始点，不需要调整
  
  // 绘制文本
  let offsetX = 0
  for (let i = 0; i < text.length; i++) {
    offsetX += drawCharToPixels(text[i], startX + offsetX, y, color, size, pixelMap)
  }
  return offsetX
}

// 获取当前时间文本
export function getCurrentTimeText() {
  const now = new Date()
  const hours = String(now.getHours()).padStart(2, '0')
  const minutes = String(now.getMinutes()).padStart(2, '0')
  return `${hours}:${minutes}`
}

// 获取当前日期文本
export function getCurrentDateText() {
  const now = new Date()
  const month = String(now.getMonth() + 1).padStart(2, '0')
  const date = String(now.getDate()).padStart(2, '0')
  return `${month}/${date}`
}

// 获取当前星期文本（英文缩写）
export function getCurrentWeekText() {
  const weekDays = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat']
  const now = new Date()
  return weekDays[now.getDay()]
}

// 将图片绘制到像素 Map
export function drawImageToPixels(imageData, x, y, width, height, pixelMap) {
  // imageData 是 base64 格式的图片
  // 这里需要在 canvas 中处理，返回 Promise
  return new Promise((resolve, reject) => {
    // 这个函数需要在页面中调用，因为需要访问 canvas context
    resolve()
  })
}

// LED 矩阵渲染工具
export const PANEL_WIDTH = 64
export const PANEL_HEIGHT = 64
export const PIXEL_SIZE = 10

// 5x7 点阵字体
export const font5x7 = {
  '0': [0x3E, 0x51, 0x49, 0x45, 0x3E], '1': [0x00, 0x42, 0x7F, 0x40, 0x00],
  '2': [0x42, 0x61, 0x51, 0x49, 0x46], '3': [0x21, 0x41, 0x45, 0x4B, 0x31],
  '4': [0x18, 0x14, 0x12, 0x7F, 0x10], '5': [0x27, 0x45, 0x45, 0x45, 0x39],
  '6': [0x3C, 0x4A, 0x49, 0x49, 0x30], '7': [0x01, 0x71, 0x09, 0x05, 0x03],
  '8': [0x36, 0x49, 0x49, 0x49, 0x36], '9': [0x06, 0x49, 0x49, 0x29, 0x1E],
  ':': [0x00, 0x36, 0x36, 0x00, 0x00], '-': [0x08, 0x08, 0x08, 0x08, 0x08],
  'S': [0x32, 0x49, 0x49, 0x49, 0x26], 'u': [0x3C, 0x40, 0x40, 0x20, 0x7C],
  'n': [0x7C, 0x08, 0x04, 0x04, 0x78], 'M': [0x7F, 0x02, 0x0C, 0x02, 0x7F],
  'o': [0x38, 0x44, 0x44, 0x44, 0x38], 'T': [0x01, 0x01, 0x7F, 0x01, 0x01],
  'W': [0x7F, 0x20, 0x18, 0x20, 0x7F], 'e': [0x38, 0x54, 0x54, 0x54, 0x18],
  'h': [0x7F, 0x08, 0x04, 0x04, 0x78], 'F': [0x7F, 0x09, 0x09, 0x09, 0x01],
  'r': [0x7C, 0x08, 0x04, 0x04, 0x08], 'i': [0x00, 0x44, 0x7D, 0x40, 0x00],
  'a': [0x20, 0x54, 0x54, 0x54, 0x78], 't': [0x04, 0x3F, 0x44, 0x40, 0x20]
}

// 清空画布并绘制网格
export function clearCanvas(ctx, width, height) {
  ctx.fillStyle = '#000'
  ctx.fillRect(0, 0, width, height)
  
  // 细网格
  ctx.strokeStyle = 'rgba(40, 40, 40, 0.5)'
  ctx.lineWidth = 0.5
  for (let x = 0; x <= PANEL_WIDTH; x++) {
    ctx.beginPath()
    ctx.moveTo(x * PIXEL_SIZE + 0.5, 0)
    ctx.lineTo(x * PIXEL_SIZE + 0.5, PANEL_HEIGHT * PIXEL_SIZE)
    ctx.stroke()
  }
  for (let y = 0; y <= PANEL_HEIGHT; y++) {
    ctx.beginPath()
    ctx.moveTo(0, y * PIXEL_SIZE + 0.5)
    ctx.lineTo(PANEL_WIDTH * PIXEL_SIZE, y * PIXEL_SIZE + 0.5)
    ctx.stroke()
  }
  
  // 粗网格（每8像素）
  ctx.strokeStyle = 'rgba(0, 243, 255, 0.4)'
  ctx.lineWidth = 1
  for (let x = 0; x <= PANEL_WIDTH; x += 8) {
    ctx.beginPath()
    ctx.moveTo(x * PIXEL_SIZE + 0.5, 0)
    ctx.lineTo(x * PIXEL_SIZE + 0.5, PANEL_HEIGHT * PIXEL_SIZE)
    ctx.stroke()
  }
  for (let y = 0; y <= PANEL_HEIGHT; y += 8) {
    ctx.beginPath()
    ctx.moveTo(0, y * PIXEL_SIZE + 0.5)
    ctx.lineTo(PANEL_WIDTH * PIXEL_SIZE, y * PIXEL_SIZE + 0.5)
    ctx.stroke()
  }
}

// 绘制单个像素
export function drawPixel(ctx, x, y, r, g, b) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) return
  ctx.fillStyle = `rgb(${r}, ${g}, ${b})`
  ctx.fillRect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE)
}

// 绘制字符
export function drawChar(ctx, char, x, y, r, g, b, size = 1) {
  const fontData = font5x7[char]
  if (!fontData) return 5 * size
  
  for (let col = 0; col < 5; col++) {
    for (let row = 0; row < 8; row++) {
      if (fontData[col] & (1 << row)) {
        for (let sy = 0; sy < size; sy++) {
          for (let sx = 0; sx < size; sx++) {
            drawPixel(ctx, x + col * size + sx, y + row * size + sy, r, g, b)
          }
        }
      }
    }
  }
  return (5 + 1) * size
}

// 绘制文本
export function drawText(ctx, text, x, y, r, g, b, size = 1) {
  let offsetX = 0
  for (let i = 0; i < text.length; i++) {
    offsetX += drawChar(ctx, text[i], x + offsetX, y, r, g, b, size)
  }
}

// 颜色转换
export function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
  return result ? [
    parseInt(result[1], 16),
    parseInt(result[2], 16),
    parseInt(result[3], 16)
  ] : [255, 255, 255]
}

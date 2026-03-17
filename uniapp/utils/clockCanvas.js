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
  
  // 英文字母（大写）
  'A': [0x7E, 0x09, 0x09, 0x09, 0x7E],
  'B': [0x7F, 0x49, 0x49, 0x49, 0x36],
  'C': [0x3E, 0x41, 0x41, 0x41, 0x22],
  'D': [0x7F, 0x41, 0x41, 0x22, 0x1C],
  'E': [0x7F, 0x49, 0x49, 0x49, 0x41],
  'F': [0x7F, 0x09, 0x09, 0x09, 0x01],
  'G': [0x3E, 0x41, 0x49, 0x49, 0x7A],
  'H': [0x7F, 0x08, 0x08, 0x08, 0x7F],
  'I': [0x00, 0x41, 0x7F, 0x41, 0x00],
  'J': [0x20, 0x40, 0x41, 0x3F, 0x01],
  'K': [0x7F, 0x08, 0x14, 0x22, 0x41],
  'L': [0x7F, 0x40, 0x40, 0x40, 0x40],
  'M': [0x7F, 0x02, 0x0C, 0x02, 0x7F],
  'N': [0x7F, 0x04, 0x08, 0x10, 0x7F],
  'O': [0x3E, 0x41, 0x41, 0x41, 0x3E],
  'P': [0x7F, 0x09, 0x09, 0x09, 0x06],
  'Q': [0x3E, 0x41, 0x51, 0x21, 0x5E],
  'R': [0x7F, 0x09, 0x19, 0x29, 0x46],
  'S': [0x46, 0x49, 0x49, 0x49, 0x31],
  'T': [0x01, 0x01, 0x7F, 0x01, 0x01],
  'U': [0x3F, 0x40, 0x40, 0x40, 0x3F],
  'V': [0x1F, 0x20, 0x40, 0x20, 0x1F],
  'W': [0x3F, 0x40, 0x38, 0x40, 0x3F],
  'X': [0x63, 0x14, 0x08, 0x14, 0x63],
  'Y': [0x07, 0x08, 0x70, 0x08, 0x07],
  'Z': [0x61, 0x51, 0x49, 0x45, 0x43],
  
  // 英文字母（小写）
  'a': [0x20, 0x54, 0x54, 0x54, 0x78],
  'b': [0x7F, 0x48, 0x44, 0x44, 0x38],
  'c': [0x38, 0x44, 0x44, 0x44, 0x20],
  'd': [0x38, 0x44, 0x44, 0x48, 0x7F],
  'e': [0x38, 0x54, 0x54, 0x54, 0x18],
  'f': [0x08, 0x7E, 0x09, 0x01, 0x02],
  'g': [0x0C, 0x52, 0x52, 0x52, 0x3E],
  'h': [0x7F, 0x08, 0x04, 0x04, 0x78],
  'i': [0x00, 0x44, 0x7D, 0x40, 0x00],
  'j': [0x20, 0x40, 0x44, 0x3D, 0x00],
  'k': [0x7F, 0x10, 0x28, 0x44, 0x00],
  'l': [0x00, 0x41, 0x7F, 0x40, 0x00],
  'm': [0x7C, 0x04, 0x18, 0x04, 0x78],
  'n': [0x7C, 0x08, 0x04, 0x04, 0x78],
  'o': [0x38, 0x44, 0x44, 0x44, 0x38],
  'p': [0x7C, 0x14, 0x14, 0x14, 0x08],
  'q': [0x08, 0x14, 0x14, 0x18, 0x7C],
  'r': [0x7C, 0x08, 0x04, 0x04, 0x08],
  's': [0x48, 0x54, 0x54, 0x54, 0x20],
  't': [0x04, 0x3F, 0x44, 0x40, 0x20],
  'u': [0x3C, 0x40, 0x40, 0x20, 0x7C],
  'v': [0x1C, 0x20, 0x40, 0x20, 0x1C],
  'w': [0x3C, 0x40, 0x30, 0x40, 0x3C],
  'x': [0x44, 0x28, 0x10, 0x28, 0x44],
  'y': [0x0C, 0x50, 0x50, 0x50, 0x3C],
  'z': [0x44, 0x64, 0x54, 0x4C, 0x44],
  
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

// 3x5 点阵字体（与 ESP32 FONT3X5 完全一致）
// 每字符5行，每行3bit: bit2=左列, bit1=中列, bit0=右列
export const font3x5 = {
  '0':[7,5,5,5,7],'1':[2,6,2,2,7],'2':[7,1,7,4,7],'3':[7,1,7,1,7],
  '4':[5,5,7,1,1],'5':[7,4,7,1,7],'6':[7,4,7,5,7],'7':[7,1,1,1,1],
  '8':[7,5,7,5,7],'9':[7,5,7,1,7],
  '.':[0,0,0,0,2],':':[0,2,0,2,0],'-':[0,0,7,0,0],' ':[0,0,0,0,0],
  'A':[2,5,7,5,5],'B':[6,5,6,5,6],'C':[7,4,4,4,7],'D':[6,5,5,5,6],
  'E':[7,4,7,4,7],'F':[7,4,7,4,4],'G':[7,4,5,5,7],'H':[5,5,7,5,5],
  'I':[7,2,2,2,7],'J':[3,1,1,5,7],'K':[5,6,4,6,5],'L':[4,4,4,4,7],
  'M':[5,7,5,5,5],'N':[5,7,7,5,5],'O':[7,5,5,5,7],'P':[7,5,7,4,4],
  'Q':[7,5,5,7,1],'R':[7,5,7,6,5],'S':[7,4,7,1,7],'T':[7,2,2,2,2],
  'U':[5,5,5,5,7],'V':[5,5,5,2,2],'W':[5,5,5,7,5],'X':[5,5,2,5,5],
  'Y':[5,5,2,2,2],'Z':[7,1,2,4,7],
  'a':[0,7,5,5,7],'b':[4,6,5,5,6],'c':[0,7,4,4,7],'d':[1,3,5,5,3],
  'e':[0,7,5,7,4],'f':[3,4,7,4,4],'g':[7,5,7,1,7],'h':[4,4,6,5,5],
  'i':[2,0,2,2,2],'k':[4,5,6,5,5],'l':[6,2,2,2,7],'m':[0,5,7,5,5],
  'n':[0,6,5,5,5],'o':[0,7,5,5,7],'p':[7,5,7,4,4],'r':[0,7,4,4,4],
  's':[7,4,7,1,7],'t':[4,7,4,4,3],'u':[0,5,5,5,7],'w':[0,5,5,7,5],
  'x':[0,5,2,2,5],'y':[5,5,7,1,7]
}

// 绘制单个字符到像素 Map（3x5 字体，与 ESP32 drawTinyText 一致）
export function drawTinyCharToPixels(char, x, y, color, pixelMap, size = 1) {
  const glyph = font3x5[char]
  if (!glyph) return 4 * size
  for (let row = 0; row < 5; row++) {
    const bits = glyph[row]
    for (let sy = 0; sy < size; sy++) {
      for (let sx = 0; sx < size; sx++) {
        if (bits & 4) { const px = x + sx,       py = y + row * size + sy; if (px >= 0 && px < 64 && py >= 0 && py < 64) pixelMap.set(`${px},${py}`, color) }
        if (bits & 2) { const px = x + size + sx, py = y + row * size + sy; if (px >= 0 && px < 64 && py >= 0 && py < 64) pixelMap.set(`${px},${py}`, color) }
        if (bits & 1) { const px = x + size*2 + sx, py = y + row * size + sy; if (px >= 0 && px < 64 && py >= 0 && py < 64) pixelMap.set(`${px},${py}`, color) }
      }
    }
  }
  return (3 * size) + size  // 3列×size + 1px间距×size
}

// 绘制文本到像素 Map（3x5 字体，支持 size 缩放和对齐）
export function drawTinyTextToPixels(text, x, y, color, pixelMap, size = 1, align = 'left') {
  const totalWidth = getTinyTextWidth(text, size)
  let startX = x
  if (align === 'center') startX = x - Math.floor(totalWidth / 2)
  else if (align === 'right') startX = x - totalWidth
  let cx = startX
  for (let i = 0; i < text.length; i++) {
    cx += drawTinyCharToPixels(text[i], cx, y, color, pixelMap, size)
  }
}

// 计算 3x5 文本宽度（支持 size 缩放）
export function getTinyTextWidth(text, size = 1) {
  if (!text || text.length === 0) return 0
  return text.length * 4 * size - size  // 每字符(3+1)×size，最后一个字符不加间距
}
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

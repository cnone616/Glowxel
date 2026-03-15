// 221色调色板 - 基于常见的 RGB 色彩空间
// 生成规则：R、G、B 各取 0, 51, 102, 153, 204, 255 (6个值)
// 6 * 6 * 6 = 216 色 + 5 个灰度色 = 221 色

export function generate221Colors() {
  const colors = []
  const values = [0, 51, 102, 153, 204, 255]
  
  // 生成 216 色
  for (let r of values) {
    for (let g of values) {
      for (let b of values) {
        const hex = rgbToHex(r, g, b)
        colors.push({
          hex,
          r,
          g,
          b
        })
      }
    }
  }
  
  // 添加额外的灰度色
  const grays = [
    { r: 17, g: 17, b: 17 },
    { r: 34, g: 34, b: 34 },
    { r: 68, g: 68, b: 68 },
    { r: 85, g: 85, b: 85 },
    { r: 119, g: 119, b: 119 }
  ]
  
  grays.forEach(gray => {
    colors.push({
      hex: rgbToHex(gray.r, gray.g, gray.b),
      ...gray
    })
  })
  
  return colors
}

function rgbToHex(r, g, b) {
  return '#' + [r, g, b].map(x => {
    const hex = x.toString(16)
    return hex.length === 1 ? '0' + hex : hex
  }).join('')
}

// 导出预生成的调色板
export const COLOR_PALETTE_221 = generate221Colors()

// 常用颜色快捷选择
export const COMMON_COLORS = [
  { name: '黑色', hex: '#000000' },
  { name: '白色', hex: '#ffffff' },
  { name: '红色', hex: '#ff0000' },
  { name: '绿色', hex: '#00ff00' },
  { name: '蓝色', hex: '#0000ff' },
  { name: '黄色', hex: '#ffff00' },
  { name: '青色', hex: '#00ffff' },
  { name: '洋红', hex: '#ff00ff' },
  { name: '橙色', hex: '#ff9900' },
  { name: '紫色', hex: '#9900ff' },
  { name: '粉色', hex: '#ff66cc' },
  { name: '灰色', hex: '#888888' }
]

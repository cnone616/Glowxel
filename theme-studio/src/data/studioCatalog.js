import {
  buildAnimatedFrames,
  buildStaticFrames,
  drawFrame,
  drawHorizontal,
  drawPoints,
  drawRect,
  drawVertical,
  setPixel,
} from '../lib/pixelArt'
import { COLOR_PALETTES } from '../lib/studioSchema'

function pulseValue(index, frameCount, values) {
  return values[index % Math.min(frameCount, values.length)]
}

function retroDigitalTemplate() {
  const frameCount = 8
  const amber = [255, 172, 77]
  const glow = [255, 210, 117]
  const teal = [47, 212, 191]

  return {
    key: 'retro-digital',
    name: '复古电子',
    strap: '琥珀数字窗 + 跳动刻度',
    description: '适合做老式桌面电子钟主题，时间层覆盖在温暖的数字舱上。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 2, 2, 60, 60, [42, 30, 24])
        drawFrame(map, 5, 5, 54, 54, [92, 54, 33])
        drawRect(map, 7, 8, 50, 19, [24, 18, 13])
        drawHorizontal(map, 8, 30, 48, [50, 35, 27])
        drawHorizontal(map, 8, 44, 48, [50, 35, 27])
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const heights = [
          [7, 11, 15, 10, 6, 9],
          [10, 14, 18, 13, 8, 11],
          [14, 17, 20, 16, 10, 13],
          [11, 15, 18, 12, 8, 10],
          [8, 12, 16, 10, 7, 8],
          [6, 11, 14, 9, 6, 7],
          [9, 13, 17, 12, 9, 11],
          [12, 16, 19, 14, 9, 12],
        ]
        const bars = heights[frameIndex]
        for (let i = 0; i < bars.length; i += 1) {
          drawRect(map, 11 + i * 7, 48 - bars[i], 4, bars[i], amber)
        }
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const blink = frameIndex % 2 === 0 ? glow : teal
        drawPoints(
          map,
          [
            [9, 11],
            [54, 11],
            [9, 24],
            [54, 24],
            [9, 47],
            [54, 47],
          ],
          blink,
        )
      })

      return {
        paletteKey: 'amber-terminal',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 10, y: 11, font: 2, color: { r: 255, g: 210, b: 117 } },
          date: { visible: true, x: 11, y: 32, font: 1, color: { r: 255, g: 172, b: 77 } },
          week: { visible: true, x: 11, y: 39, font: 1, color: { r: 47, g: 212, b: 191 } },
          showSeconds: true,
          hourFormat: 24,
        },
      }
    },
  }
}

function neonHudTemplate() {
  const frameCount = 10
  const cyan = [0, 245, 212]
  const sky = [0, 187, 249]
  const amber = [254, 228, 64]

  return {
    key: 'neon-hud',
    name: '霓虹 HUD',
    strap: '扫描线 + 雷达边框',
    description: '未来感 HUD 风格，适合夜间桌面氛围。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 3, 3, 58, 58, [16, 39, 48])
        drawFrame(map, 6, 6, 52, 52, cyan)
        drawHorizontal(map, 10, 20, 44, [18, 83, 94])
        drawHorizontal(map, 10, 42, 44, [18, 83, 94])
        drawVertical(map, 32, 9, 46, [18, 83, 94])
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const radarY = 12 + frameIndex * 4
        drawHorizontal(map, 10, radarY, 44, sky)
        drawRect(map, 14 + (frameIndex % 5) * 6, 46, 5, 5, amber)
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const pulse = frameIndex % 3 === 0 ? amber : cyan
        drawPoints(
          map,
          [
            [8, 8],
            [55, 8],
            [8, 55],
            [55, 55],
            [20, 8],
            [43, 55],
          ],
          pulse,
        )
      })

      return {
        paletteKey: 'arcade-neon',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 13, y: 23, font: 3, color: { r: 214, g: 246, b: 255 } },
          date: { visible: true, x: 13, y: 31, font: 1, color: { r: 123, g: 223, b: 242 } },
          week: { visible: true, x: 13, y: 38, font: 1, color: { r: 254, g: 228, b: 64 } },
          showSeconds: false,
          hourFormat: 24,
        },
      }
    },
  }
}

function pixelPetTemplate() {
  const frameCount = 8
  const pet = [255, 191, 105]
  const fur = [255, 159, 28]
  const mint = [203, 243, 240]

  return {
    key: 'pixel-pet',
    name: '像素宠物',
    strap: '眨眼伙伴 + 柔和舱室',
    description: '中央宠物会眨眼和抬爪，适合陪伴型主题。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 4, 4, 56, 56, [55, 38, 24])
        drawRect(map, 8, 8, 48, 48, [20, 14, 10])
        drawHorizontal(map, 10, 45, 44, [65, 45, 29])
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        drawRect(map, 23, 27, 18, 12, pet)
        drawRect(map, 25, 24, 14, 4, pet)
        drawRect(map, 24, 21, 4, 3, fur)
        drawRect(map, 36, 21, 4, 3, fur)
        if (frameIndex === 2 || frameIndex === 3) {
          drawHorizontal(map, 28, 28, 3, [18, 10, 8])
          drawHorizontal(map, 34, 28, 3, [18, 10, 8])
        } else {
          drawRect(map, 28, 27, 2, 3, [18, 10, 8])
          drawRect(map, 34, 27, 2, 3, [18, 10, 8])
        }
        if (frameIndex >= 4 && frameIndex <= 6) {
          drawRect(map, 22, 36, 4, 3, fur)
        } else {
          drawRect(map, 38, 36, 4, 3, fur)
        }
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        if (frameIndex % 2 === 0) {
          drawPoints(map, [[19, 22], [44, 22]], mint)
        }
        if (frameIndex >= 5) {
          drawPoints(map, [[17, 18], [46, 18]], [255, 111, 89])
        }
      })

      return {
        paletteKey: 'amber-terminal',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 14, y: 9, font: 2, color: { r: 255, g: 191, b: 105 } },
          date: { visible: true, x: 14, y: 47, font: 1, color: { r: 203, g: 243, b: 240 } },
          week: { visible: false, x: 14, y: 54, font: 1, color: { r: 255, g: 159, b: 28 } },
          showSeconds: false,
          hourFormat: 12,
        },
      }
    },
  }
}

function weatherPulseTemplate() {
  const frameCount = 9
  const cloud = [214, 246, 255]
  const rain = [123, 223, 242]
  const sun = [249, 199, 79]

  return {
    key: 'weather-pulse',
    name: '天气脉冲',
    strap: '云团呼吸 + 阳光光环',
    description: '柔和天气动画，适合白天和工作台主题。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 6, 6, 52, 52, [38, 56, 71])
        drawRect(map, 11, 13, 42, 16, [16, 30, 41])
        drawRect(map, 12, 41, 40, 10, [16, 30, 41])
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const offset = pulseValue(frameIndex, frameCount, [0, 1, 2, 1, 0, -1, -2, -1, 0])
        drawRect(map, 18, 20 + offset, 20, 7, cloud)
        drawRect(map, 24, 16 + offset, 12, 7, cloud)
        drawRect(map, 33, 19 + offset, 8, 5, cloud)
        if (frameIndex % 3 === 0) {
          drawVertical(map, 25, 31, 7, rain)
          drawVertical(map, 31, 32, 7, rain)
          drawVertical(map, 37, 31, 7, rain)
        } else {
          drawRect(map, 39, 16, 6, 6, sun)
        }
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const haloSize = 2 + (frameIndex % 3)
        drawFrame(map, 38 - haloSize, 15 - haloSize, 8 + haloSize * 2, 8 + haloSize * 2, sun)
      })

      return {
        paletteKey: 'calm-signal',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 14, y: 47, font: 2, color: { r: 214, g: 246, b: 255 } },
          date: { visible: true, x: 14, y: 10, font: 1, color: { r: 249, g: 199, b: 79 } },
          week: { visible: true, x: 14, y: 17, font: 1, color: { r: 123, g: 223, b: 242 } },
          showSeconds: true,
          hourFormat: 24,
        },
      }
    },
  }
}

function festiveMarqueeTemplate() {
  const frameCount = 10
  const red = [249, 65, 68]
  const orange = [243, 114, 44]
  const gold = [249, 199, 79]
  const green = [144, 190, 109]

  return {
    key: 'festive-marquee',
    name: '节日灯牌',
    strap: '灯珠追光 + 节庆横幅',
    description: '适合节日主题与活动感场景。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 4, 4, 56, 56, [72, 19, 28])
        drawRect(map, 9, 11, 46, 14, [40, 13, 17])
        drawRect(map, 11, 31, 42, 18, [43, 24, 10])
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const ribbonY = 33 + (frameIndex % 2)
        drawRect(map, 14, ribbonY, 36, 8, orange)
        drawRect(map, 16 + (frameIndex % 5) * 5, 44, 4, 4, gold)
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const bulbColors = [red, orange, gold, green]
        for (let i = 0; i < 12; i += 1) {
          const color = bulbColors[(i + frameIndex) % bulbColors.length]
          setPixel(map, 8 + i * 4, 8, color)
          setPixel(map, 8 + i * 4, 55, color)
        }
      })

      return {
        paletteKey: 'holiday-marquee',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 14, y: 15, font: 2, color: { r: 249, g: 199, b: 79 } },
          date: { visible: true, x: 16, y: 34, font: 1, color: { r: 255, g: 235, b: 166 } },
          week: { visible: true, x: 16, y: 41, font: 1, color: { r: 144, g: 190, b: 109 } },
          showSeconds: false,
          hourFormat: 24,
        },
      }
    },
  }
}

function minimalCalendarTemplate() {
  const frameCount = 6
  const ink = [214, 246, 255]
  const calm = [123, 223, 242]
  const line = [31, 69, 99]

  return {
    key: 'minimal-calendar',
    name: '极简日历',
    strap: '净面卡片 + 轻微翻页',
    description: '安静、干净、偏信息设计的像素时钟主题。',
    frameCount,
    buildSeed() {
      const background = buildStaticFrames(frameCount, (map) => {
        drawFrame(map, 8, 8, 48, 48, line)
        drawRect(map, 11, 11, 42, 42, [7, 21, 34])
        drawHorizontal(map, 12, 24, 40, line)
        drawVertical(map, 28, 25, 25, line)
      })

      const motion = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        drawRect(map, 13, 13, 14, 9, ink)
        drawRect(map, 31, 13, 18, 9, ink)
        drawRect(map, 14 + frameIndex, 47, 4, 3, calm)
      })

      const accent = buildAnimatedFrames(frameCount, (map, frameIndex) => {
        const pulse = frameIndex % 2 === 0 ? calm : ink
        drawPoints(map, [[14, 28], [14, 35], [14, 42], [33, 28], [33, 35], [33, 42]], pulse)
      })

      return {
        paletteKey: 'calm-signal',
        background,
        motion,
        accent,
        clock: {
          time: { visible: true, x: 30, y: 30, font: 2, color: { r: 214, g: 246, b: 255 } },
          date: { visible: true, x: 31, y: 39, font: 1, color: { r: 123, g: 223, b: 242 } },
          week: { visible: false, x: 31, y: 46, font: 1, color: { r: 214, g: 246, b: 255 } },
          showSeconds: false,
          hourFormat: 24,
        },
      }
    },
  }
}

function motionPresetFactory(key, name, description, painter) {
  return {
    key,
    name,
    description,
    apply(frameCount) {
      return buildAnimatedFrames(frameCount, (map, frameIndex) => {
        painter(map, frameIndex, frameCount)
      })
    },
  }
}

function backgroundPresetFactory(key, name, description, painter) {
  return {
    key,
    name,
    description,
    apply(frameCount) {
      return buildAnimatedFrames(frameCount, (map, frameIndex) => {
        painter(map, frameIndex, frameCount)
      })
    },
  }
}

export const OFFICIAL_TEMPLATES = [
  retroDigitalTemplate(),
  neonHudTemplate(),
  pixelPetTemplate(),
  weatherPulseTemplate(),
  festiveMarqueeTemplate(),
  minimalCalendarTemplate(),
]

export const MOTION_PRESETS = [
  motionPresetFactory('pulse', '呼吸', '让块面做平滑呼吸亮度变化。', (map, frameIndex) => {
    const sizes = [8, 10, 12, 10, 8, 6, 8, 10]
    const size = sizes[frameIndex % sizes.length]
    drawFrame(map, 32 - size / 2, 32 - size / 2, size, size, [0, 245, 212])
  }),
  motionPresetFactory('blink', '闪烁', '交替亮灭的点位氛围。', (map, frameIndex) => {
    const points = frameIndex % 2 === 0 ? [[18, 18], [46, 18], [18, 46], [46, 46]] : [[32, 18], [18, 32], [46, 32], [32, 46]]
    drawPoints(map, points, [254, 228, 64])
  }),
  motionPresetFactory('sweep', '扫光', '从左到右的 HUD 扫描。', (map, frameIndex) => {
    drawRect(map, 8 + frameIndex * 4, 24, 8, 16, [0, 187, 249])
  }),
  motionPresetFactory('jump-text', '跳字', '用于烘托时间文本的上下跳动。', (map, frameIndex) => {
    const y = frameIndex % 2 === 0 ? 44 : 46
    drawHorizontal(map, 14, y, 36, [255, 191, 105])
  }),
  motionPresetFactory('line-reveal', '逐行显现', '每帧增加一行，适合开场。', (map, frameIndex) => {
    const rows = 6 + frameIndex * 5
    for (let y = 10; y < rows; y += 5) {
      drawHorizontal(map, 12, y, 40, [123, 223, 242])
    }
  }),
  motionPresetFactory('pixel-rain', '像素雨', '垂直下落的点阵雨滴。', (map, frameIndex, frameCount) => {
    for (let i = 0; i < 6; i += 1) {
      const x = 12 + i * 8
      const y = (frameIndex * 6 + i * 5) % 48
      drawVertical(map, x, 10 + y, 4, [144, 190, 109])
    }
  }),
]

export const BACKGROUND_PRESETS = [
  backgroundPresetFactory('starfield', '星流', '适合夜景与科幻主题的流动星点。', (map, frameIndex) => {
    drawFrame(map, 3, 3, 58, 58, [16, 26, 30])
    for (let i = 0; i < 12; i += 1) {
      const x = 7 + ((i * 5 + frameIndex * 3) % 48)
      const y = 8 + ((i * 9 + frameIndex * 4) % 48)
      setPixel(map, x, y, [102, 234, 214])
    }
  }),
  backgroundPresetFactory('scan-grid', '扫描网格', '让背景出现缓慢移动的扫描线。', (map, frameIndex) => {
    drawFrame(map, 4, 4, 56, 56, [22, 44, 52])
    for (let y = 8 + (frameIndex % 6); y < 56; y += 6) {
      drawHorizontal(map, 8, y, 48, [18, 83, 94])
    }
    drawVertical(map, 8 + (frameIndex % 6), 8, 48, [12, 55, 67])
  }),
  backgroundPresetFactory('wave-band', '波纹带', '用左右摆动的条带营造动态背景。', (map, frameIndex, frameCount) => {
    const offsets = [0, 2, 4, 2, 0, -2, -4, -2]
    for (let band = 0; band < 6; band += 1) {
      const shift = offsets[(frameIndex + band) % offsets.length]
      drawHorizontal(map, 8 + shift, 10 + band * 8, 40, [31, 69, 99])
    }
    drawFrame(map, 5, 5, 54, 54, [102, 234, 214])
  }),
  backgroundPresetFactory('diagonal-drift', '斜向漂移', '轻量斜向像素带，适合极简动态主题。', (map, frameIndex) => {
    drawFrame(map, 6, 6, 52, 52, [42, 30, 24])
    for (let i = 0; i < 8; i += 1) {
      const x = 8 + ((i * 6 + frameIndex * 4) % 40)
      const y = 8 + i * 6
      drawRect(map, x, y, 6, 2, [255, 191, 105])
    }
  }),
]

export function getTemplateByKey(templateKey) {
  return OFFICIAL_TEMPLATES.find((template) => template.key === templateKey)
}

export function getMotionPresetByKey(presetKey) {
  return MOTION_PRESETS.find((preset) => preset.key === presetKey)
}

export function getBackgroundPresetByKey(presetKey) {
  return BACKGROUND_PRESETS.find((preset) => preset.key === presetKey)
}

export function getPaletteByKey(paletteKey) {
  return COLOR_PALETTES.find((palette) => palette.key === paletteKey)
}

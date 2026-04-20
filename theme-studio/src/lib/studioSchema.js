export const MATRIX_SIZE = 64

export const PIXEL_LAYER_KEYS = ['background', 'motion', 'accent']
export const TEXT_LAYER_KEYS = ['time', 'date', 'week']
export const LAYER_ORDER = ['background', 'motion', 'accent', 'time', 'date', 'week']

export const LAYER_LABELS = {
  background: '背景层',
  motion: '动效层',
  accent: '点缀层',
  time: '时间层',
  date: '日期层',
  week: '星期层',
}

export const PIXEL_TOOLS = [
  { key: 'brush', label: '画笔' },
  { key: 'eraser', label: '橡皮' },
  { key: 'fill', label: '填充' },
  { key: 'picker', label: '取色' },
]

export const FONT_OPTIONS = [
  { id: 1, label: 'Terminal' },
  { id: 2, label: 'Grid' },
  { id: 3, label: 'Mono' },
]

export const TIME_PRESET_OPTIONS = [
  { key: '08:00', label: '08:00', value: '08:00:00' },
  { key: '12:34', label: '12:34', value: '12:34:00' },
  { key: '18:45', label: '18:45', value: '18:45:00' },
  { key: '23:59', label: '23:59', value: '23:59:00' },
]

export const COLOR_PALETTES = [
  {
    key: 'arcade-neon',
    name: 'Arcade Neon',
    colors: ['#00f5d4', '#00bbf9', '#fee440', '#ff6f59', '#0c0910'],
  },
  {
    key: 'amber-terminal',
    name: 'Amber Terminal',
    colors: ['#ffbf69', '#ff9f1c', '#cbf3f0', '#2ec4b6', '#0f0f14'],
  },
  {
    key: 'holiday-marquee',
    name: 'Holiday Marquee',
    colors: ['#f94144', '#f3722c', '#f9c74f', '#90be6d', '#577590'],
  },
  {
    key: 'calm-signal',
    name: 'Calm Signal',
    colors: ['#d6f6ff', '#7bdff2', '#b2f7ef', '#eff7f6', '#112a46'],
  },
]

export const EXPORT_FILE_NAMES = {
  themeJson: 'theme.json',
  coverPng: 'cover.png',
  previewGif: 'preview.gif',
  validationJson: 'validation.json',
}

import { MATRIX_SIZE } from './themeContract'

const WEEK_TEXT = ['周日', '周一', '周二', '周三', '周四', '周五', '周六']
const FONT_MAP = {
  1: '"Courier New", monospace',
  2: '"Lucida Console", monospace',
  3: '"Consolas", monospace',
}
const FONT_SIZE_MAP = {
  1: 8,
  2: 10,
  3: 13,
}

function pad2(value) {
  if (value >= 10) {
    return String(value)
  }
  return `0${value}`
}

function toCssColor(r, g, b) {
  return `rgb(${r}, ${g}, ${b})`
}

function getFontStackById(fontId) {
  if (!FONT_MAP[fontId]) {
    throw new Error('未支持的字体 ID')
  }

  return FONT_MAP[fontId]
}

export function getWidgetFontSize(size) {
  if (!FONT_SIZE_MAP[size]) {
    return FONT_SIZE_MAP[2]
  }

  return FONT_SIZE_MAP[size]
}

function drawWidgetText(ctx, layer, text) {
  ctx.fillStyle = toCssColor(layer.color.r, layer.color.g, layer.color.b)
  ctx.font = `${getWidgetFontSize(layer.size)}px ${getFontStackById(layer.font)}`
  ctx.fillText(text, layer.x, layer.y)
}

export function formatClockTexts(now, textLayout) {
  let hour = now.getHours()
  if (textLayout.hourFormat === 12) {
    if (hour === 0) {
      hour = 12
    } else if (hour > 12) {
      hour -= 12
    }
  }

  const minute = now.getMinutes()
  const second = now.getSeconds()

  let timeText = `${pad2(hour)}:${pad2(minute)}`
  if (textLayout.showSeconds) {
    timeText = `${timeText}:${pad2(second)}`
  }

  const dateText = `${now.getFullYear()}-${pad2(now.getMonth() + 1)}-${pad2(now.getDate())}`
  const weekText = WEEK_TEXT[now.getDay()]

  return {
    timeText,
    dateText,
    weekText,
    yearText: `${now.getFullYear()}`,
  }
}

export function createCanvas(width, height) {
  const canvas = document.createElement('canvas')
  canvas.width = width
  canvas.height = height
  return canvas
}

export function drawBaseFrame(ctx, pixelTuples) {
  ctx.clearRect(0, 0, MATRIX_SIZE, MATRIX_SIZE)
  ctx.fillStyle = 'rgb(0, 0, 0)'
  ctx.fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE)

  for (let i = 0; i < pixelTuples.length; i += 1) {
    const tuple = pixelTuples[i]
    ctx.fillStyle = toCssColor(tuple[2], tuple[3], tuple[4])
    ctx.fillRect(tuple[0], tuple[1], 1, 1)
  }
}

export function drawTextLayers(ctx, textLayers, now) {
  const texts = formatClockTexts(now, textLayers)
  ctx.textBaseline = 'top'

  if (textLayers.time.visible) {
    drawWidgetText(ctx, textLayers.time, texts.timeText)
  }

  if (textLayers.date.visible) {
    drawWidgetText(ctx, textLayers.date, texts.dateText)
  }

  if (textLayers.week.visible) {
    drawWidgetText(ctx, textLayers.week, texts.weekText)
  }

  if (textLayers.year?.visible) {
    drawWidgetText(ctx, textLayers.year, texts.yearText)
  }
}

export function buildPreviewFrameCanvas(pixelTuples, textLayers, now) {
  const canvas = createCanvas(MATRIX_SIZE, MATRIX_SIZE)
  const ctx = canvas.getContext('2d', { willReadFrequently: true })
  drawBaseFrame(ctx, pixelTuples)
  drawTextLayers(ctx, textLayers, now)
  return canvas
}

export function drawScaledCanvas(targetContext, sourceCanvas, width, height) {
  targetContext.imageSmoothingEnabled = false
  targetContext.clearRect(0, 0, width, height)
  targetContext.drawImage(sourceCanvas, 0, 0, width, height)
}

export function tuplesToRgba(pixelTuples, textLayers, now) {
  const previewCanvas = buildPreviewFrameCanvas(pixelTuples, textLayers, now)
  const ctx = previewCanvas.getContext('2d', { willReadFrequently: true })
  const imageData = ctx.getImageData(0, 0, MATRIX_SIZE, MATRIX_SIZE)
  return imageData.data
}

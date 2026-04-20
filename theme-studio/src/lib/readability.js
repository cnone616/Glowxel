import { formatClockTexts, getWidgetFontSize } from '../utils/canvasRenderer'
import { MATRIX_SIZE } from './studioSchema'

function estimateTextWidth(text, fontId) {
  const charWidth = fontId === 2 ? 4 : 5
  return text.length * charWidth
}

function buildLayerRect(layer, text) {
  const fontSize = getWidgetFontSize(layer.size)
  const scale = fontSize / 8
  return {
    left: layer.x,
    top: layer.y,
    right: layer.x + Math.ceil(estimateTextWidth(text, layer.font) * scale),
    bottom: layer.y + fontSize,
  }
}

function intersects(a, b) {
  return !(a.right <= b.left || b.right <= a.left || a.bottom <= b.top || b.bottom <= a.top)
}

function outside(rect) {
  return rect.left < 0 || rect.top < 0 || rect.right > MATRIX_SIZE || rect.bottom > MATRIX_SIZE
}

export function buildReadabilityReport(textLayout, now) {
  const texts = formatClockTexts(now, textLayout)
  const warnings = []
  const visibleLayers = []

  if (textLayout.time.visible) {
    visibleLayers.push({ key: 'time', rect: buildLayerRect(textLayout.time, texts.timeText), text: texts.timeText })
  }

  if (textLayout.date.visible) {
    visibleLayers.push({ key: 'date', rect: buildLayerRect(textLayout.date, texts.dateText), text: texts.dateText })
  }

  if (textLayout.week.visible) {
    visibleLayers.push({ key: 'week', rect: buildLayerRect(textLayout.week, texts.weekText), text: texts.weekText })
  }

  if (textLayout.year?.visible) {
    visibleLayers.push({ key: 'year', rect: buildLayerRect(textLayout.year, texts.yearText), text: texts.yearText })
  }

  for (let i = 0; i < visibleLayers.length; i += 1) {
    if (outside(visibleLayers[i].rect)) {
      warnings.push({
        code: 'TEXT_OUTSIDE_MATRIX',
        message: `${visibleLayers[i].key} 文本超出了 64x64 预览边界`,
      })
    }
  }

  for (let i = 0; i < visibleLayers.length; i += 1) {
    for (let j = i + 1; j < visibleLayers.length; j += 1) {
      if (intersects(visibleLayers[i].rect, visibleLayers[j].rect)) {
        warnings.push({
          code: 'TEXT_OVERLAP',
          message: `${visibleLayers[i].key} 与 ${visibleLayers[j].key} 存在重叠风险`,
        })
      }
    }
  }

  return {
    ok: warnings.length === 0,
    warnings,
  }
}

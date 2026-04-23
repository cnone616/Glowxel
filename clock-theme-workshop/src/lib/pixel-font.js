const FONT_MAP = {
  ' ': ['00000', '00000', '00000', '00000', '00000', '00000', '00000'],
  '-': ['00000', '00000', '00000', '11111', '00000', '00000', '00000'],
  '/': ['00001', '00010', '00100', '01000', '10000', '00000', '00000'],
  ':': ['00000', '00100', '00100', '00000', '00100', '00100', '00000'],
  '.': ['00000', '00000', '00000', '00000', '00000', '00100', '00100'],
  '0': ['01110', '10001', '10011', '10101', '11001', '10001', '01110'],
  '1': ['00100', '01100', '00100', '00100', '00100', '00100', '01110'],
  '2': ['01110', '10001', '00001', '00010', '00100', '01000', '11111'],
  '3': ['11110', '00001', '00001', '01110', '00001', '00001', '11110'],
  '4': ['00010', '00110', '01010', '10010', '11111', '00010', '00010'],
  '5': ['11111', '10000', '10000', '11110', '00001', '00001', '11110'],
  '6': ['01110', '10000', '10000', '11110', '10001', '10001', '01110'],
  '7': ['11111', '00001', '00010', '00100', '01000', '01000', '01000'],
  '8': ['01110', '10001', '10001', '01110', '10001', '10001', '01110'],
  '9': ['01110', '10001', '10001', '01111', '00001', '00001', '01110'],
  A: ['01110', '10001', '10001', '11111', '10001', '10001', '10001'],
  B: ['11110', '10001', '10001', '11110', '10001', '10001', '11110'],
  C: ['01110', '10001', '10000', '10000', '10000', '10001', '01110'],
  D: ['11110', '10001', '10001', '10001', '10001', '10001', '11110'],
  E: ['11111', '10000', '10000', '11110', '10000', '10000', '11111'],
  F: ['11111', '10000', '10000', '11110', '10000', '10000', '10000'],
  G: ['01110', '10001', '10000', '10111', '10001', '10001', '01111'],
  H: ['10001', '10001', '10001', '11111', '10001', '10001', '10001'],
  I: ['01110', '00100', '00100', '00100', '00100', '00100', '01110'],
  J: ['00111', '00010', '00010', '00010', '00010', '10010', '01100'],
  K: ['10001', '10010', '10100', '11000', '10100', '10010', '10001'],
  L: ['10000', '10000', '10000', '10000', '10000', '10000', '11111'],
  M: ['10001', '11011', '10101', '10101', '10001', '10001', '10001'],
  N: ['10001', '11001', '10101', '10011', '10001', '10001', '10001'],
  O: ['01110', '10001', '10001', '10001', '10001', '10001', '01110'],
  P: ['11110', '10001', '10001', '11110', '10000', '10000', '10000'],
  Q: ['01110', '10001', '10001', '10001', '10101', '10010', '01101'],
  R: ['11110', '10001', '10001', '11110', '10100', '10010', '10001'],
  S: ['01111', '10000', '10000', '01110', '00001', '00001', '11110'],
  T: ['11111', '00100', '00100', '00100', '00100', '00100', '00100'],
  U: ['10001', '10001', '10001', '10001', '10001', '10001', '01110'],
  V: ['10001', '10001', '10001', '10001', '10001', '01010', '00100'],
  W: ['10001', '10001', '10001', '10101', '10101', '10101', '01010'],
  X: ['10001', '10001', '01010', '00100', '01010', '10001', '10001'],
  Y: ['10001', '10001', '01010', '00100', '00100', '00100', '00100'],
  Z: ['11111', '00001', '00010', '00100', '01000', '10000', '11111'],
  '?': ['01110', '10001', '00001', '00010', '00100', '00000', '00100'],
}

function normalizeChar(character) {
  if (Object.prototype.hasOwnProperty.call(FONT_MAP, character)) {
    return character
  }
  const upper = character.toUpperCase()
  if (Object.prototype.hasOwnProperty.call(FONT_MAP, upper)) {
    return upper
  }
  return '?'
}

export function formatWidgetText(widget, previewDate) {
  if (widget.type === 'time') {
    let hours = previewDate.getHours()
    if (widget.hourFormat === 12) {
      hours %= 12
      if (hours === 0) {
        hours = 12
      }
    }
    const hh = String(hours).padStart(2, '0')
    const mm = String(previewDate.getMinutes()).padStart(2, '0')
    if (widget.showSeconds === true) {
      const ss = String(previewDate.getSeconds()).padStart(2, '0')
      return `${hh}:${mm}:${ss}`
    }
    return `${hh}:${mm}`
  }

  if (widget.type === 'date') {
    const month = String(previewDate.getMonth() + 1).padStart(2, '0')
    const day = String(previewDate.getDate()).padStart(2, '0')
    return `${month}-${day}`
  }

  if (widget.type === 'week') {
    return ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT'][previewDate.getDay()]
  }

  if (widget.type === 'year') {
    return String(previewDate.getFullYear())
  }

  return widget.type.toUpperCase()
}

export function measurePixelText(text, scale, letterSpacing) {
  const normalizedText = text.length === 0 ? ' ' : text
  let width = 0

  for (let index = 0; index < normalizedText.length; index += 1) {
    const glyph = FONT_MAP[normalizeChar(normalizedText[index])]
    width += glyph[0].length * scale
    if (index < normalizedText.length - 1) {
      width += scale + letterSpacing
    }
  }

  return {
    width,
    height: 7 * scale,
  }
}

export function drawPixelText(context, widget, previewDate) {
  const text = formatWidgetText(widget, previewDate)
  const scale = Math.max(1, widget.scale)
  const letterSpacing = Math.max(0, widget.letterSpacing)
  let cursorX = widget.x

  context.save()
  context.fillStyle = widget.color

  for (let charIndex = 0; charIndex < text.length; charIndex += 1) {
    const glyph = FONT_MAP[normalizeChar(text[charIndex])]
    for (let row = 0; row < glyph.length; row += 1) {
      for (let column = 0; column < glyph[row].length; column += 1) {
        if (glyph[row][column] !== '1') {
          continue
        }
        context.fillRect(cursorX + column * scale, widget.y + row * scale, scale, scale)
      }
    }
    cursorX += glyph[0].length * scale + scale + letterSpacing
  }

  context.restore()
}

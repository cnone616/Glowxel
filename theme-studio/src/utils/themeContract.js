export const MATRIX_SIZE = 64
export const FRAME_TYPE_FULL = 1
export const FRAME_TYPE_DIFF = 0

const TOP_LEVEL_FIELDS = ['name', 'f', 'd']
const FRAME_FIELDS = ['t', 'd', 'c', 'p']

function hasOwnField(target, field) {
  return Object.prototype.hasOwnProperty.call(target, field)
}

function isInteger(value) {
  return Number.isInteger(value)
}

function isRgbValue(value) {
  return isInteger(value) && value >= 0 && value <= 255
}

function isCoordinate(value) {
  return isInteger(value) && value >= 0 && value < MATRIX_SIZE
}

function pushError(errors, path, code, message) {
  errors.push({ path, code, message })
}

function pushWarning(warnings, path, code, message) {
  warnings.push({ path, code, message })
}

function ensureExactFields(target, fields, path, errors) {
  if (typeof target !== 'object' || target === null || Array.isArray(target)) {
    pushError(errors, path, 'INVALID_OBJECT', '必须为对象')
    return
  }

  for (let i = 0; i < fields.length; i += 1) {
    const field = fields[i]
    if (!hasOwnField(target, field)) {
      pushError(errors, path, 'MISSING_FIELD', `缺少字段 ${field}`)
    }
  }

  const keys = Object.keys(target)
  for (let i = 0; i < keys.length; i += 1) {
    const key = keys[i]
    if (!fields.includes(key)) {
      pushError(errors, path, 'UNAGREED_FIELD', `存在未约定字段 ${key}`)
    }
  }
}

function validatePixelTuple(tuple, path, errors) {
  if (!Array.isArray(tuple)) {
    pushError(errors, path, 'INVALID_PIXEL', '像素必须为数组 [x,y,r,g,b]')
    return
  }

  if (tuple.length !== 5) {
    pushError(errors, path, 'INVALID_PIXEL_LENGTH', '像素数组长度必须为 5')
    return
  }

  const x = tuple[0]
  const y = tuple[1]
  const r = tuple[2]
  const g = tuple[3]
  const b = tuple[4]

  if (!isCoordinate(x)) {
    pushError(errors, `${path}[0]`, 'PIXEL_X_OUT_OF_RANGE', 'x 必须在 0-63')
  }

  if (!isCoordinate(y)) {
    pushError(errors, `${path}[1]`, 'PIXEL_Y_OUT_OF_RANGE', 'y 必须在 0-63')
  }

  if (!isRgbValue(r)) {
    pushError(errors, `${path}[2]`, 'PIXEL_R_OUT_OF_RANGE', 'r 必须在 0-255')
  }

  if (!isRgbValue(g)) {
    pushError(errors, `${path}[3]`, 'PIXEL_G_OUT_OF_RANGE', 'g 必须在 0-255')
  }

  if (!isRgbValue(b)) {
    pushError(errors, `${path}[4]`, 'PIXEL_B_OUT_OF_RANGE', 'b 必须在 0-255')
  }
}

export function createEmptyFrame(type, delay) {
  return {
    t: type,
    d: delay,
    c: 0,
    p: []
  }
}

export function createEmptyThemePackage(name) {
  return {
    name,
    f: 1,
    d: [createEmptyFrame(FRAME_TYPE_FULL, 120)]
  }
}

export function sortPixelTuples(pixels) {
  pixels.sort((a, b) => {
    if (a[1] !== b[1]) {
      return a[1] - b[1]
    }

    return a[0] - b[0]
  })
}

export function buildDeterministicThemePackage(themePackage) {
  const packageCopy = JSON.parse(JSON.stringify(themePackage))

  for (let i = 0; i < packageCopy.d.length; i += 1) {
    const frame = packageCopy.d[i]
    sortPixelTuples(frame.p)
    frame.c = frame.p.length
  }

  packageCopy.f = packageCopy.d.length
  return packageCopy
}

export function validateThemePackageStrict(themePackage) {
  const errors = []
  const warnings = []

  ensureExactFields(themePackage, TOP_LEVEL_FIELDS, 'root', errors)

  if (typeof themePackage.name !== 'string') {
    pushError(errors, 'root.name', 'INVALID_NAME', 'name 必须为字符串')
  } else if (themePackage.name.trim().length === 0) {
    pushError(errors, 'root.name', 'EMPTY_NAME', 'name 不能为空')
  }

  if (!isInteger(themePackage.f) || themePackage.f < 1) {
    pushError(errors, 'root.f', 'INVALID_FRAME_COUNT', 'f 必须为大于等于 1 的整数')
  }

  if (!Array.isArray(themePackage.d)) {
    pushError(errors, 'root.d', 'INVALID_FRAME_ARRAY', 'd 必须为数组')
  }

  if (Array.isArray(themePackage.d)) {
    if (isInteger(themePackage.f) && themePackage.f !== themePackage.d.length) {
      pushError(errors, 'root.f', 'FRAME_COUNT_MISMATCH', 'f 与 d.length 不一致')
    }

    for (let i = 0; i < themePackage.d.length; i += 1) {
      const frame = themePackage.d[i]
      const framePath = `root.d[${i}]`
      ensureExactFields(frame, FRAME_FIELDS, framePath, errors)

      if (!isInteger(frame.t) || (frame.t !== FRAME_TYPE_FULL && frame.t !== FRAME_TYPE_DIFF)) {
        pushError(errors, `${framePath}.t`, 'INVALID_FRAME_TYPE', 't 只能为 1(全量) 或 0(差分)')
      }

      if (!isInteger(frame.d) || frame.d <= 0) {
        pushError(errors, `${framePath}.d`, 'INVALID_FRAME_DELAY', 'd 必须为正整数毫秒')
      }

      if (!isInteger(frame.c) || frame.c < 0) {
        pushError(errors, `${framePath}.c`, 'INVALID_PIXEL_COUNT', 'c 必须为大于等于 0 的整数')
      }

      if (!Array.isArray(frame.p)) {
        pushError(errors, `${framePath}.p`, 'INVALID_PIXEL_ARRAY', 'p 必须为像素数组')
      }

      if (Array.isArray(frame.p)) {
        if (isInteger(frame.c) && frame.c !== frame.p.length) {
          pushError(errors, `${framePath}.c`, 'PIXEL_COUNT_MISMATCH', 'c 与 p.length 不一致')
        }

        const duplicateGuard = new Set()

        for (let j = 0; j < frame.p.length; j += 1) {
          const pixel = frame.p[j]
          const pixelPath = `${framePath}.p[${j}]`
          validatePixelTuple(pixel, pixelPath, errors)

          if (Array.isArray(pixel) && pixel.length === 5) {
            const key = `${pixel[0]}:${pixel[1]}`
            if (duplicateGuard.has(key)) {
              pushWarning(warnings, pixelPath, 'DUPLICATE_COORDINATE', '同一帧存在重复坐标，后者将覆盖前者')
            } else {
              duplicateGuard.add(key)
            }
          }
        }
      }

      if (i === 0 && frame.t === FRAME_TYPE_DIFF) {
        pushError(errors, `${framePath}.t`, 'FIRST_FRAME_DIFF_FORBIDDEN', '首帧必须为全量帧')
      }
    }
  }

  const stats = {
    frameCount: 0,
    fullFrameCount: 0,
    diffFrameCount: 0,
    totalPixels: 0,
    estimatedBinaryBytes: 0
  }

  if (Array.isArray(themePackage.d)) {
    stats.frameCount = themePackage.d.length
    for (let i = 0; i < themePackage.d.length; i += 1) {
      const frame = themePackage.d[i]
      if (frame.t === FRAME_TYPE_FULL) {
        stats.fullFrameCount += 1
      }
      if (frame.t === FRAME_TYPE_DIFF) {
        stats.diffFrameCount += 1
      }
      if (Array.isArray(frame.p)) {
        stats.totalPixels += frame.p.length
      }
    }
  }

  stats.estimatedBinaryBytes = stats.totalPixels * 5

  return {
    ok: errors.length === 0,
    errors,
    warnings,
    stats,
    checkedAt: new Date().toISOString()
  }
}

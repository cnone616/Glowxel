import { findClosestColor, getColorByCode } from '@/data/artkal-colors.js'

const IMAGE_SAMPLE_SCALE = 6
const IMAGE_SAMPLE_MAX_EDGE = 2048

function clampColor(value) {
  if (value < 0) {
    return 0
  }
  if (value > 255) {
    return 255
  }
  return value
}

function drawImageCover(ctx, image, targetWidth, targetHeight) {
  const scale = Math.max(targetWidth / image.width, targetHeight / image.height)
  const drawWidth = image.width * scale
  const drawHeight = image.height * scale
  const offsetX = (targetWidth - drawWidth) / 2
  const offsetY = (targetHeight - drawHeight) / 2

  ctx.clearRect(0, 0, targetWidth, targetHeight)
  ctx.drawImage(image, offsetX, offsetY, drawWidth, drawHeight)
}

function drawImageFit(ctx, image, targetWidth, targetHeight) {
  ctx.clearRect(0, 0, targetWidth, targetHeight)
  ctx.drawImage(image, 0, 0, targetWidth, targetHeight)
}

function adjustImageData(imageData, options = {}) {
  const contrast = Number(options.contrast ?? 0.12)
  const saturation = Number(options.saturation ?? 0.08)
  const brightness = Number(options.brightness ?? 0.02)
  const pixels = imageData.data

  for (let index = 0; index < pixels.length; index += 4) {
    const r = pixels[index]
    const g = pixels[index + 1]
    const b = pixels[index + 2]

    const baseRed = (r - 128) * (1 + contrast) + 128
    const baseGreen = (g - 128) * (1 + contrast) + 128
    const baseBlue = (b - 128) * (1 + contrast) + 128
    const gray = (baseRed + baseGreen + baseBlue) / 3

    pixels[index] = clampColor(gray + (baseRed - gray) * (1 + saturation) + brightness * 255)
    pixels[index + 1] = clampColor(gray + (baseGreen - gray) * (1 + saturation) + brightness * 255)
    pixels[index + 2] = clampColor(gray + (baseBlue - gray) * (1 + saturation) + brightness * 255)
  }

  return imageData
}

function rgbToLab(r, g, b) {
  const srgb = [r, g, b].map((value) => {
    const normalized = value / 255
    return normalized > 0.04045
      ? ((normalized + 0.055) / 1.055) ** 2.4
      : normalized / 12.92
  })

  const x = (srgb[0] * 0.4124 + srgb[1] * 0.3576 + srgb[2] * 0.1805) / 0.95047
  const y = (srgb[0] * 0.2126 + srgb[1] * 0.7152 + srgb[2] * 0.0722) / 1
  const z = (srgb[0] * 0.0193 + srgb[1] * 0.1192 + srgb[2] * 0.9505) / 1.08883

  const xyz = [x, y, z].map((value) => {
    return value > 0.008856 ? value ** (1 / 3) : (7.787 * value) + (16 / 116)
  })

  return {
    l: (116 * xyz[1]) - 16,
    a: 500 * (xyz[0] - xyz[1]),
    b: 200 * (xyz[1] - xyz[2]),
  }
}

function labDistance(left, right) {
  return Math.sqrt(
    ((left.l - right.l) ** 2) +
    ((left.a - right.a) ** 2) +
    ((left.b - right.b) ** 2),
  )
}

function resolveClosestColorByLab(r, g, b, colorCodes) {
  const sourceLab = rgbToLab(r, g, b)
  let bestColor = null
  let bestDistance = Number.POSITIVE_INFINITY

  colorCodes.forEach((code) => {
    const color = getColorByCode(code)
    if (!color) {
      return
    }

    if (!color.lab) {
      color.lab = rgbToLab(color.r, color.g, color.b)
    }

    const distance = labDistance(sourceLab, color.lab)
    if (distance < bestDistance) {
      bestDistance = distance
      bestColor = color
    }
  })

  return bestColor
}

function averageSamples(samples) {
  if (!samples.length) {
    return { r: 255, g: 255, b: 255 }
  }

  let red = 0
  let green = 0
  let blue = 0

  samples.forEach((sample) => {
    red += sample.r
    green += sample.g
    blue += sample.b
  })

  return {
    r: Math.round(red / samples.length),
    g: Math.round(green / samples.length),
    b: Math.round(blue / samples.length),
  }
}

function buildDominantColor(samples, step = 12) {
  const buckets = new Map()

  samples.forEach((sample) => {
    const key = [
      Math.round(sample.r / step),
      Math.round(sample.g / step),
      Math.round(sample.b / step),
    ].join('-')

    if (!buckets.has(key)) {
      buckets.set(key, {
        red: 0,
        green: 0,
        blue: 0,
        count: 0,
      })
    }

    const bucket = buckets.get(key)
    bucket.red += sample.r
    bucket.green += sample.g
    bucket.blue += sample.b
    bucket.count += 1
  })

  let bestBucket = null
  buckets.forEach((bucket) => {
    if (!bestBucket || bucket.count > bestBucket.count) {
      bestBucket = bucket
    }
  })

  if (!bestBucket) {
    return averageSamples(samples)
  }

  return {
    r: Math.round(bestBucket.red / bestBucket.count),
    g: Math.round(bestBucket.green / bestBucket.count),
    b: Math.round(bestBucket.blue / bestBucket.count),
  }
}

function sampleCellColor(imageData, sampleX, sampleY, sampleWidth, sampleHeight) {
  const { width, height, data } = imageData
  const insetX = Math.max(0, Math.floor(sampleWidth * 0.12))
  const insetY = Math.max(0, Math.floor(sampleHeight * 0.12))
  const startX = Math.max(0, Math.floor(sampleX + insetX))
  const startY = Math.max(0, Math.floor(sampleY + insetY))
  const endX = Math.min(width - 1, Math.ceil(sampleX + sampleWidth - insetX))
  const endY = Math.min(height - 1, Math.ceil(sampleY + sampleHeight - insetY))
  const samples = []

  for (let y = startY; y <= endY; y += 1) {
    for (let x = startX; x <= endX; x += 1) {
      const idx = (y * width + x) * 4
      const alpha = data[idx + 3]
      if (alpha < 128) {
        continue
      }

      samples.push({
        r: data[idx],
        g: data[idx + 1],
        b: data[idx + 2],
      })
    }
  }

  if (!samples.length) {
    return null
  }

  return buildDominantColor(samples)
}

function imageDataToPixelMap(imageData, targetWidth, targetHeight, colorCodes) {
  const pixels = imageData.data
  const pixelMap = new Map()

  for (let y = 0; y < targetHeight; y++) {
    for (let x = 0; x < targetWidth; x++) {
      const idx = (y * targetWidth + x) * 4
      const r = pixels[idx]
      const g = pixels[idx + 1]
      const b = pixels[idx + 2]
      const a = pixels[idx + 3]

      if (a < 128) {
        continue
      }

      const closestColor = findClosestColor(r, g, b, colorCodes)
      if (closestColor) {
        pixelMap.set(`${x},${y}`, closestColor.code)
      }
    }
  }

  return pixelMap
}

function denoisePixelMap(pixelMap, width, height, passes = 1) {
  let nextMap = new Map(pixelMap)

  for (let passIndex = 0; passIndex < passes; passIndex += 1) {
    const workingMap = new Map(nextMap)

    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const key = `${x},${y}`
        const currentCode = nextMap.get(key) || ''
        const counts = new Map()

        for (let offsetY = -1; offsetY <= 1; offsetY += 1) {
          for (let offsetX = -1; offsetX <= 1; offsetX += 1) {
            if (offsetX === 0 && offsetY === 0) {
              continue
            }
            const neighborCode = nextMap.get(`${x + offsetX},${y + offsetY}`) || ''
            if (!neighborCode) {
              continue
            }
            counts.set(neighborCode, (counts.get(neighborCode) || 0) + 1)
          }
        }

        const sortedCounts = Array.from(counts.entries()).sort((left, right) => right[1] - left[1])
        if (!sortedCounts.length) {
          continue
        }

        const dominantCode = sortedCounts[0][0]
        const dominantCount = sortedCounts[0][1]
        if (dominantCode && dominantCode !== currentCode && dominantCount >= 4) {
          workingMap.set(key, dominantCode)
        }
      }
    }

    nextMap = workingMap
  }

  return nextMap
}

/**
 * 将图片转换为像素数据
 * @param {HTMLImageElement} image - 图片对象
 * @param {number} targetWidth - 目标宽度
 * @param {number} targetHeight - 目标高度
 * @param {Array} colorCodes - 颜色代码数组
 * @returns {Map} 像素数据 Map
 */
export function imageToPixels(image, targetWidth, targetHeight, colorCodes) {
  const sampleScale = Math.max(2, Math.min(IMAGE_SAMPLE_SCALE, Math.floor(IMAGE_SAMPLE_MAX_EDGE / Math.max(targetWidth, targetHeight, 1))))
  const sampleWidth = Math.max(targetWidth, targetWidth * sampleScale)
  const sampleHeight = Math.max(targetHeight, targetHeight * sampleScale)
  const canvas = document.createElement('canvas')
  canvas.width = sampleWidth
  canvas.height = sampleHeight
  const ctx = canvas.getContext('2d')

  ctx.imageSmoothingEnabled = true
  ctx.imageSmoothingQuality = 'high'
  drawImageFit(ctx, image, sampleWidth, sampleHeight)

  const adjustedData = adjustImageData(
    ctx.getImageData(0, 0, sampleWidth, sampleHeight),
    {
      contrast: 0.08,
      saturation: 0.06,
      brightness: 0.01,
    },
  )

  const pixelMap = new Map()
  const cellWidth = sampleWidth / targetWidth
  const cellHeight = sampleHeight / targetHeight

  for (let y = 0; y < targetHeight; y += 1) {
    for (let x = 0; x < targetWidth; x += 1) {
      const sampledColor = sampleCellColor(
        adjustedData,
        x * cellWidth,
        y * cellHeight,
        cellWidth,
        cellHeight,
      )

      if (!sampledColor) {
        continue
      }

      const closestColor =
        resolveClosestColorByLab(sampledColor.r, sampledColor.g, sampledColor.b, colorCodes) ||
        findClosestColor(sampledColor.r, sampledColor.g, sampledColor.b, colorCodes)

      if (closestColor) {
        pixelMap.set(`${x},${y}`, closestColor.code)
      }
    }
  }

  return denoisePixelMap(pixelMap, targetWidth, targetHeight, 1)
}

export function captureImageToPixels(image, targetWidth, targetHeight, colorCodes, options = {}) {
  const canvas = document.createElement('canvas')
  canvas.width = targetWidth
  canvas.height = targetHeight
  const ctx = canvas.getContext('2d')

  ctx.imageSmoothingEnabled = true
  drawImageCover(ctx, image, targetWidth, targetHeight)

  const adjustedData = adjustImageData(
    ctx.getImageData(0, 0, targetWidth, targetHeight),
    options,
  )
  const pixelMap = imageDataToPixelMap(adjustedData, targetWidth, targetHeight, colorCodes)
  const denoisePasses = Number(options.denoisePasses ?? 1)
  return denoisePixelMap(pixelMap, targetWidth, targetHeight, denoisePasses)
}

/**
 * 统计使用的颜色
 * @param {Map} pixels - 像素数据
 * @returns {Map} 使用的颜色及数量的 Map
 */
export function getUsedColors(pixels) {
  const colorCount = new Map()
  
  pixels.forEach(colorCode => {
    colorCount.set(colorCode, (colorCount.get(colorCode) || 0) + 1)
  })
  
  return colorCount
}

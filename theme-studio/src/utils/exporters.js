import { GIFEncoder, quantize, applyPalette } from 'gifenc'
import { buildDeterministicThemePackage, validateThemePackageStrict, MATRIX_SIZE } from './themeContract'
import { resolveAllFrameMaps, pixelMapToTuples } from './frameRuntime'
import { buildPreviewFrameCanvas, tuplesToRgba } from './canvasRenderer'

const MAX_RECOMMENDED_FRAMES = 180
const MAX_RECOMMENDED_TOTAL_PIXELS = 80000
const MAX_RECOMMENDED_BINARY_BYTES = 600000

function toBlobAsync(canvas, mimeType) {
  return new Promise((resolve, reject) => {
    canvas.toBlob((blob) => {
      if (blob === null) {
        reject(new Error('画布导出失败'))
        return
      }

      resolve(blob)
    }, mimeType)
  })
}

export function buildPerformanceHints(validationReport) {
  const hints = []

  if (validationReport.stats.frameCount > MAX_RECOMMENDED_FRAMES) {
    hints.push({
      level: 'warn',
      title: '帧数偏高',
      message: `当前帧数 ${validationReport.stats.frameCount}，建议不超过 ${MAX_RECOMMENDED_FRAMES}`
    })
  }

  if (validationReport.stats.totalPixels > MAX_RECOMMENDED_TOTAL_PIXELS) {
    hints.push({
      level: 'warn',
      title: '总像素量偏高',
      message: `总像素 ${validationReport.stats.totalPixels}，建议不超过 ${MAX_RECOMMENDED_TOTAL_PIXELS}`
    })
  }

  if (validationReport.stats.estimatedBinaryBytes > MAX_RECOMMENDED_BINARY_BYTES) {
    hints.push({
      level: 'warn',
      title: '估算包体偏大',
      message: `约 ${validationReport.stats.estimatedBinaryBytes} 字节，建议不超过 ${MAX_RECOMMENDED_BINARY_BYTES} 字节`
    })
  }

  if (validationReport.ok && hints.length === 0) {
    hints.push({
      level: 'ok',
      title: '预算健康',
      message: '当前主题包在建议预算范围内'
    })
  }

  return hints
}

export function buildValidationReport(themePackage) {
  const deterministicPackage = buildDeterministicThemePackage(themePackage)
  const baseReport = validateThemePackageStrict(deterministicPackage)
  const hints = buildPerformanceHints(baseReport)

  return {
    ...baseReport,
    hints,
    packageName: deterministicPackage.name
  }
}

export async function buildCoverPng(themePackage, textLayers) {
  const deterministicPackage = buildDeterministicThemePackage(themePackage)
  const frameMaps = resolveAllFrameMaps(deterministicPackage)
  const previewTime = new Date('2026-01-01T12:34:56Z')

  const firstFrameMap = frameMaps[0]
  const tuples = pixelMapToTuples(firstFrameMap)
  const coverCanvas = buildPreviewFrameCanvas(tuples, textLayers, previewTime)

  return toBlobAsync(coverCanvas, 'image/png')
}

export async function buildPreviewGif(themePackage, textLayers) {
  const deterministicPackage = buildDeterministicThemePackage(themePackage)
  const frameMaps = resolveAllFrameMaps(deterministicPackage)
  const gif = GIFEncoder()
  const startTime = Date.parse('2026-01-01T12:34:56Z')

  let elapsed = 0

  for (let i = 0; i < frameMaps.length; i += 1) {
    const map = frameMaps[i]
    const tuples = pixelMapToTuples(map)
    const now = new Date(startTime + elapsed)
    const rgba = tuplesToRgba(tuples, textLayers, now)

    const palette = quantize(rgba, 256)
    const index = applyPalette(rgba, palette)

    const frame = deterministicPackage.d[i]
    gif.writeFrame(index, MATRIX_SIZE, MATRIX_SIZE, {
      palette,
      delay: frame.d,
      repeat: 0
    })

    elapsed += frame.d
  }

  gif.finish()
  const output = gif.bytes()
  return new Blob([output], { type: 'image/gif' })
}

export async function buildThemeExportBundle(themePackage, textLayers) {
  const deterministicPackage = buildDeterministicThemePackage(themePackage)
  const report = buildValidationReport(deterministicPackage)

  const packageText = JSON.stringify(deterministicPackage, null, 2)
  const reportText = JSON.stringify(report, null, 2)

  const themeJsonBlob = new Blob([packageText], { type: 'application/json' })
  const validationBlob = new Blob([reportText], { type: 'application/json' })

  let coverPngBlob = null
  let previewGifBlob = null

  if (report.ok) {
    coverPngBlob = await buildCoverPng(deterministicPackage, textLayers)
    previewGifBlob = await buildPreviewGif(deterministicPackage, textLayers)
  }

  return {
    themePackage: deterministicPackage,
    report,
    files: {
      themeJsonBlob,
      validationBlob,
      coverPngBlob,
      previewGifBlob
    }
  }
}

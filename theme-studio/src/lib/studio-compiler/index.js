import { parseGIF, decompressFrames } from 'gifuct-js'
import { FRAME_TYPE_DIFF, FRAME_TYPE_FULL, MATRIX_SIZE, buildDeterministicThemePackage } from '../../utils/themeContract'
import { diffTuplesFromMaps, pixelMapToTuples, pixelKey } from '../../utils/frameRuntime'

const LOOP_DURATION_CAP_MS = 12000
const DEFAULT_LOOP_DURATION_MS = 1000
const DEFAULT_SAMPLE_STEP_MS = 200
const MIN_SAMPLE_STEP_MS = 80
const MAX_SAMPLE_STEP_MS = 240
const COLOR_ALPHA_THRESHOLD = 24

const assetRuntimeCache = new Map()

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value))
}

function gcd(a, b) {
  let left = Math.abs(a)
  let right = Math.abs(b)
  while (right !== 0) {
    const next = left % right
    left = right
    right = next
  }
  return left || 1
}

function lcm(a, b) {
  return Math.abs(a * b) / gcd(a, b)
}

function roundTo(value, step) {
  return Math.max(step, Math.round(value / step) * step)
}

function normalizeDelayMs(delayMs) {
  return Math.max(MIN_SAMPLE_STEP_MS, Math.round(delayMs || DEFAULT_SAMPLE_STEP_MS))
}

function createCanvas(width, height) {
  const canvas = document.createElement('canvas')
  canvas.width = width
  canvas.height = height
  return canvas
}

function cloneColor(color) {
  return [color[0], color[1], color[2]]
}

function mapsEqual(first, second) {
  if (first.size !== second.size) {
    return false
  }

  for (const [key, color] of first.entries()) {
    const other = second.get(key)
    if (!other) {
      return false
    }
    if (color[0] !== other[0] || color[1] !== other[1] || color[2] !== other[2]) {
      return false
    }
  }

  return true
}

function distance(first, second) {
  return Math.hypot(second.x - first.x, second.y - first.y)
}

function samplePathPoint(points, progress) {
  if (!Array.isArray(points) || points.length === 0) {
    return { x: 32, y: 32 }
  }

  if (points.length === 1) {
    return { x: points[0].x, y: points[0].y }
  }

  const segments = []
  let totalLength = 0
  for (let index = 0; index < points.length - 1; index += 1) {
    const start = points[index]
    const end = points[index + 1]
    const length = distance(start, end)
    totalLength += length
    segments.push({ start, end, length })
  }

  if (totalLength === 0) {
    return { x: points[0].x, y: points[0].y }
  }

  let targetLength = totalLength * progress
  for (const segment of segments) {
    if (targetLength <= segment.length) {
      const ratio = segment.length === 0 ? 0 : targetLength / segment.length
      return {
        x: segment.start.x + (segment.end.x - segment.start.x) * ratio,
        y: segment.start.y + (segment.end.y - segment.start.y) * ratio,
      }
    }
    targetLength -= segment.length
  }

  const lastPoint = points[points.length - 1]
  return { x: lastPoint.x, y: lastPoint.y }
}

function buildAssetCacheKey(asset) {
  return `${asset.id}:${asset.createdAt}:${asset.fileName}`
}

function loadImageElement(dataUrl) {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('素材图片解码失败'))
    image.src = dataUrl
  })
}

async function decodeStaticAsset(asset) {
  const image = await loadImageElement(asset.sourceDataUrl)
  const width = image.naturalWidth || asset.width || 1
  const height = image.naturalHeight || asset.height || 1
  const canvas = createCanvas(width, height)
  const context = canvas.getContext('2d', { willReadFrequently: true })
  context.imageSmoothingEnabled = false
  context.drawImage(image, 0, 0, width, height)

  return {
    width,
    height,
    totalDurationMs: DEFAULT_LOOP_DURATION_MS,
    frames: [
      {
        canvas,
        delayMs: DEFAULT_LOOP_DURATION_MS,
      },
    ],
  }
}

function buildGifFrameCanvas(width, height, imageData) {
  const canvas = createCanvas(width, height)
  const context = canvas.getContext('2d', { willReadFrequently: true })
  context.putImageData(imageData, 0, 0)
  return canvas
}

async function decodeGifAsset(asset) {
  const response = await fetch(asset.sourceDataUrl)
  const buffer = await response.arrayBuffer()
  const parsedGif = parseGIF(buffer)
  const rawFrames = decompressFrames(parsedGif, true)
  const width = parsedGif.lsd.width
  const height = parsedGif.lsd.height
  const workCanvas = createCanvas(width, height)
  const workContext = workCanvas.getContext('2d', { willReadFrequently: true })
  const frames = []

  let previousFrame = null
  let previousSnapshot = null

  for (const rawFrame of rawFrames) {
    if (previousFrame && previousFrame.disposalType === 2) {
      workContext.clearRect(
        previousFrame.dims.left,
        previousFrame.dims.top,
        previousFrame.dims.width,
        previousFrame.dims.height,
      )
    }

    if (previousFrame && previousFrame.disposalType === 3 && previousSnapshot) {
      workContext.putImageData(previousSnapshot, 0, 0)
      previousSnapshot = null
    }

    if (rawFrame.disposalType === 3) {
      previousSnapshot = workContext.getImageData(0, 0, width, height)
    }

    const patch = new ImageData(rawFrame.patch, rawFrame.dims.width, rawFrame.dims.height)
    workContext.putImageData(patch, rawFrame.dims.left, rawFrame.dims.top)

    const snapshot = workContext.getImageData(0, 0, width, height)
    frames.push({
      canvas: buildGifFrameCanvas(width, height, snapshot),
      delayMs: normalizeDelayMs(rawFrame.delay),
    })

    previousFrame = rawFrame
  }

  if (frames.length === 0) {
    return decodeStaticAsset(asset)
  }

  return {
    width,
    height,
    totalDurationMs: frames.reduce((sum, frame) => sum + frame.delayMs, 0),
    frames,
  }
}

async function loadAssetRuntime(asset) {
  const cacheKey = buildAssetCacheKey(asset)
  if (assetRuntimeCache.has(cacheKey)) {
    return assetRuntimeCache.get(cacheKey)
  }

  const runtimePromise = asset.mimeType === 'image/gif' ? decodeGifAsset(asset) : decodeStaticAsset(asset)
  assetRuntimeCache.set(cacheKey, runtimePromise)
  return runtimePromise
}

function getRuntimeFrame(runtime, elapsedMs) {
  if (!runtime || runtime.frames.length === 0) {
    return null
  }

  if (runtime.frames.length === 1 || runtime.totalDurationMs <= 0) {
    return runtime.frames[0]
  }

  let time = elapsedMs % runtime.totalDurationMs
  if (time < 0) {
    time += runtime.totalDurationMs
  }

  for (const frame of runtime.frames) {
    if (time < frame.delayMs) {
      return frame
    }
    time -= frame.delayMs
  }

  return runtime.frames[runtime.frames.length - 1]
}

function computeObjectState(object, elapsedMs) {
  const state = {
    x: object.x,
    y: object.y,
    opacity: object.visible ? 1 : 0,
  }

  if (!object.visible) {
    return state
  }

  if (object.motionType === 'blink') {
    const intervalMs = Math.max(200, object.blinkIntervalMs)
    const phase = Math.floor(elapsedMs / intervalMs) % 2
    state.opacity = phase === 0 ? 1 : 0.18
    return state
  }

  if (object.motionType !== 'path' || object.pathPoints.length === 0) {
    return state
  }

  const rawTime = Math.max(0, elapsedMs - object.delayMs)
  const durationMs = Math.max(600, object.durationMs)
  let progress = 0

  if (object.pathMode === 'once') {
    progress = Math.min(1, rawTime / durationMs)
  } else if (object.pathMode === 'pingpong') {
    const total = rawTime % (durationMs * 2)
    progress = total <= durationMs ? total / durationMs : 1 - (total - durationMs) / durationMs
  } else {
    progress = (rawTime % durationMs) / durationMs
  }

  const point = samplePathPoint(object.pathPoints, progress)
  state.x = Math.round(point.x)
  state.y = Math.round(point.y)
  return state
}

function computeFitRect(sourceWidth, sourceHeight, targetWidth, targetHeight, fitMode) {
  if (sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
    return { dx: 0, dy: 0, dw: 0, dh: 0 }
  }

  const scale = fitMode === 'cover'
    ? Math.max(targetWidth / sourceWidth, targetHeight / sourceHeight)
    : Math.min(targetWidth / sourceWidth, targetHeight / sourceHeight)

  const drawWidth = Math.max(1, Math.round(sourceWidth * scale))
  const drawHeight = Math.max(1, Math.round(sourceHeight * scale))

  return {
    dx: Math.round((targetWidth - drawWidth) / 2),
    dy: Math.round((targetHeight - drawHeight) / 2),
    dw: drawWidth,
    dh: drawHeight,
  }
}

function drawRuntimeFrame(context, runtime, elapsedMs, options) {
  if (!runtime) {
    return
  }

  const frame = getRuntimeFrame(runtime, elapsedMs)
  if (!frame) {
    return
  }

  const { canvas } = frame
  const rect = computeFitRect(canvas.width, canvas.height, options.width, options.height, options.fitMode)
  if (rect.dw === 0 || rect.dh === 0 || options.opacity <= 0) {
    return
  }

  context.save()
  context.imageSmoothingEnabled = false
  context.globalAlpha = options.opacity
  context.drawImage(canvas, options.x + rect.dx, options.y + rect.dy, rect.dw, rect.dh)
  context.restore()
}

function buildBackgroundState(project, elapsedMs) {
  const background = project.studio.scene.background
  const sequence = background.assetIds
    .map((assetId) => project.studio.materials.find((asset) => asset.id === assetId))
    .filter(Boolean)

  if (sequence.length === 0) {
    return {
      asset: null,
      opacity: 1,
      elapsedMs,
    }
  }

  if (background.behavior === 'rotate' && sequence.length > 1) {
    const intervalMs = Math.max(400, background.intervalMs)
    const slotIndex = Math.floor(elapsedMs / intervalMs) % sequence.length
    return {
      asset: sequence[slotIndex],
      opacity: 1,
      elapsedMs: elapsedMs % intervalMs,
    }
  }

  if (background.behavior === 'blink') {
    const intervalMs = Math.max(240, background.intervalMs)
    const phase = Math.floor(elapsedMs / intervalMs) % 2
    return {
      asset: sequence[0],
      opacity: phase === 0 ? 1 : 0.22,
      elapsedMs,
    }
  }

  return {
    asset: sequence[0],
    opacity: 1,
    elapsedMs,
  }
}

function collectTimingCandidates(project) {
  const candidates = []
  const background = project.studio.scene.background

  if (background.behavior === 'rotate' && background.assetIds.length > 1) {
    candidates.push(Math.max(400, background.intervalMs) * background.assetIds.length)
  }

  if (background.behavior === 'blink') {
    candidates.push(Math.max(240, background.intervalMs) * 2)
  }

  for (const object of project.studio.scene.objects) {
    if (object.motionType === 'blink') {
      candidates.push(Math.max(200, object.blinkIntervalMs) * 2)
    }

    if (object.motionType === 'path') {
      const durationMs = Math.max(600, object.durationMs)
      if (object.pathMode === 'pingpong') {
        candidates.push(durationMs * 2)
      } else {
        candidates.push(durationMs)
      }
    }
  }

  return candidates.filter((value) => Number.isFinite(value) && value > 0)
}

function collectSamplingHints(project) {
  const hints = []
  const background = project.studio.scene.background

  if (background.behavior === 'blink') {
    hints.push(Math.max(120, Math.round(background.intervalMs / 2)))
  }

  for (const object of project.studio.scene.objects) {
    if (object.motionType === 'blink') {
      hints.push(Math.max(120, Math.round(object.blinkIntervalMs / 2)))
    }

    if (object.motionType === 'path') {
      hints.push(Math.max(80, Math.round(object.durationMs / 12)))
    }
  }

  return hints.filter((value) => Number.isFinite(value) && value > 0)
}

function resolveLoopDurationMs(project) {
  const candidates = collectTimingCandidates(project).map((value) => roundTo(value, 100))
  if (candidates.length === 0) {
    return DEFAULT_LOOP_DURATION_MS
  }

  let loopDurationMs = candidates[0]
  for (let index = 1; index < candidates.length; index += 1) {
    const next = lcm(loopDurationMs, candidates[index])
    if (next > LOOP_DURATION_CAP_MS) {
      loopDurationMs = Math.max(loopDurationMs, candidates[index])
      continue
    }
    loopDurationMs = next
  }

  return clamp(loopDurationMs, DEFAULT_LOOP_DURATION_MS, LOOP_DURATION_CAP_MS)
}

export function estimateStudioCompilePlan(project) {
  const loopDurationMs = resolveLoopDurationMs(project)
  const samplingHints = collectSamplingHints(project)
  const minHint = samplingHints.length > 0 ? Math.min(...samplingHints) : DEFAULT_SAMPLE_STEP_MS
  const sampleStepMs = clamp(roundTo(minHint, 20), MIN_SAMPLE_STEP_MS, MAX_SAMPLE_STEP_MS)
  const estimatedFrameCount = Math.max(1, Math.ceil(loopDurationMs / sampleStepMs))

  return {
    loopDurationMs,
    sampleStepMs,
    estimatedFrameCount,
  }
}

export function buildStudioSummary(project) {
  const plan = estimateStudioCompilePlan(project)
  const background = project.studio.scene.background
  const widgets = project.studio.timeWidgets

  return {
    materialCount: project.studio.materials.length,
    objectCount: project.studio.scene.objects.length,
    backgroundCount: background.assetIds.length,
    minuteTriggerCount: project.studio.triggers.minute.length,
    hourTriggerCount: project.studio.triggers.hour.length,
    visibleWidgetCount: Object.values(widgets).filter((widget) => widget.visible).length,
    backgroundBehavior: background.behavior,
    ...plan,
  }
}

function canvasToPixelMap(context) {
  const imageData = context.getImageData(0, 0, MATRIX_SIZE, MATRIX_SIZE)
  const map = new Map()

  for (let y = 0; y < MATRIX_SIZE; y += 1) {
    for (let x = 0; x < MATRIX_SIZE; x += 1) {
      const index = (y * MATRIX_SIZE + x) * 4
      const r = imageData.data[index]
      const g = imageData.data[index + 1]
      const b = imageData.data[index + 2]
      const a = imageData.data[index + 3]

      if (a < COLOR_ALPHA_THRESHOLD) {
        continue
      }

      if (r === 0 && g === 0 && b === 0) {
        continue
      }

      map.set(pixelKey(x, y), [r, g, b])
    }
  }

  return map
}

async function loadRelevantRuntimes(project) {
  const relevantAssets = []
  const seen = new Set()

  for (const asset of project.studio.materials) {
    const inBackground = project.studio.scene.background.assetIds.includes(asset.id)
    const inObjects = project.studio.scene.objects.some((object) => object.assetId === asset.id)
    if (!inBackground && !inObjects) {
      continue
    }

    if (!seen.has(asset.id)) {
      relevantAssets.push(asset)
      seen.add(asset.id)
    }
  }

  const runtimes = new Map()
  const entries = await Promise.all(
    relevantAssets.map(async (asset) => {
      const runtime = await loadAssetRuntime(asset)
      return [asset.id, runtime]
    }),
  )

  for (const entry of entries) {
    runtimes.set(entry[0], entry[1])
  }

  return runtimes
}

function renderStudioScene(project, runtimes, elapsedMs) {
  const canvas = createCanvas(MATRIX_SIZE, MATRIX_SIZE)
  const context = canvas.getContext('2d', { willReadFrequently: true })
  context.clearRect(0, 0, MATRIX_SIZE, MATRIX_SIZE)
  context.fillStyle = 'rgb(0, 0, 0)'
  context.fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE)
  context.imageSmoothingEnabled = false

  const backgroundState = buildBackgroundState(project, elapsedMs)
  if (backgroundState.asset) {
    drawRuntimeFrame(context, runtimes.get(backgroundState.asset.id), backgroundState.elapsedMs, {
      x: 0,
      y: 0,
      width: MATRIX_SIZE,
      height: MATRIX_SIZE,
      fitMode: backgroundState.asset.fitMode,
      opacity: backgroundState.opacity,
    })
  }

  for (const object of project.studio.scene.objects) {
    const runtime = runtimes.get(object.assetId)
    if (!runtime) {
      continue
    }

    const state = computeObjectState(object, elapsedMs)
    if (state.opacity <= 0) {
      continue
    }

    const width = Math.max(6, object.size)
    const height = Math.max(6, Math.round((width * runtime.height) / Math.max(1, runtime.width)))

    drawRuntimeFrame(context, runtime, Math.max(0, elapsedMs - object.delayMs), {
      x: state.x,
      y: state.y,
      width,
      height,
      fitMode: 'contain',
      opacity: state.opacity,
    })
  }

  return canvasToPixelMap(context)
}

export async function compileStudioThemePackage(project) {
  const plan = estimateStudioCompilePlan(project)
  const runtimes = await loadRelevantRuntimes(project)
  const samples = Math.max(1, Math.ceil(plan.loopDurationMs / plan.sampleStepMs))
  const frames = []

  let previousMap = new Map()
  let previousRenderedMap = null
  let elapsedMs = 0

  for (let index = 0; index < samples; index += 1) {
    const remainingMs = plan.loopDurationMs - elapsedMs
    const delayMs = index === samples - 1 ? remainingMs : Math.min(plan.sampleStepMs, remainingMs)
    const currentMap = renderStudioScene(project, runtimes, elapsedMs)

    if (previousRenderedMap && mapsEqual(previousRenderedMap, currentMap)) {
      frames[frames.length - 1].d += delayMs
      elapsedMs += delayMs
      continue
    }

    const tuples = frames.length === 0 ? pixelMapToTuples(currentMap) : diffTuplesFromMaps(previousMap, currentMap)
    frames.push({
      t: frames.length === 0 ? FRAME_TYPE_FULL : FRAME_TYPE_DIFF,
      d: delayMs,
      c: tuples.length,
      p: tuples,
    })

    previousMap = new Map()
    currentMap.forEach((color, key) => {
      previousMap.set(key, cloneColor(color))
    })
    previousRenderedMap = currentMap
    elapsedMs += delayMs
  }

  if (frames.length === 0) {
    frames.push({ t: FRAME_TYPE_FULL, d: DEFAULT_LOOP_DURATION_MS, c: 0, p: [] })
  }

  return buildDeterministicThemePackage({
    name: project.name,
    f: frames.length,
    d: frames,
  })
}

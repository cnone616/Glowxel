import { OFFICIAL_TEMPLATES, getTemplateByKey } from '../data/studioCatalog'
import { diffTuplesFromMaps, pixelMapToTuples, tuplesToPixelMap } from '../utils/frameRuntime'
import { buildDeterministicThemePackage, FRAME_TYPE_DIFF, FRAME_TYPE_FULL } from '../utils/themeContract'
import { createCanvas, drawBaseFrame } from '../utils/canvasRenderer'
import { buildStudioSummary } from './studio-compiler'
import { FONT_OPTIONS, MATRIX_SIZE, PIXEL_LAYER_KEYS, TIME_PRESET_OPTIONS } from './studioSchema'

function nowIso() {
  return new Date().toISOString()
}

function randomId() {
  return crypto.randomUUID()
}

function cloneFrameTuples(frameTuples) {
  return frameTuples.map((tuple) => [...tuple])
}

function parsePixelKey(key) {
  const split = key.split(':')
  return {
    x: Number(split[0]),
    y: Number(split[1]),
  }
}

function clonePixelMap(pixelMap) {
  const nextMap = new Map()
  pixelMap.forEach((color, key) => {
    nextMap.set(key, [color[0], color[1], color[2]])
  })
  return nextMap
}

function areColorsEqual(firstColor, secondColor) {
  return (
    firstColor[0] === secondColor[0] &&
    firstColor[1] === secondColor[1] &&
    firstColor[2] === secondColor[2]
  )
}

function createTimelineFrames(frameCount) {
  const frames = []
  for (let i = 0; i < frameCount; i += 1) {
    frames.push({
      id: randomId(),
      delay: 120,
      type: i === 0 ? FRAME_TYPE_FULL : FRAME_TYPE_DIFF,
    })
  }
  return frames
}

function createTextLayer(visible, x, y, font, color) {
  return {
    visible,
    x,
    y,
    font,
    color,
  }
}

function createDefaultClockConfig() {
  return {
    time: createTextLayer(true, 10, 12, 2, { r: 255, g: 210, b: 117 }),
    date: createTextLayer(true, 10, 34, 1, { r: 255, g: 172, b: 77 }),
    week: createTextLayer(true, 10, 41, 1, { r: 47, g: 212, b: 191 }),
    showSeconds: true,
    hourFormat: 24,
  }
}

function createBlankPixelFrames(frameCount) {
  return Array.from({ length: frameCount }, () => [])
}

function clonePixelFrames(pixelFrames) {
  return pixelFrames.map((frame) => cloneFrameTuples(frame))
}

function createPublishDraft() {
  return {
    version: 'v1.0.0',
    description: '',
    tags: ['像素时钟', '本地发布'],
    coverChoice: 'cover.png',
  }
}

function createTimelineMeta(frameCount) {
  return {
    introEnabled: true,
    introEndIndex: Math.min(1, frameCount - 1),
    loopStartIndex: Math.min(2, frameCount - 1),
    frames: createTimelineFrames(frameCount),
  }
}

function createStudioWidget(visible, x, y, size, font, color) {
  return {
    visible,
    x,
    y,
    size,
    font,
    color,
  }
}

function createStudioBackgroundRule() {
  return {
    assetIds: [],
    behavior: 'static',
    intervalMs: 5000,
    description: '背景静止显示',
  }
}

function createStudioObject(objectPatch = {}) {
  return {
    id: objectPatch.id || randomId(),
    name: objectPatch.name || '新对象',
    assetId: objectPatch.assetId || '',
    category: objectPatch.category || 'object',
    visible: objectPatch.visible ?? true,
    x: objectPatch.x ?? 32,
    y: objectPatch.y ?? 32,
    size: objectPatch.size ?? 18,
    motionType: objectPatch.motionType || 'static',
    blinkIntervalMs: objectPatch.blinkIntervalMs ?? 1000,
    pathMode: objectPatch.pathMode || 'loop',
    durationMs: objectPatch.durationMs ?? 4000,
    delayMs: objectPatch.delayMs ?? 0,
    pathDescription: objectPatch.pathDescription || '',
    pathPoints: Array.isArray(objectPatch.pathPoints)
      ? objectPatch.pathPoints.map((point) => ({ x: point.x, y: point.y }))
      : [],
  }
}

function createStudioTriggerAction(actionPatch = {}, triggerKey = 'minute') {
  return {
    id: actionPatch.id || randomId(),
    label: actionPatch.label || (triggerKey === 'hour' ? '整点动作' : '分钟动作'),
    type: actionPatch.type || 'object_blink',
    targetObjectId: actionPatch.targetObjectId || '',
    durationMs: actionPatch.durationMs ?? 1000,
    description: actionPatch.description || '',
  }
}

function createStudioWorkspace(clock) {
  return {
    materials: [],
    scene: {
      background: createStudioBackgroundRule(),
      objects: [],
    },
    timeWidgets: {
      time: createStudioWidget(clock.time.visible, clock.time.x, clock.time.y, 2, clock.time.font, clock.time.color),
      date: createStudioWidget(clock.date.visible, clock.date.x, clock.date.y, 1, clock.date.font, clock.date.color),
      week: createStudioWidget(clock.week.visible, clock.week.x, clock.week.y, 1, clock.week.font, clock.week.color),
      year: createStudioWidget(false, 10, 48, 1, 1, { r: 214, g: 246, b: 255 }),
    },
    triggers: {
      minute: [],
      hour: [],
    },
    notes: '',
  }
}

function buildEmptyLayers(frameCount) {
  return {
    background: { frames: createBlankPixelFrames(frameCount) },
    motion: { frames: createBlankPixelFrames(frameCount) },
    accent: { frames: createBlankPixelFrames(frameCount) },
  }
}

function buildSeedClock(seedClock) {
  return {
    time: createTextLayer(seedClock.time.visible, seedClock.time.x, seedClock.time.y, seedClock.time.font, seedClock.time.color),
    date: createTextLayer(seedClock.date.visible, seedClock.date.x, seedClock.date.y, seedClock.date.font, seedClock.date.color),
    week: createTextLayer(seedClock.week.visible, seedClock.week.x, seedClock.week.y, seedClock.week.font, seedClock.week.color),
    showSeconds: seedClock.showSeconds,
    hourFormat: seedClock.hourFormat,
  }
}

function resolveTemplateSeed(template) {
  if (!template) {
    throw new Error('模板不存在')
  }

  const built = template.buildSeed()
  return {
    frameCount: template.frameCount,
    layers: {
      background: { frames: clonePixelFrames(built.background) },
      motion: { frames: clonePixelFrames(built.motion) },
      accent: { frames: clonePixelFrames(built.accent) },
    },
    clock: buildSeedClock(built.clock),
    paletteKey: built.paletteKey,
  }
}

function createGeneratedBackgroundAsset(templateName, frameIndex, pixelTuples) {
  const canvas = createCanvas(MATRIX_SIZE, MATRIX_SIZE)
  const context = canvas.getContext('2d', { willReadFrequently: true })
  drawBaseFrame(context, pixelTuples)

  return {
    id: randomId(),
    name: `${templateName} ${String(frameIndex + 1).padStart(2, '0')}`,
    category: 'background',
    fileName: `${templateName}-${frameIndex + 1}.png`,
    mimeType: 'image/png',
    sourceDataUrl: canvas.toDataURL('image/png'),
    width: MATRIX_SIZE,
    height: MATRIX_SIZE,
    fitMode: 'cover',
    createdAt: nowIso(),
  }
}

function buildTemplateFrameTuples(seedLayers, frameIndex) {
  const composed = new Map()

  for (let layerIndex = 0; layerIndex < PIXEL_LAYER_KEYS.length; layerIndex += 1) {
    const layerKey = PIXEL_LAYER_KEYS[layerIndex]
    const tuples = seedLayers[layerKey].frames[frameIndex]
    const layerMap = tuplesToPixelMap(tuples)
    layerMap.forEach((color, key) => {
      composed.set(key, [color[0], color[1], color[2]])
    })
  }

  return pixelMapToTuples(composed)
}

function buildTemplateStudioWorkspace(templateName, clock, seedLayers, frameCount) {
  const workspace = createStudioWorkspace(clock)
  const materials = []

  for (let frameIndex = 0; frameIndex < frameCount; frameIndex += 1) {
    materials.push(createGeneratedBackgroundAsset(templateName, frameIndex, buildTemplateFrameTuples(seedLayers, frameIndex)))
  }

  workspace.materials = materials
  workspace.scene.background.assetIds = materials.map((asset) => asset.id)
  workspace.scene.background.behavior = materials.length > 1 ? 'rotate' : 'static'
  workspace.scene.background.intervalMs = 140
  workspace.scene.background.description = `${templateName} 背景序列循环播放`
  workspace.notes = `${templateName} 模板已拆成可替换的背景序列，你可以直接替换背景素材或继续加入移动对象。`
  return workspace
}

function createBaseProject({ name, templateKey, paletteKey, frameCount, layers, clock, studio }) {
  const createdAt = nowIso()
  const project = {
    id: randomId(),
    name,
    templateKey,
    paletteKey,
    createdAt,
    updatedAt: createdAt,
    timeline: createTimelineMeta(frameCount),
    layers,
    clock,
    preview: {
      currentTime: TIME_PRESET_OPTIONS[1].value,
      currentDate: '2026-01-01',
    },
    studio: studio || createStudioWorkspace(clock),
    publishDraft: createPublishDraft(),
    themePackage: {
      name,
      f: 0,
      d: [],
    },
  }

  syncProjectThemePackage(project)
  return project
}

export function createStudioProjectFromTemplate(templateKey) {
  const template = getTemplateByKey(templateKey)
  const seed = resolveTemplateSeed(template)
  return createBaseProject({
    name: template.name,
    templateKey: template.key,
    paletteKey: seed.paletteKey,
    frameCount: seed.frameCount,
    layers: seed.layers,
    clock: seed.clock,
    studio: buildTemplateStudioWorkspace(template.name, seed.clock, seed.layers, seed.frameCount),
  })
}

export function createBlankStudioProject() {
  const frameCount = 6
  return createBaseProject({
    name: '空白主题',
    templateKey: 'blank-studio',
    paletteKey: OFFICIAL_TEMPLATES[0].buildSeed().paletteKey,
    frameCount,
    layers: buildEmptyLayers(frameCount),
    clock: createDefaultClockConfig(),
  })
}

export function normalizeStudioProject(project) {
  if (!project.preview || typeof project.preview !== 'object') {
    project.preview = {}
  }

  if (!project.preview.currentTime) {
    project.preview.currentTime = TIME_PRESET_OPTIONS[1].value
  }

  if (!project.preview.currentDate) {
    project.preview.currentDate = '2026-01-01'
  }

  if (!project.clock || typeof project.clock !== 'object') {
    project.clock = createDefaultClockConfig()
  }

  if (!project.publishDraft || typeof project.publishDraft !== 'object') {
    project.publishDraft = createPublishDraft()
  }

  if (!Array.isArray(project.publishDraft.tags)) {
    project.publishDraft.tags = ['像素时钟', '本地发布']
  }

  if (!project.studio || typeof project.studio !== 'object') {
    project.studio = createStudioWorkspace(project.clock)
    return project
  }

  if (!Array.isArray(project.studio.materials)) {
    project.studio.materials = []
  }

  if (
    project.templateKey &&
    project.templateKey !== 'blank-studio' &&
    project.studio.materials.length === 0 &&
    project.studio.scene?.objects?.length !== undefined
  ) {
    const template = getTemplateByKey(project.templateKey)
    if (template) {
      const seed = resolveTemplateSeed(template)
      const seededStudio = buildTemplateStudioWorkspace(template.name, seed.clock, seed.layers, seed.frameCount)
      project.studio.materials = seededStudio.materials
      project.studio.scene = seededStudio.scene
      project.studio.notes = project.studio.notes || seededStudio.notes
    }
  }

  if (!project.studio.scene || typeof project.studio.scene !== 'object') {
    project.studio.scene = {
      background: createStudioBackgroundRule(),
      objects: [],
    }
  }

  if (!project.studio.scene.background || typeof project.studio.scene.background !== 'object') {
    project.studio.scene.background = createStudioBackgroundRule()
  }

  if (!Array.isArray(project.studio.scene.background.assetIds)) {
    project.studio.scene.background.assetIds = []
  }

  if (!project.studio.scene.background.behavior) {
    project.studio.scene.background.behavior = 'static'
  }

  if (typeof project.studio.scene.background.intervalMs !== 'number') {
    project.studio.scene.background.intervalMs = 5000
  }

  if (typeof project.studio.scene.background.description !== 'string') {
    project.studio.scene.background.description = '背景静止显示'
  }

  if (!Array.isArray(project.studio.scene.objects)) {
    project.studio.scene.objects = []
  }

  project.studio.scene.objects = project.studio.scene.objects.map((item) => createStudioObject(item))

  if (!project.studio.timeWidgets || typeof project.studio.timeWidgets !== 'object') {
    project.studio.timeWidgets = createStudioWorkspace(project.clock).timeWidgets
  }

  const defaultWidgets = createStudioWorkspace(project.clock).timeWidgets
  for (const widgetKey of Object.keys(defaultWidgets)) {
    if (!project.studio.timeWidgets[widgetKey] || typeof project.studio.timeWidgets[widgetKey] !== 'object') {
      project.studio.timeWidgets[widgetKey] = defaultWidgets[widgetKey]
      continue
    }

    const widget = project.studio.timeWidgets[widgetKey]
    widget.visible = widget.visible ?? defaultWidgets[widgetKey].visible
    widget.x = widget.x ?? defaultWidgets[widgetKey].x
    widget.y = widget.y ?? defaultWidgets[widgetKey].y
    widget.size = widget.size ?? defaultWidgets[widgetKey].size
    widget.font = widget.font ?? defaultWidgets[widgetKey].font
    widget.color = widget.color || defaultWidgets[widgetKey].color
  }

  if (!project.studio.triggers || typeof project.studio.triggers !== 'object') {
    project.studio.triggers = { minute: [], hour: [] }
  }

  if (!Array.isArray(project.studio.triggers.minute)) {
    project.studio.triggers.minute = []
  }
  if (!Array.isArray(project.studio.triggers.hour)) {
    project.studio.triggers.hour = []
  }

  project.studio.triggers.minute = project.studio.triggers.minute.map((action) => createStudioTriggerAction(action, 'minute'))
  project.studio.triggers.hour = project.studio.triggers.hour.map((action) => createStudioTriggerAction(action, 'hour'))

  if (typeof project.studio.notes !== 'string') {
    project.studio.notes = ''
  }

  return project
}

export function createStudioObjectFromAsset(asset) {
  return createStudioObject({
    name: asset.name,
    assetId: asset.id,
    category: asset.category,
  })
}

export function createTriggerAction(triggerKey) {
  return createStudioTriggerAction({}, triggerKey)
}

export function buildCanvasTextLayout(project) {
  const widgets = project.studio?.timeWidgets || createStudioWorkspace(project.clock).timeWidgets
  return {
    hourFormat: project.clock.hourFormat,
    showSeconds: project.clock.showSeconds,
    time: widgets.time,
    date: widgets.date,
    week: widgets.week,
    year: widgets.year,
  }
}

function getProjectFrameCount(project) {
  return project.timeline.frames.length
}

export function composePixelFrameMap(project, frameIndex) {
  const composed = new Map()

  for (let layerIndex = 0; layerIndex < PIXEL_LAYER_KEYS.length; layerIndex += 1) {
    const layerKey = PIXEL_LAYER_KEYS[layerIndex]
    const tuples = project.layers[layerKey].frames[frameIndex]
    const layerMap = tuplesToPixelMap(tuples)
    layerMap.forEach((color, key) => {
      composed.set(key, [color[0], color[1], color[2]])
    })
  }

  return composed
}

export function buildTweenFramesFromTuples(startFrameTuples, endFrameTuples, stepCount) {
  if (stepCount < 1) {
    return []
  }

  const startMap = tuplesToPixelMap(startFrameTuples)
  const endMap = tuplesToPixelMap(endFrameTuples)
  const allKeys = new Set()

  startMap.forEach((_, key) => {
    allKeys.add(key)
  })

  endMap.forEach((_, key) => {
    allKeys.add(key)
  })

  const changedKeys = Array.from(allKeys)
    .filter((key) => {
      const startColor = startMap.has(key) ? startMap.get(key) : [0, 0, 0]
      const endColor = endMap.has(key) ? endMap.get(key) : [0, 0, 0]
      return !areColorsEqual(startColor, endColor)
    })
    .sort((firstKey, secondKey) => {
      const first = parsePixelKey(firstKey)
      const second = parsePixelKey(secondKey)
      if (first.y !== second.y) {
        return first.y - second.y
      }
      return first.x - second.x
    })

  const tweenFrames = []

  for (let step = 1; step <= stepCount; step += 1) {
    const progressCount = Math.ceil((changedKeys.length * step) / (stepCount + 1))
    const workingMap = clonePixelMap(startMap)

    for (let keyIndex = 0; keyIndex < progressCount; keyIndex += 1) {
      const key = changedKeys[keyIndex]
      if (endMap.has(key)) {
        const color = endMap.get(key)
        workingMap.set(key, [color[0], color[1], color[2]])
      } else {
        workingMap.delete(key)
      }
    }

    tweenFrames.push(pixelMapToTuples(workingMap))
  }

  return tweenFrames
}

export function buildThemePackageFromProject(project) {
  const frames = []
  let previousFrameMap = new Map()

  for (let frameIndex = 0; frameIndex < getProjectFrameCount(project); frameIndex += 1) {
    const frameMeta = project.timeline.frames[frameIndex]
    const currentMap = composePixelFrameMap(project, frameIndex)

    let tuples
    if (frameMeta.type === FRAME_TYPE_FULL || frameIndex === 0) {
      tuples = pixelMapToTuples(currentMap)
    } else {
      tuples = diffTuplesFromMaps(previousFrameMap, currentMap)
    }

    frames.push({
      t: frameMeta.type,
      d: frameMeta.delay,
      c: tuples.length,
      p: tuples,
    })

    previousFrameMap = currentMap
  }

  return buildDeterministicThemePackage({
    name: project.name,
    f: frames.length,
    d: frames,
  })
}

export function syncProjectThemePackage(project) {
  normalizeStudioProject(project)
  project.updatedAt = nowIso()
  project.themePackage = buildThemePackageFromProject(project)
  return project
}

export function cloneProject(project) {
  return structuredClone(project)
}

export function createProjectSummary(project) {
  normalizeStudioProject(project)
  const studioSummary = buildStudioSummary(project)
  return {
    id: project.id,
    name: project.name,
    templateKey: project.templateKey,
    updatedAt: project.updatedAt,
    frameCount: studioSummary.estimatedFrameCount,
    totalPixels: project.themePackage.d.reduce((sum, frame) => sum + frame.c, 0),
    previewTime: project.preview.currentTime,
    materialCount: studioSummary.materialCount,
    objectCount: studioSummary.objectCount,
  }
}

export function parsePreviewTime(project) {
  const parts = project.preview.currentTime.split(':').map((value) => Number(value))
  const date = new Date(`${project.preview.currentDate}T00:00:00`)
  date.setHours(parts[0], parts[1], parts[2] || 0, 0)
  return date
}

export function ensureFontId(fontId) {
  const matched = FONT_OPTIONS.find((font) => font.id === fontId)
  if (!matched) {
    throw new Error('未支持的字体 ID')
  }
  return fontId
}

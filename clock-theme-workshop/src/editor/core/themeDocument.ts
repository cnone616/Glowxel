import type {
  AssetRecord,
  BackgroundLayer,
  BackgroundStateKey,
  BackgroundTransformKey,
  GroupStateKey,
  GroupTransformKey,
  LayerTiming,
  ObjectGroup,
  ObjectMember,
  PixelLayer,
  Point,
  Scene,
  SelectionKind,
  ThemeDocument,
  ThemeDocumentMeta,
  ThemeRules,
  TimeWidget,
} from '../types'
import { fitRect } from './math'
import { uid } from './id'

const LEGACY_DOCUMENT_VERSION = 'ThemeWorkshopDocumentV2'
export const DOCUMENT_VERSION = 'ThemeWorkshopDocumentV3'
export const DEFAULT_FADE_FRAMES = 6
const DEFAULT_SCENE_DURATION = 24

interface ThemeDocumentV2 {
  version: 'ThemeWorkshopDocumentV2'
  meta: ThemeDocumentMeta & { frameCount: number }
  assets: AssetRecord[]
  backgroundLayers: BackgroundLayer[]
  objectGroups: Array<Omit<ObjectGroup, 'timing'>>
  timeWidgets: Array<Omit<TimeWidget, 'timing'>>
}

export function sortKeys<T extends { frame: number }>(keys: T[]): T[] {
  return keys.slice().sort((first, second) => first.frame - second.frame)
}

export function hasKeyframe<T extends { frame: number }>(keys: T[], frame: number): boolean {
  return keys.some((key) => key.frame === frame)
}

export function upsertKeyframe<T extends { frame: number }>(
  keys: T[],
  frame: number,
  patch: Omit<Partial<T>, 'frame'>,
): T[] {
  const next: T[] = []
  let found = false

  for (const key of keys) {
    if (key.frame === frame) {
      next.push({ ...key, ...patch, frame } as T)
      found = true
      continue
    }
    next.push(key)
  }

  if (!found) {
    next.push({ frame, ...patch } as T)
  }

  return sortKeys(next)
}

export function removeKeyframe<T extends { frame: number }>(keys: T[], frame: number): T[] {
  return sortKeys(keys.filter((key) => key.frame !== frame))
}

function interpolateValue(previousValue: unknown, nextValue: unknown, ratio: number): unknown {
  if (typeof previousValue === 'number' && typeof nextValue === 'number') {
    return previousValue + (nextValue - previousValue) * ratio
  }

  return previousValue
}

export function sampleContinuousKeys<T extends { frame: number }>(
  keys: T[],
  frame: number,
  defaults: Omit<T, 'frame'>,
): T {
  if (keys.length === 0) {
    return { frame, ...defaults } as T
  }

  const ordered = sortKeys(keys)
  const exact = ordered.find((key) => key.frame === frame)
  let previous: T | null = null
  let next: T | null = null

  for (const key of ordered) {
    if (key.frame < frame) {
      previous = key
      continue
    }

    if (key.frame > frame) {
      next = key
      break
    }
  }

  const sampled: Record<string, unknown> = { ...defaults }
  if (!previous && next) {
    Object.assign(sampled, next)
  } else if (previous && !next) {
    Object.assign(sampled, previous)
  } else if (previous && next) {
    const ratio = (frame - previous.frame) / (next.frame - previous.frame)
    const keySet = new Set<string>([
      ...Object.keys(defaults),
      ...Object.keys(previous),
      ...Object.keys(next),
    ])

    for (const key of keySet) {
      if (key === 'frame') {
        continue
      }
      sampled[key] = interpolateValue(previous[key as keyof T], next[key as keyof T], ratio)
    }
  }

  if (exact) {
    const { frame: _ignoredFrame, ...exactValue } = exact
    return { ...sampled, ...exactValue, frame } as T
  }

  return { frame, ...sampled } as T
}

export function sampleStepKeys<T extends { frame: number }>(
  keys: T[],
  frame: number,
  defaults: Omit<T, 'frame'>,
): T {
  if (keys.length === 0) {
    return { frame, ...defaults } as T
  }

  const ordered = sortKeys(keys)
  const exact = ordered.find((key) => key.frame === frame)
  let chosen = ordered[0]
  let previous: T | null = null

  for (const key of ordered) {
    if (key.frame < frame) {
      previous = key
      chosen = key
      continue
    }
    if (key.frame === frame) {
      chosen = key
      break
    }
    break
  }

  if (exact) {
    const { frame: _ignoredFrame, ...exactValue } = exact
    return { ...defaults, ...(previous ?? {}), ...exactValue, frame } as T
  }

  return { ...defaults, ...chosen, frame: chosen.frame } as T
}

export function clampSceneDuration(durationFrames: number): number {
  return Math.max(8, Math.round(durationFrames))
}

export function clampLayerTiming(timing: LayerTiming, durationFrames: number): LayerTiming {
  const maxFrame = Math.max(0, clampSceneDuration(durationFrames) - 1)
  const startFrame = Math.max(0, Math.min(maxFrame, Math.round(timing.startFrame)))
  const endFrame = Math.max(startFrame, Math.min(maxFrame, Math.round(timing.endFrame)))

  return {
    startFrame,
    endFrame,
  }
}

export function createDefaultThemeRules(): ThemeRules {
  return {
    paletteLock: false,
    matrixBound: {
      width: 64,
      height: 64,
    },
    pixelGridSize: 1,
  }
}

export function createLayerTiming(durationFrames: number): LayerTiming {
  return {
    startFrame: 0,
    endFrame: clampSceneDuration(durationFrames) - 1,
  }
}

export function createEmptyBitmap(): number[] {
  return new Array(64 * 64 * 4).fill(0)
}

function getAssetFrameSize(asset: AssetRecord | null): { width: number; height: number } {
  if (!asset) {
    return { width: 12, height: 12 }
  }

  if (asset.kind === 'uploaded-image') {
    return { width: asset.width, height: asset.height }
  }

  const firstFrame = asset.frames?.[0]
  if (firstFrame) {
    return { width: firstFrame.width, height: firstFrame.height }
  }

  return { width: 12, height: 12 }
}

function createBaseBackgroundLayer(): BackgroundLayer {
  return {
    id: uid('background'),
    name: 'Base Wash',
    assetId: '',
    baseColor: '#081318',
    locked: false,
    transformKeys: [
      {
        frame: 0,
        x: 0,
        y: 0,
        width: 64,
        height: 64,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: 'fixed',
        rhythmFrames: 6,
        speedX: 0,
        speedY: 0,
        sequenceStepFrames: 1,
      },
    ],
  }
}

function createObjectMember(asset: AssetRecord): ObjectMember {
  return {
    id: uid('member'),
    name: asset.label,
    assetId: asset.id,
    offsetX: 0,
    offsetY: 0,
    width: asset.width,
    height: asset.height,
    opacity: 1,
    visible: true,
  }
}

export function createBackgroundLayer(asset: AssetRecord | null): BackgroundLayer {
  const assetSize = getAssetFrameSize(asset)
  const fitted = fitRect(assetSize.width, assetSize.height, 64, 64)
  const defaultBehavior = asset?.kind === 'catalog-sequence' ? 'loop' : 'fixed'

  return {
    id: uid('background'),
    name: asset ? `${asset.label} Background` : 'Background',
    assetId: asset ? asset.id : '',
    baseColor: '#081318',
    locked: false,
    transformKeys: [
      {
        frame: 0,
        x: fitted.x,
        y: fitted.y,
        width: fitted.width,
        height: fitted.height,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: defaultBehavior,
        rhythmFrames: 6,
        speedX: 0,
        speedY: 0,
        sequenceStepFrames: 1,
      },
    ],
  }
}

export function createObjectGroup(asset: AssetRecord | null, durationFrames = DEFAULT_SCENE_DURATION): ObjectGroup {
  const assetSize = getAssetFrameSize(asset)
  const fitted = fitRect(assetSize.width, assetSize.height, 28, 28)
  const x = Math.floor((64 - fitted.width) / 2)
  const y = Math.floor((64 - fitted.height) / 2)

  return {
    id: uid('object'),
    name: asset ? asset.label : 'Object Group',
    locked: false,
    timing: createLayerTiming(durationFrames),
    transformKeys: [
      {
        frame: 0,
        x,
        y,
        scale: 1,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: 'fixed',
        rhythmFrames: 6,
        sequenceStepFrames: 1,
      },
    ],
    motion: {
      mode: 'loop',
      durationFrames: 24,
      points: asset
        ? [
            {
              id: uid('path'),
              x,
              y,
            },
          ]
        : [],
    },
    members: asset ? [createObjectMember(asset)] : [],
  }
}

export function createPixelLayer(durationFrames = DEFAULT_SCENE_DURATION): PixelLayer {
  return {
    id: uid('pixel'),
    name: 'Pixel Layer',
    visible: true,
    locked: false,
    x: 0,
    y: 0,
    opacity: 1,
    zIndex: 0,
    bitmap: createEmptyBitmap(),
    timing: createLayerTiming(durationFrames),
  }
}

export function createTimeWidgets(durationFrames = DEFAULT_SCENE_DURATION): TimeWidget[] {
  const timing = createLayerTiming(durationFrames)

  return [
    {
      id: uid('time'),
      type: 'time',
      label: 'TIME',
      visible: true,
      locked: false,
      x: 4,
      y: 7,
      scale: 2,
      letterSpacing: 0,
      color: '#ffffff',
      hourFormat: 24,
      showSeconds: false,
      timing,
    },
    {
      id: uid('time'),
      type: 'date',
      label: 'DATE',
      visible: true,
      locked: false,
      x: 4,
      y: 54,
      scale: 1,
      letterSpacing: 0,
      color: '#7ef2d7',
      hourFormat: 24,
      showSeconds: false,
      timing,
    },
    {
      id: uid('time'),
      type: 'week',
      label: 'WEEK',
      visible: true,
      locked: false,
      x: 40,
      y: 54,
      scale: 1,
      letterSpacing: 0,
      color: '#e0be72',
      hourFormat: 24,
      showSeconds: false,
      timing,
    },
    {
      id: uid('time'),
      type: 'year',
      label: 'YEAR',
      visible: false,
      locked: false,
      x: 4,
      y: 46,
      scale: 1,
      letterSpacing: 0,
      color: '#d0d9e4',
      hourFormat: 24,
      showSeconds: false,
      timing,
    },
  ]
}

export function createScene(name?: string, durationFrames = DEFAULT_SCENE_DURATION): Scene {
  const normalizedDuration = clampSceneDuration(durationFrames)

  return {
    id: uid('scene'),
    name: name ?? 'Scene',
    durationFrames: normalizedDuration,
    transitionOut: 'cut',
    backgroundLayers: [createBaseBackgroundLayer()],
    objectGroups: [],
    pixelLayers: [],
    timeWidgets: createTimeWidgets(normalizedDuration),
  }
}

export function createDefaultDocument(): ThemeDocument {
  const firstScene = createScene('Scene 1')

  return {
    version: DOCUMENT_VERSION,
    meta: {
      name: 'Glowxel Theme',
      matrix: {
        width: 64,
        height: 64,
      },
      frameDelay: 140,
      previewMode: 'loop',
      previewDate: '2026-04-22',
      previewTime: '12:34:56',
      updatedAt: new Date().toISOString(),
    },
    scenes: [firstScene],
    materials: [],
    themeRules: createDefaultThemeRules(),
  }
}

function migrateV2ObjectGroup(group: ThemeDocumentV2['objectGroups'][number], durationFrames: number): ObjectGroup {
  return {
    ...group,
    timing: createLayerTiming(durationFrames),
  }
}

function migrateV2TimeWidget(widget: ThemeDocumentV2['timeWidgets'][number], durationFrames: number): TimeWidget {
  return {
    ...widget,
    timing: createLayerTiming(durationFrames),
  }
}

function migrateV2Document(documentV2: ThemeDocumentV2): ThemeDocument {
  const durationFrames = clampSceneDuration(documentV2.meta.frameCount)
  const scene: Scene = {
    id: uid('scene'),
    name: 'Scene 1',
    durationFrames,
    transitionOut: 'cut',
    backgroundLayers: documentV2.backgroundLayers,
    objectGroups: documentV2.objectGroups.map((group) => migrateV2ObjectGroup(group, durationFrames)),
    pixelLayers: [],
    timeWidgets: documentV2.timeWidgets.map((widget) => migrateV2TimeWidget(widget, durationFrames)),
  }

  return {
    version: DOCUMENT_VERSION,
    meta: {
      name: documentV2.meta.name,
      matrix: documentV2.meta.matrix,
      frameDelay: documentV2.meta.frameDelay,
      previewMode: documentV2.meta.previewMode,
      previewDate: documentV2.meta.previewDate,
      previewTime: documentV2.meta.previewTime,
      updatedAt: documentV2.meta.updatedAt,
    },
    scenes: [scene],
    materials: documentV2.assets,
    themeRules: createDefaultThemeRules(),
  }
}

function isThemeDocumentV3(raw: unknown): raw is ThemeDocument {
  if (typeof raw !== 'object' || raw === null) {
    return false
  }

  const candidate = raw as ThemeDocument
  return candidate.version === DOCUMENT_VERSION
}

function isThemeDocumentV2(raw: unknown): raw is ThemeDocumentV2 {
  if (typeof raw !== 'object' || raw === null) {
    return false
  }

  const candidate = raw as ThemeDocumentV2
  return candidate.version === LEGACY_DOCUMENT_VERSION
}

export function ensureThemeDocument(raw: unknown): ThemeDocument {
  if (isThemeDocumentV3(raw)) {
    if (!Array.isArray(raw.scenes)) {
      throw new Error('导入文件缺少 scenes 数组')
    }
    if (!Array.isArray(raw.materials)) {
      throw new Error('导入文件缺少 materials 数组')
    }
    if (typeof raw.themeRules !== 'object' || raw.themeRules === null) {
      throw new Error('导入文件缺少 themeRules')
    }

    return raw
  }

  if (isThemeDocumentV2(raw)) {
    if (!Array.isArray(raw.assets)) {
      throw new Error('导入文件缺少 assets 数组')
    }
    if (!Array.isArray(raw.backgroundLayers)) {
      throw new Error('导入文件缺少 backgroundLayers 数组')
    }
    if (!Array.isArray(raw.objectGroups)) {
      throw new Error('导入文件缺少 objectGroups 数组')
    }
    if (!Array.isArray(raw.timeWidgets)) {
      throw new Error('导入文件缺少 timeWidgets 数组')
    }

    return migrateV2Document(raw)
  }

  throw new Error(`只支持 ${DOCUMENT_VERSION} 或 ${LEGACY_DOCUMENT_VERSION} 格式`)
}

export function getDocumentFrameCount(document: ThemeDocument): number {
  return document.scenes.reduce((total, scene) => total + clampSceneDuration(scene.durationFrames), 0)
}

export function getSceneById(document: ThemeDocument, sceneId: string): Scene | null {
  return document.scenes.find((scene) => scene.id === sceneId) ?? null
}

export function getSceneIndexById(document: ThemeDocument, sceneId: string): number {
  return document.scenes.findIndex((scene) => scene.id === sceneId)
}

export function getInitialSceneId(document: ThemeDocument): string {
  return document.scenes[0]?.id ?? ''
}

export function resolveSceneFrame(document: ThemeDocument, globalFrame: number): {
  scene: Scene
  sceneIndex: number
  sceneStartFrame: number
  sceneEndFrame: number
  localFrame: number
} {
  const frameCount = getDocumentFrameCount(document)
  const safeFrame = Math.max(0, Math.min(Math.max(0, frameCount - 1), Math.round(globalFrame)))
  let offset = 0

  for (let index = 0; index < document.scenes.length; index += 1) {
    const scene = document.scenes[index]
    const durationFrames = clampSceneDuration(scene.durationFrames)
    const endFrame = offset + durationFrames - 1
    if (safeFrame <= endFrame) {
      return {
        scene,
        sceneIndex: index,
        sceneStartFrame: offset,
        sceneEndFrame: endFrame,
        localFrame: safeFrame - offset,
      }
    }
    offset += durationFrames
  }

  const fallbackScene = document.scenes[Math.max(0, document.scenes.length - 1)] ?? createScene('Scene 1')
  const durationFrames = clampSceneDuration(fallbackScene.durationFrames)

  return {
    scene: fallbackScene,
    sceneIndex: Math.max(0, document.scenes.length - 1),
    sceneStartFrame: Math.max(0, frameCount - durationFrames),
    sceneEndFrame: Math.max(0, frameCount - 1),
    localFrame: Math.max(0, durationFrames - 1),
  }
}

export function getActiveScene(document: ThemeDocument, activeSceneId: string): Scene {
  return getSceneById(document, activeSceneId) ?? document.scenes[0]
}

export function isLayerVisibleAtFrame(timing: LayerTiming, frame: number): boolean {
  return frame >= timing.startFrame && frame <= timing.endFrame
}

export function normalizeScene(scene: Scene): Scene {
  const durationFrames = clampSceneDuration(scene.durationFrames)

  return {
    ...scene,
    durationFrames,
    objectGroups: scene.objectGroups.map((group) => ({
      ...group,
      timing: clampLayerTiming(group.timing, durationFrames),
    })),
    pixelLayers: scene.pixelLayers.map((layer, index) => ({
      ...layer,
      zIndex: index,
      timing: clampLayerTiming(layer.timing, durationFrames),
    })),
    timeWidgets: scene.timeWidgets.map((widget) => ({
      ...widget,
      timing: clampLayerTiming(widget.timing, durationFrames),
    })),
  }
}

export function normalizeDocument(document: ThemeDocument): ThemeDocument {
  const usedTimeWidgetIds = new Set<string>()
  const scenes = document.scenes.map((scene) => {
    const normalizedScene = normalizeScene(scene)
    return {
      ...normalizedScene,
      timeWidgets: normalizedScene.timeWidgets.map((widget) => {
        if (widget.id.length > 0 && !usedTimeWidgetIds.has(widget.id)) {
          usedTimeWidgetIds.add(widget.id)
          return widget
        }

        const nextId = uid('time')
        usedTimeWidgetIds.add(nextId)
        return {
          ...widget,
          id: nextId,
        }
      }),
    }
  })

  return {
    ...document,
    scenes,
  }
}

export function findSceneForSelection(
  document: ThemeDocument,
  selectionKind: SelectionKind,
  selectionId: string,
): Scene | null {
  if (selectionKind === 'scene') {
    return getSceneById(document, selectionId)
  }

  if (selectionKind === 'material') {
    return null
  }

  for (const scene of document.scenes) {
    if (selectionKind === 'background' && scene.backgroundLayers.some((layer) => layer.id === selectionId)) {
      return scene
    }
    if (selectionKind === 'object' && scene.objectGroups.some((group) => group.id === selectionId)) {
      return scene
    }
    if (selectionKind === 'pixel' && scene.pixelLayers.some((layer) => layer.id === selectionId)) {
      return scene
    }
    if (selectionKind === 'time' && scene.timeWidgets.some((widget) => widget.id === selectionId)) {
      return scene
    }
  }

  return null
}

export const DEFAULT_BACKGROUND_TRANSFORM: Omit<BackgroundTransformKey, 'frame'> = {
  x: 0,
  y: 0,
  width: 64,
  height: 64,
  opacity: 1,
}

export const DEFAULT_BACKGROUND_STATE: Omit<BackgroundStateKey, 'frame'> = {
  visible: true,
  behavior: 'fixed',
  rhythmFrames: 6,
  speedX: 0,
  speedY: 0,
  sequenceStepFrames: 1,
}

export const DEFAULT_GROUP_TRANSFORM: Omit<GroupTransformKey, 'frame'> = {
  x: 20,
  y: 20,
  scale: 1,
  opacity: 1,
}

export const DEFAULT_GROUP_STATE: Omit<GroupStateKey, 'frame'> = {
  visible: true,
  behavior: 'fixed',
  rhythmFrames: 6,
  sequenceStepFrames: 1,
}

export function shiftPoint(point: Point, delta: Point): Point {
  return {
    x: point.x + delta.x,
    y: point.y + delta.y,
  }
}

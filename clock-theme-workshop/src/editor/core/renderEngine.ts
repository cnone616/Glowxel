import type {
  AssetRecord,
  BackgroundLayer,
  CatalogFrame,
  DrawableBackground,
  DrawableMember,
  DrawableObjectGroup,
  DrawablePixelLayer,
  DrawableTimeWidget,
  ObjectGroup,
  ObjectMember,
  PathPoint,
  Point,
  Rect,
  RenderSnapshot,
  Scene,
  ThemeDocument,
  TimeWidget,
  TransitionOverlay,
} from '../types'
import { formatWidgetText, measurePixelText } from './pixelFont'
import {
  DEFAULT_BACKGROUND_STATE,
  DEFAULT_BACKGROUND_TRANSFORM,
  DEFAULT_FADE_FRAMES,
  DEFAULT_GROUP_STATE,
  DEFAULT_GROUP_TRANSFORM,
  getDocumentFrameCount,
  isLayerVisibleAtFrame,
  resolveSceneFrame,
  sampleContinuousKeys,
  sampleStepKeys,
} from './themeDocument'
import { clamp, distance } from './math'

function parsePreviewDate(previewDate: string, previewTime: string): Date {
  const parsed = new Date(`${previewDate}T${previewTime}`)
  if (Number.isNaN(parsed.getTime())) {
    return new Date('2026-04-22T12:34:56')
  }

  return parsed
}

export function buildPreviewDate(document: ThemeDocument, currentFrame: number): Date {
  const previewDate = parsePreviewDate(document.meta.previewDate, document.meta.previewTime)

  if (document.meta.previewMode === 'minute') {
    previewDate.setMinutes(previewDate.getMinutes() + 1)
  }

  if (document.meta.previewMode === 'hour') {
    previewDate.setHours(previewDate.getHours() + 1)
    previewDate.setMinutes(0)
    previewDate.setSeconds(0)
  }

  previewDate.setMilliseconds(0)
  previewDate.setTime(previewDate.getTime() + currentFrame * document.meta.frameDelay)
  return previewDate
}

function getAsset(document: ThemeDocument, assetId: string): AssetRecord | null {
  return document.materials.find((asset) => asset.id === assetId) ?? null
}

export function selectAssetFrame(
  asset: AssetRecord | null,
  frameIndex: number,
  sequenceStepFrames: number,
): CatalogFrame | AssetRecord | null {
  if (!asset) {
    return null
  }

  if (asset.kind === 'uploaded-image') {
    return asset
  }

  if (!asset.frames || asset.frames.length === 0) {
    return null
  }

  const step = Math.max(1, sequenceStepFrames)
  const index = asset.frames.length === 1 ? 0 : Math.floor(frameIndex / step) % asset.frames.length
  return asset.frames[index]
}

function resolveObjectMotionPoint(
  points: PathPoint[],
  frame: number,
  durationFrames: number,
  mode: ObjectGroup['motion']['mode'],
): Point | null {
  if (points.length === 0) {
    return null
  }

  if (points.length === 1) {
    return { x: points[0].x, y: points[0].y }
  }

  const segments = []
  let totalLength = 0

  for (let index = 0; index < points.length - 1; index += 1) {
    const first = points[index]
    const second = points[index + 1]
    const segmentLength = distance(first, second)
    segments.push({ first, second, length: segmentLength })
    totalLength += segmentLength
  }

  if (totalLength === 0) {
    return { x: points[0].x, y: points[0].y }
  }

  const normalizedDuration = Math.max(1, durationFrames)
  let progress = frame / normalizedDuration

  if (mode === 'loop') {
    progress %= 1
  } else if (mode === 'pingpong') {
    const cycle = progress % 2
    progress = cycle > 1 ? 2 - cycle : cycle
  } else {
    progress = clamp(progress, 0, 1)
  }

  const targetDistance = progress * totalLength
  let travelled = 0
  for (const segment of segments) {
    if (travelled + segment.length >= targetDistance) {
      const segmentProgress =
        segment.length === 0 ? 0 : (targetDistance - travelled) / segment.length
      return {
        x: segment.first.x + (segment.second.x - segment.first.x) * segmentProgress,
        y: segment.first.y + (segment.second.y - segment.first.y) * segmentProgress,
      }
    }
    travelled += segment.length
  }

  return { x: points[points.length - 1].x, y: points[points.length - 1].y }
}

function buildObjectBounds(members: DrawableMember[]): Rect | null {
  if (members.length === 0) {
    return null
  }

  let minX = Number.POSITIVE_INFINITY
  let minY = Number.POSITIVE_INFINITY
  let maxX = Number.NEGATIVE_INFINITY
  let maxY = Number.NEGATIVE_INFINITY

  for (const member of members) {
    minX = Math.min(minX, member.x)
    minY = Math.min(minY, member.y)
    maxX = Math.max(maxX, member.x + member.width)
    maxY = Math.max(maxY, member.y + member.height)
  }

  return {
    x: minX,
    y: minY,
    width: maxX - minX,
    height: maxY - minY,
  }
}

function buildBackgroundDrawables(
  document: ThemeDocument,
  scene: Scene,
  localFrame: number,
): DrawableBackground[] {
  return scene.backgroundLayers.map((layer: BackgroundLayer) => ({
    id: layer.id,
    name: layer.name,
    baseColor: layer.baseColor,
    asset: getAsset(document, layer.assetId),
    transform: sampleContinuousKeys(layer.transformKeys, localFrame, DEFAULT_BACKGROUND_TRANSFORM),
    state: sampleStepKeys(layer.stateKeys, localFrame, DEFAULT_BACKGROUND_STATE),
  }))
}

function buildDrawableMember(
  document: ThemeDocument,
  member: ObjectMember,
  transform: DrawableObjectGroup['transform'],
  motionOffset: Point | null,
): DrawableMember {
  return {
    ...member,
    asset: getAsset(document, member.assetId),
    x: Math.round((motionOffset?.x ?? transform.x) + member.offsetX * transform.scale),
    y: Math.round((motionOffset?.y ?? transform.y) + member.offsetY * transform.scale),
  }
}

function buildObjectDrawables(
  document: ThemeDocument,
  scene: Scene,
  localFrame: number,
): DrawableObjectGroup[] {
  return scene.objectGroups
    .filter((group) => isLayerVisibleAtFrame(group.timing, localFrame))
    .map((group) => {
      const transform = sampleContinuousKeys(group.transformKeys, localFrame, DEFAULT_GROUP_TRANSFORM)
      const state = sampleStepKeys(group.stateKeys, localFrame, DEFAULT_GROUP_STATE)
      const motionOffset =
        state.behavior === 'path'
          ? resolveObjectMotionPoint(group.motion.points, localFrame, group.motion.durationFrames, group.motion.mode)
          : null
      const members = group.members.map((member) =>
        buildDrawableMember(document, member, transform, motionOffset),
      )

      return {
        id: group.id,
        name: group.name,
        transform: motionOffset ? { ...transform, x: motionOffset.x, y: motionOffset.y } : transform,
        state,
        motion: group.motion,
        members,
        bounds: buildObjectBounds(
          members.map((member) => ({
            ...member,
            width: Math.max(1, Math.round(member.width * transform.scale)),
            height: Math.max(1, Math.round(member.height * transform.scale)),
          })),
        ),
      }
    })
}

function buildPixelLayerDrawables(scene: Scene, localFrame: number): DrawablePixelLayer[] {
  return scene.pixelLayers
    .filter((layer) => layer.visible && isLayerVisibleAtFrame(layer.timing, localFrame))
    .sort((first, second) => first.zIndex - second.zIndex)
    .map((layer) => ({
      id: layer.id,
      name: layer.name,
      x: layer.x,
      y: layer.y,
      opacity: layer.opacity,
      zIndex: layer.zIndex,
      bitmap: layer.bitmap,
      bounds: {
        x: layer.x,
        y: layer.y,
        width: 64,
        height: 64,
      },
    }))
}

function buildWidgetDrawables(scene: Scene, previewDate: Date, localFrame: number): DrawableTimeWidget[] {
  return scene.timeWidgets
    .filter((widget) => widget.visible && isLayerVisibleAtFrame(widget.timing, localFrame))
    .map((widget: TimeWidget) => {
      const text = formatWidgetText(widget, previewDate)
      const bounds = measurePixelText(text, widget.scale, widget.letterSpacing)

      return {
        ...widget,
        text,
        bounds: {
          x: widget.x,
          y: widget.y,
          width: bounds.width,
          height: bounds.height,
        },
      }
    })
}

function buildSceneLayerPack(
  document: ThemeDocument,
  scene: Scene,
  localFrame: number,
  previewDate: Date,
): Omit<TransitionOverlay, 'alpha'> {
  return {
    sceneId: scene.id,
    localFrame,
    backgrounds: buildBackgroundDrawables(document, scene, localFrame),
    pixelLayers: buildPixelLayerDrawables(scene, localFrame),
    objects: buildObjectDrawables(document, scene, localFrame),
    widgets: buildWidgetDrawables(scene, previewDate, localFrame),
  }
}

function buildTransitionOverlay(
  document: ThemeDocument,
  sceneIndex: number,
  scene: Scene,
  localFrame: number,
  previewDate: Date,
): { baseAlpha: number; overlay: TransitionOverlay | null } {
  if (scene.transitionOut !== 'fade') {
    return { baseAlpha: 1, overlay: null }
  }

  const nextScene = document.scenes[sceneIndex + 1]
  if (!nextScene) {
    return { baseAlpha: 1, overlay: null }
  }

  const fadeWindow = Math.min(DEFAULT_FADE_FRAMES, Math.max(1, scene.durationFrames))
  const fadeStartFrame = scene.durationFrames - fadeWindow
  if (localFrame < fadeStartFrame) {
    return { baseAlpha: 1, overlay: null }
  }

  const fadeProgress = (localFrame - fadeStartFrame + 1) / fadeWindow
  const overlayLocalFrame = Math.min(nextScene.durationFrames - 1, Math.max(0, localFrame - fadeStartFrame))
  const overlayPack = buildSceneLayerPack(document, nextScene, overlayLocalFrame, previewDate)

  return {
    baseAlpha: 1 - fadeProgress,
    overlay: {
      ...overlayPack,
      alpha: fadeProgress,
    },
  }
}

export class RenderEngine {
  buildSnapshot(document: ThemeDocument, currentFrame: number): RenderSnapshot {
    const previewDate = buildPreviewDate(document, currentFrame)
    const frameInfo = resolveSceneFrame(document, currentFrame)
    const basePack = buildSceneLayerPack(document, frameInfo.scene, frameInfo.localFrame, previewDate)
    const transition = buildTransitionOverlay(
      document,
      frameInfo.sceneIndex,
      frameInfo.scene,
      frameInfo.localFrame,
      previewDate,
    )

    return {
      matrix: document.meta.matrix,
      previewDate,
      sceneId: frameInfo.scene.id,
      sceneName: frameInfo.scene.name,
      sceneFrame: frameInfo.localFrame,
      sceneDurationFrames: frameInfo.scene.durationFrames,
      documentFrameCount: getDocumentFrameCount(document),
      baseAlpha: transition.baseAlpha,
      backgrounds: basePack.backgrounds,
      pixelLayers: basePack.pixelLayers,
      objects: basePack.objects,
      widgets: basePack.widgets,
      transitionOverlay: transition.overlay,
    }
  }
}

function pixelLayerContainsPoint(layer: DrawablePixelLayer, point: Point): boolean {
  const localX = point.x - layer.x
  const localY = point.y - layer.y

  if (localX < 0 || localY < 0 || localX >= 64 || localY >= 64) {
    return false
  }

  const alphaIndex = (localY * 64 + localX) * 4 + 3
  return layer.bitmap[alphaIndex] > 0
}

export function hitTestSnapshot(snapshot: RenderSnapshot, point: Point): {
  kind: 'background' | 'object' | 'pixel' | 'time' | null
  id: string
  pointId?: string
} {
  for (let index = snapshot.widgets.length - 1; index >= 0; index -= 1) {
    const widget = snapshot.widgets[index]
    if (
      point.x >= widget.bounds.x &&
      point.x <= widget.bounds.x + widget.bounds.width &&
      point.y >= widget.bounds.y &&
      point.y <= widget.bounds.y + widget.bounds.height
    ) {
      return { kind: 'time', id: widget.id }
    }
  }

  for (let index = snapshot.objects.length - 1; index >= 0; index -= 1) {
    const group = snapshot.objects[index]
    if (group.bounds) {
      if (
        point.x >= group.bounds.x &&
        point.x <= group.bounds.x + group.bounds.width &&
        point.y >= group.bounds.y &&
        point.y <= group.bounds.y + group.bounds.height
      ) {
        const hitPoint = group.motion.points.find((pathPoint) => distance(pathPoint, point) <= 2)
        if (hitPoint) {
          return { kind: 'object', id: group.id, pointId: hitPoint.id }
        }

        return { kind: 'object', id: group.id }
      }
    }
  }

  for (let index = snapshot.pixelLayers.length - 1; index >= 0; index -= 1) {
    const layer = snapshot.pixelLayers[index]
    if (pixelLayerContainsPoint(layer, point)) {
      return { kind: 'pixel', id: layer.id }
    }
  }

  for (let index = snapshot.backgrounds.length - 1; index >= 0; index -= 1) {
    const background = snapshot.backgrounds[index]
    if (
      point.x >= background.transform.x &&
      point.x <= background.transform.x + background.transform.width &&
      point.y >= background.transform.y &&
      point.y <= background.transform.y + background.transform.height
    ) {
      return { kind: 'background', id: background.id }
    }
  }

  return { kind: null, id: '' }
}

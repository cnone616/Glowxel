import { useMemo, useRef, useState } from 'react'
import type { PointerEvent as ReactPointerEvent } from 'react'
import { commandBus, buildFreehandStroke, buildToolStroke } from '../../editor/core/commandBus'
import { hitTestSnapshot } from '../../editor/core/renderEngine'
import { getActiveScene } from '../../editor/core/themeDocument'
import { useEditorStore } from '../../editor/state/editorStore'
import type { Point, Rect } from '../../editor/types'
import { StageCanvas } from './StageCanvas'

type StageDraft =
  | { kind: 'pixel-stroke'; layerId: string; tool: 'brush' | 'eraser'; anchor: Point; points: Point[] }
  | { kind: 'pixel-shape'; layerId: string; tool: 'line' | 'rect' | 'rect-select'; anchor: Point; current: Point }
  | { kind: 'pixel-move-selection'; layerId: string; selection: Rect; anchor: Point; delta: Point }
  | {
      kind: 'entity-move'
      entityKind: 'background' | 'object' | 'time'
      id: string
      origin: Point
      current: Point
      baseRect: Rect
    }
  | { kind: 'path-point'; objectId: string; pointId: string; origin: Point; current: Point }

function clamp(value: number, min: number, max: number): number {
  return Math.max(min, Math.min(max, value))
}

function normalizeRect(first: Point, second: Point): Rect {
  const minX = Math.min(first.x, second.x)
  const minY = Math.min(first.y, second.y)
  return {
    x: minX,
    y: minY,
    width: Math.abs(first.x - second.x) + 1,
    height: Math.abs(first.y - second.y) + 1,
  }
}

function isPointInRect(point: Point, rect: Rect): boolean {
  return (
    point.x >= rect.x &&
    point.x < rect.x + rect.width &&
    point.y >= rect.y &&
    point.y < rect.y + rect.height
  )
}

const pixelToolItems = [
  { id: 'brush', label: '画笔' },
  { id: 'eraser', label: '橡皮' },
  { id: 'eyedropper', label: '取色' },
  { id: 'fill', label: '填充' },
  { id: 'rect-select', label: '选区' },
  { id: 'move-selection', label: '移动选区' },
  { id: 'line', label: '直线' },
  { id: 'rect', label: '矩形' },
] as const

const objectToolItems = [
  { id: 'select', label: '选择' },
  { id: 'place', label: '摆放' },
  { id: 'path', label: '路径' },
] as const

function pointInRect(point: Point, rect: Rect): boolean {
  return (
    point.x >= rect.x &&
    point.x < rect.x + rect.width &&
    point.y >= rect.y &&
    point.y < rect.y + rect.height
  )
}

function toLayerPoint(point: Point, origin: Point): Point {
  return {
    x: point.x - origin.x,
    y: point.y - origin.y,
  }
}

function toStagePoint(point: Point, origin: Point): Point {
  return {
    x: point.x + origin.x,
    y: point.y + origin.y,
  }
}

function toStageRect(rect: Rect, origin: Point): Rect {
  return {
    ...rect,
    x: rect.x + origin.x,
    y: rect.y + origin.y,
  }
}

export function PreviewStage() {
  const document = useEditorStore((state) => state.document)
  const snapshot = useEditorStore((state) => state.snapshot)
  const session = useEditorStore((state) => state.session)
  const wrapperRef = useRef<HTMLDivElement | null>(null)
  const [draft, setDraft] = useState<StageDraft | null>(null)

  const activeScene = useMemo(
    () => getActiveScene(document, session.activeSceneId),
    [document, session.activeSceneId],
  )
  const selectedPixelLayer =
    session.selection.kind === 'pixel'
      ? activeScene.pixelLayers.find((layer) => layer.id === session.selection.ids[0]) ?? null
      : null
  const selectedObject =
    session.selection.kind === 'object'
      ? snapshot.objects.find((item) => item.id === session.selection.ids[0]) ?? null
      : null
  const selectedBackground =
    session.selection.kind === 'background'
      ? snapshot.backgrounds.find((item) => item.id === session.selection.ids[0]) ?? null
      : null
  const selectedWidget =
    session.selection.kind === 'time'
      ? snapshot.widgets.find((item) => item.id === session.selection.ids[0]) ?? null
      : null

  const emitStageNotice = (text: string, tone: 'info' | 'success' | 'warning' | 'error' = 'warning') => {
    commandBus.execute({
      type: 'session.setNotice',
      notice: {
        text,
        tone,
        source: 'viewport',
      },
    })
  }

  const toWorldPoint = (event: ReactPointerEvent<HTMLDivElement>): Point => {
    const rect = wrapperRef.current?.getBoundingClientRect()
    if (!rect) {
      return { x: 0, y: 0 }
    }

    return {
      x: clamp(
        Math.floor(((event.clientX - rect.left) / rect.width) * snapshot.matrix.width),
        0,
        snapshot.matrix.width - 1,
      ),
      y: clamp(
        Math.floor(((event.clientY - rect.top) / rect.height) * snapshot.matrix.height),
        0,
        snapshot.matrix.height - 1,
      ),
    }
  }

  const getPreviewPixels = (): Point[] => {
    if (!draft) {
      return []
    }

    if (draft.kind === 'pixel-stroke') {
      const stroke = buildFreehandStroke(draft.anchor, draft.points)
      if (!selectedPixelLayer) {
        return stroke
      }
      return stroke.map((point) => toStagePoint(point, selectedPixelLayer))
    }
    if (draft.kind === 'pixel-shape') {
      if (draft.tool === 'rect-select') {
        return []
      }
      const stroke = buildToolStroke(draft.anchor, [draft.current], draft.tool === 'line' ? 'line' : 'rect')
      if (!selectedPixelLayer) {
        return stroke
      }
      return stroke.map((point) => toStagePoint(point, selectedPixelLayer))
    }
    return []
  }

  const getSelectionOverlayRect = (): Rect | null => {
    if (draft?.kind === 'pixel-shape' && draft.tool === 'rect-select') {
      const selection = normalizeRect(draft.anchor, draft.current)
      return selectedPixelLayer ? toStageRect(selection, selectedPixelLayer) : selection
    }
    if (draft?.kind === 'pixel-move-selection') {
      const selection = {
        ...draft.selection,
        x: draft.selection.x + draft.delta.x,
        y: draft.selection.y + draft.delta.y,
      }
      return selectedPixelLayer ? toStageRect(selection, selectedPixelLayer) : selection
    }
    return session.pixelSelection && selectedPixelLayer
      ? toStageRect(session.pixelSelection, selectedPixelLayer)
      : session.pixelSelection
  }

  const getEntityOverlayRect = (): Rect | null => {
    if (draft?.kind === 'entity-move') {
      return {
        ...draft.baseRect,
        x: draft.baseRect.x + (draft.current.x - draft.origin.x),
        y: draft.baseRect.y + (draft.current.y - draft.origin.y),
      }
    }

    if (selectedBackground) {
      return {
        x: selectedBackground.transform.x,
        y: selectedBackground.transform.y,
        width: selectedBackground.transform.width,
        height: selectedBackground.transform.height,
      }
    }
    if (selectedObject?.bounds) {
      return selectedObject.bounds
    }
    if (selectedPixelLayer) {
      return {
        x: selectedPixelLayer.x,
        y: selectedPixelLayer.y,
        width: 64,
        height: 64,
      }
    }
    if (selectedWidget) {
      return selectedWidget.bounds
    }
    return null
  }

  const commitEntityMove = (entityKind: 'background' | 'object' | 'time', id: string, delta: Point) => {
    if (entityKind === 'background' && selectedBackground) {
      commandBus.execute({
        type: 'background.updateTransform',
        id,
        patch: {
          x: Math.round(selectedBackground.transform.x + delta.x),
          y: Math.round(selectedBackground.transform.y + delta.y),
        },
      })
    }

    if (entityKind === 'object' && selectedObject) {
      commandBus.execute({
        type: 'object.updateTransform',
        id,
        patch: {
          x: Math.round(selectedObject.transform.x + delta.x),
          y: Math.round(selectedObject.transform.y + delta.y),
        },
      })
    }

    if (entityKind === 'time' && selectedWidget) {
      commandBus.execute({
        type: 'time.update',
        id,
        patch: {
          x: selectedWidget.x + delta.x,
          y: selectedWidget.y + delta.y,
        },
      })
    }
  }

  const handlePointerDown = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = toWorldPoint(event)
    const hit = hitTestSnapshot(snapshot, point)

    if (selectedPixelLayer) {
      const pixelPoint = toLayerPoint(point, selectedPixelLayer)

      if (session.currentPixelTool === 'eyedropper') {
        commandBus.execute({ type: 'pixelLayer.eyedropper', id: selectedPixelLayer.id, point: pixelPoint })
        return
      }

      if (session.currentPixelTool === 'fill') {
        commandBus.execute({
          type: 'pixelLayer.fill',
          id: selectedPixelLayer.id,
          point: pixelPoint,
          color: session.drawColor,
        })
        return
      }

      if (session.currentPixelTool === 'move-selection' && session.pixelSelection) {
        if (isPointInRect(pixelPoint, session.pixelSelection)) {
          setDraft({
            kind: 'pixel-move-selection',
            layerId: selectedPixelLayer.id,
            selection: session.pixelSelection,
            anchor: pixelPoint,
            delta: { x: 0, y: 0 },
          })
        }
        return
      }

      if (session.currentPixelTool === 'brush' || session.currentPixelTool === 'eraser') {
        setDraft({
          kind: 'pixel-stroke',
          layerId: selectedPixelLayer.id,
          tool: session.currentPixelTool,
          anchor: pixelPoint,
          points: [pixelPoint],
        })
        return
      }

      if (
        session.currentPixelTool === 'line' ||
        session.currentPixelTool === 'rect' ||
        session.currentPixelTool === 'rect-select'
      ) {
        setDraft({
          kind: 'pixel-shape',
          layerId: selectedPixelLayer.id,
          tool: session.currentPixelTool,
          anchor: pixelPoint,
          current: pixelPoint,
        })
        return
      }
    }

    if (session.currentTool === 'path') {
      if (session.selection.kind === 'object' && session.selection.ids[0]) {
        if (hit.kind === 'object' && hit.pointId) {
          setDraft({
            kind: 'path-point',
            objectId: hit.id,
            pointId: hit.pointId,
            origin: point,
            current: point,
          })
        } else {
          commandBus.execute({
            type: 'object.addPathPoint',
            id: session.selection.ids[0],
            point,
          })
        }
        return
      }

      if (hit.kind === 'object') {
        commandBus.execute({ type: 'selection.set', kind: 'object', ids: [hit.id], additive: false })
        return
      }

      emitStageNotice('先选中对象组，再进入路径编辑。')
      return
    }

    if (session.currentTool === 'place') {
      if (
        session.selection.kind !== 'background' &&
        session.selection.kind !== 'object' &&
        session.selection.kind !== 'time'
      ) {
        emitStageNotice('摆放模式只对背景层、对象组和时间组件生效。')
        return
      }

      if (session.selection.kind === 'background' && session.selection.ids[0]) {
        commandBus.execute({
          type: 'background.updateTransform',
          id: session.selection.ids[0],
          patch: { x: point.x, y: point.y },
        })
      }
      if (session.selection.kind === 'object' && session.selection.ids[0]) {
        commandBus.execute({
          type: 'object.updateTransform',
          id: session.selection.ids[0],
          patch: { x: point.x, y: point.y },
        })
      }
      if (session.selection.kind === 'time' && session.selection.ids[0]) {
        commandBus.execute({
          type: 'time.update',
          id: session.selection.ids[0],
          patch: { x: point.x, y: point.y },
        })
      }
      return
    }

    if (hit.kind === 'background') {
      commandBus.execute({ type: 'selection.set', kind: 'background', ids: [hit.id], additive: false })
      const target = snapshot.backgrounds.find((item) => item.id === hit.id)
      if (target) {
        setDraft({
          kind: 'entity-move',
          entityKind: 'background',
          id: hit.id,
          origin: point,
          current: point,
          baseRect: {
            x: target.transform.x,
            y: target.transform.y,
            width: target.transform.width,
            height: target.transform.height,
          },
        })
      }
      return
    }

    if (hit.kind === 'object') {
      commandBus.execute({ type: 'selection.set', kind: 'object', ids: [hit.id], additive: false })
      const target = snapshot.objects.find((item) => item.id === hit.id)
      if (target?.bounds) {
        setDraft({
          kind: 'entity-move',
          entityKind: 'object',
          id: hit.id,
          origin: point,
          current: point,
          baseRect: target.bounds,
        })
      }
      return
    }

    if (hit.kind === 'pixel') {
      commandBus.execute({ type: 'pixelLayer.select', id: hit.id })
      return
    }

    if (hit.kind === 'time') {
      commandBus.execute({ type: 'selection.set', kind: 'time', ids: [hit.id], additive: false })
      const target = snapshot.widgets.find((item) => item.id === hit.id)
      if (target) {
        setDraft({
          kind: 'entity-move',
          entityKind: 'time',
          id: hit.id,
          origin: point,
          current: point,
          baseRect: target.bounds,
        })
      }
      return
    }

    const pixelLayerByBounds = snapshot.pixelLayers
      .slice()
      .reverse()
      .find((layer) => pointInRect(point, layer.bounds))

    if (pixelLayerByBounds) {
      commandBus.execute({ type: 'pixelLayer.select', id: pixelLayerByBounds.id })
      return
    }

    commandBus.execute({ type: 'scene.select', id: activeScene.id })
  }

  const handlePointerMove = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = toWorldPoint(event)
    const pixelPoint = selectedPixelLayer ? toLayerPoint(point, selectedPixelLayer) : point

    if (!draft) {
      const hit = hitTestSnapshot(snapshot, point)
      commandBus.execute({ type: 'session.setHoverEntity', entityId: hit.id })
      return
    }

    if (draft.kind === 'pixel-stroke') {
      setDraft({
        ...draft,
        points:
          draft.points[draft.points.length - 1]?.x === pixelPoint.x &&
          draft.points[draft.points.length - 1]?.y === pixelPoint.y
            ? draft.points
            : draft.points.concat(pixelPoint),
      })
      return
    }

    if (draft.kind === 'pixel-shape') {
      setDraft({
        ...draft,
        current: pixelPoint,
      })
      return
    }

    if (draft.kind === 'pixel-move-selection') {
      setDraft({
        ...draft,
        delta: {
          x: pixelPoint.x - draft.anchor.x,
          y: pixelPoint.y - draft.anchor.y,
        },
      })
      return
    }

    if (draft.kind === 'entity-move' || draft.kind === 'path-point') {
      setDraft({
        ...draft,
        current: point,
      })
    }
  }

  const handlePointerUp = () => {
    if (!draft) {
      return
    }

    if (draft.kind === 'pixel-stroke') {
      const stroke = buildFreehandStroke(draft.anchor, draft.points)
      if (draft.tool === 'brush') {
        commandBus.execute({
          type: 'pixelLayer.paint',
          id: draft.layerId,
          points: stroke,
          color: session.drawColor,
        })
      } else {
        commandBus.execute({
          type: 'pixelLayer.erase',
          id: draft.layerId,
          points: stroke,
        })
      }
      setDraft(null)
      return
    }

    if (draft.kind === 'pixel-shape') {
      if (draft.tool === 'rect-select') {
        commandBus.execute({
          type: 'session.setPixelSelection',
          selection: normalizeRect(draft.anchor, draft.current),
        })
      } else {
        commandBus.execute({
          type: 'pixelLayer.paint',
          id: draft.layerId,
          points: buildToolStroke(
            draft.anchor,
            [draft.current],
            draft.tool === 'line' ? 'line' : 'rect',
          ),
          color: session.drawColor,
        })
      }
      setDraft(null)
      return
    }

    if (draft.kind === 'pixel-move-selection') {
      commandBus.execute({
        type: 'pixelLayer.transformSelection',
        id: draft.layerId,
        selection: draft.selection,
        delta: draft.delta,
      })
      commandBus.execute({
        type: 'session.setPixelSelection',
        selection: {
          ...draft.selection,
          x: draft.selection.x + draft.delta.x,
          y: draft.selection.y + draft.delta.y,
        },
      })
      setDraft(null)
      return
    }

    if (draft.kind === 'entity-move') {
      commitEntityMove(draft.entityKind, draft.id, {
        x: draft.current.x - draft.origin.x,
        y: draft.current.y - draft.origin.y,
      })
      setDraft(null)
      return
    }

    if (draft.kind === 'path-point') {
      commandBus.execute({
        type: 'object.updatePathPoint',
        id: draft.objectId,
        pointId: draft.pointId,
        patch: draft.current,
      })
      setDraft(null)
    }
  }

  const previewPixels = getPreviewPixels()
  const pixelSelectionRect = getSelectionOverlayRect()
  const entityRect = getEntityOverlayRect()

  return (
    <section className="mx-pane mx-stage-pane">
      <div className="mx-stage-toolbar">
        <div className="mx-stage-toolgroup">
          {selectedPixelLayer ? (
            pixelToolItems.map((item) => (
              <button
                key={item.id}
                className={session.currentPixelTool === item.id ? 'mx-toolchip is-active' : 'mx-toolchip'}
                onClick={() => commandBus.execute({ type: 'session.setPixelTool', tool: item.id })}
                type="button"
              >
                {item.label}
              </button>
            ))
          ) : (
            objectToolItems.map((item) => (
              <button
                key={item.id}
                className={session.currentTool === item.id ? 'mx-toolchip is-active' : 'mx-toolchip'}
                onClick={() => commandBus.execute({ type: 'session.setTool', tool: item.id })}
                type="button"
              >
                {item.label}
              </button>
            ))
          )}
        </div>

        <div className="mx-stage-toolgroup">
          <button
            className="mx-toolchip"
            onClick={() => commandBus.execute({ type: 'pixelLayer.create', sceneId: activeScene.id })}
            type="button"
          >
            新建像素层
          </button>
          {selectedPixelLayer ? (
            <>
              <button
                className="mx-toolchip"
                onClick={() => commandBus.execute({ type: 'pixelLayer.duplicate', id: selectedPixelLayer.id })}
                type="button"
              >
                复制像素层
              </button>
              <button
                className="mx-toolchip"
                onClick={() => commandBus.execute({ type: 'pixelLayer.delete', id: selectedPixelLayer.id })}
                type="button"
              >
                删除像素层
              </button>
              <button
                className="mx-toolchip"
                onClick={() => commandBus.execute({ type: 'pixelLayer.flip', id: selectedPixelLayer.id, axis: 'horizontal' })}
                type="button"
              >
                水平镜像
              </button>
              <button
                className="mx-toolchip"
                onClick={() => commandBus.execute({ type: 'pixelLayer.flip', id: selectedPixelLayer.id, axis: 'vertical' })}
                type="button"
              >
                垂直镜像
              </button>
              <input
                className="mx-color-input"
                onChange={(event) => commandBus.execute({ type: 'session.setDrawColor', color: event.target.value })}
                type="color"
                value={session.drawColor}
              />
            </>
          ) : null}
        </div>
      </div>

      <div className="mx-stage-shell">
        <div
          ref={wrapperRef}
          className="mx-stage-square"
          onPointerDown={handlePointerDown}
          onPointerMove={handlePointerMove}
          onPointerUp={handlePointerUp}
        >
          <StageCanvas className="mx-stage-canvas" snapshot={snapshot} />
          <div className="mx-stage-grid" />

          <svg className="mx-stage-overlay" viewBox={`0 0 ${snapshot.matrix.width} ${snapshot.matrix.height}`}>
            {entityRect ? (
              <rect
                className={`mx-selection-rect ${selectedPixelLayer ? 'is-pixel' : ''}`}
                x={entityRect.x}
                y={entityRect.y}
                width={entityRect.width}
                height={entityRect.height}
              />
            ) : null}

            {pixelSelectionRect ? (
              <rect
                className="mx-selection-rect is-selection"
                x={pixelSelectionRect.x}
                y={pixelSelectionRect.y}
                width={pixelSelectionRect.width}
                height={pixelSelectionRect.height}
              />
            ) : null}

            {previewPixels.map((point, index) => (
              <rect
                key={`${point.x}-${point.y}-${index}`}
                className="mx-preview-pixel"
                x={point.x}
                y={point.y}
                width={1}
                height={1}
              />
            ))}

            {selectedObject ? (
              <>
                <polyline
                  className="mx-path-line"
                  points={selectedObject.motion.points.map((point) => `${point.x},${point.y}`).join(' ')}
                />
                {selectedObject.motion.points.map((point) => (
                  <circle className="mx-path-point" key={point.id} cx={point.x} cy={point.y} r={1.1} />
                ))}
              </>
            ) : null}
          </svg>
        </div>
      </div>
    </section>
  )
}

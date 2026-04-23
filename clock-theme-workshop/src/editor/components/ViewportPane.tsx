import { useEffect, useMemo, useRef, useState } from 'react'
import type { PointerEvent as ReactPointerEvent } from 'react'
import { Application, extend } from '@pixi/react'
import { Container, Graphics, Sprite, Texture } from 'pixi.js'
import { buildPixelTextCells } from '../core/pixelFont'
import { cloneDocument } from '../core/clone'
import { commandBus } from '../core/commandBus'
import { clamp } from '../core/math'
import { hitTestSnapshot, selectAssetFrame } from '../core/renderEngine'
import { useEditorStore } from '../state/editorStore'
import type { CatalogFrame, DrawableBackground, DrawableObjectGroup, DrawableTimeWidget, Point, ThemeDocument } from '../types'
import { upsertKeyframe } from '../core/themeDocument'

extend({ Container, Graphics, Sprite })

const pixelTextureCache = new Map<string, Texture>()
const uploadedTextureCache = new Map<string, Texture>()

function toColor(hex: string): number {
  return Number.parseInt(hex.slice(1), 16)
}

function getPixelFrameTexture(frame: CatalogFrame): Texture {
  const cacheKey = `${frame.id}-${frame.width}-${frame.height}`
  const cached = pixelTextureCache.get(cacheKey)
  if (cached) {
    return cached
  }

  const canvas = document.createElement('canvas')
  canvas.width = frame.width
  canvas.height = frame.height
  const context = canvas.getContext('2d')
  if (!context) {
    throw new Error('像素纹理上下文创建失败')
  }

  const imageData = context.createImageData(frame.width, frame.height)
  for (let index = 0; index < frame.pixels.length; index += 1) {
    const pixel = frame.pixels[index]
    const pixelIndex = (pixel[1] * frame.width + pixel[0]) * 4
    imageData.data[pixelIndex] = pixel[2]
    imageData.data[pixelIndex + 1] = pixel[3]
    imageData.data[pixelIndex + 2] = pixel[4]
    imageData.data[pixelIndex + 3] = 255
  }
  context.putImageData(imageData, 0, 0)

  const texture = Texture.from(canvas)
  pixelTextureCache.set(cacheKey, texture)
  return texture
}

function getUploadedTexture(dataUrl: string): Texture {
  const cached = uploadedTextureCache.get(dataUrl)
  if (cached) {
    return cached
  }

  const texture = Texture.from(dataUrl)
  uploadedTextureCache.set(dataUrl, texture)
  return texture
}

function RasterNode({
  asset,
  x,
  y,
  width,
  height,
  alpha,
}: {
  asset: CatalogFrame | { dataUrl: string } | null
  x: number
  y: number
  width: number
  height: number
  alpha: number
}) {
  const texture = useMemo(() => {
    if (!asset) {
      return null
    }
    if ('dataUrl' in asset) {
      return getUploadedTexture(asset.dataUrl)
    }
    return getPixelFrameTexture(asset)
  }, [asset])

  if (!texture) {
    return null
  }

  return <pixiSprite alpha={alpha} height={height} texture={texture} width={width} x={x} y={y} />
}

function toRasterAsset(asset: CatalogFrame | { dataUrl?: string } | null): CatalogFrame | { dataUrl: string } | null {
  if (!asset) {
    return null
  }

  if ('pixels' in asset) {
    return asset
  }

  if (typeof asset.dataUrl === 'string') {
    return { dataUrl: asset.dataUrl }
  }

  return null
}

function BackgroundLayerNode({
  background,
  currentFrame,
}: {
  background: DrawableBackground
  currentFrame: number
}) {
  const assetFrame = selectAssetFrame(background.asset, currentFrame, background.state.sequenceStepFrames)
  let opacity = background.transform.opacity
  if (background.state.behavior === 'blink') {
    const phase = Math.floor(currentFrame / Math.max(1, background.state.rhythmFrames)) % 2
    opacity = phase === 0 ? opacity : opacity * 0.18
  }

  const tiles =
    background.state.behavior === 'loop'
      ? Array.from({ length: 16 }, (_, index) => ({
          tileX: (index % 4) - 1,
          tileY: Math.floor(index / 4) - 1,
        }))
      : [{ tileX: 0, tileY: 0 }]

  const offsetX =
    background.state.behavior === 'loop'
      ? ((currentFrame * background.state.speedX) % background.transform.width + background.transform.width) %
        background.transform.width
      : 0
  const offsetY =
    background.state.behavior === 'loop'
      ? ((currentFrame * background.state.speedY) % background.transform.height + background.transform.height) %
        background.transform.height
      : 0

  return (
    <pixiContainer>
      <pixiGraphics
        draw={(graphics) => {
          graphics.clear()
          graphics.rect(
            background.transform.x,
            background.transform.y,
            background.transform.width,
            background.transform.height,
          )
          graphics.fill({ color: toColor(background.baseColor), alpha: background.transform.opacity })
        }}
      />
      {assetFrame
        ? tiles.map((tile) => (
            <RasterNode
              key={`${background.id}-${tile.tileX}-${tile.tileY}`}
              alpha={opacity}
              asset={toRasterAsset(assetFrame)}
              height={background.transform.height}
              width={background.transform.width}
              x={Math.round(background.transform.x - offsetX + tile.tileX * background.transform.width)}
              y={Math.round(background.transform.y - offsetY + tile.tileY * background.transform.height)}
            />
          ))
        : null}
    </pixiContainer>
  )
}

function ObjectLayerNode({
  group,
  currentFrame,
}: {
  group: DrawableObjectGroup
  currentFrame: number
}) {
  if (!group.state.visible) {
    return null
  }

  let opacity = group.transform.opacity
  if (group.state.behavior === 'blink') {
    const phase = Math.floor(currentFrame / Math.max(1, group.state.rhythmFrames)) % 2
    opacity = phase === 0 ? opacity : opacity * 0.18
  }

  return (
    <pixiContainer>
      {group.members.map((member) => {
        if (!member.visible || !member.asset) {
          return null
        }

        const assetFrame = selectAssetFrame(member.asset, currentFrame, group.state.sequenceStepFrames)
        if (!assetFrame) {
          return null
        }

        return (
          <RasterNode
            key={member.id}
            alpha={opacity * member.opacity}
            asset={toRasterAsset(assetFrame)}
            height={Math.max(1, Math.round(member.height * group.transform.scale))}
            width={Math.max(1, Math.round(member.width * group.transform.scale))}
            x={member.x}
            y={member.y}
          />
        )
      })}
    </pixiContainer>
  )
}

function TimeLayerNode({ widgets }: { widgets: DrawableTimeWidget[] }) {
  return (
    <pixiContainer>
      {widgets.map((widget) => (
        <pixiGraphics
          key={widget.id}
          draw={(graphics) => {
            graphics.clear()
            if (!widget.visible) {
              return
            }
            for (const cell of buildPixelTextCells(widget)) {
              graphics.rect(cell.x, cell.y, cell.size, cell.size)
              graphics.fill({ color: toColor(widget.color) })
            }
          }}
        />
      ))}
    </pixiContainer>
  )
}

function applyPreviewMove(document: ThemeDocument, selectionKind: 'background' | 'object' | 'time', selectionIds: string[], currentFrame: number, delta: Point) {
  if (selectionKind === 'background') {
    for (const id of selectionIds) {
      const layer = document.backgroundLayers.find((item) => item.id === id)
      if (!layer || layer.locked) {
        continue
      }
      const current = layer.transformKeys.slice().reverse().find((key) => key.frame <= currentFrame) ?? layer.transformKeys[0]
      layer.transformKeys = upsertKeyframe(layer.transformKeys, currentFrame, {
        x: Math.round(current.x + delta.x),
        y: Math.round(current.y + delta.y),
      })
    }
  }

  if (selectionKind === 'object') {
    for (const id of selectionIds) {
      const objectGroup = document.objectGroups.find((item) => item.id === id)
      if (!objectGroup || objectGroup.locked) {
        continue
      }
      const current = objectGroup.transformKeys.slice().reverse().find((key) => key.frame <= currentFrame) ?? objectGroup.transformKeys[0]
      objectGroup.transformKeys = upsertKeyframe(objectGroup.transformKeys, currentFrame, {
        x: Math.round(current.x + delta.x),
        y: Math.round(current.y + delta.y),
      })
    }
  }

  if (selectionKind === 'time') {
    for (const id of selectionIds) {
      const widget = document.timeWidgets.find((item) => item.id === id)
      if (!widget || widget.locked) {
        continue
      }
      widget.x = Math.round(widget.x + delta.x)
      widget.y = Math.round(widget.y + delta.y)
    }
  }
}

export function ViewportPane() {
  const document = useEditorStore((state) => state.document)
  const snapshot = useEditorStore((state) => state.snapshot)
  const session = useEditorStore((state) => state.session)
  const wrapperRef = useRef<HTMLDivElement | null>(null)
  const [viewportSize, setViewportSize] = useState({ width: 640, height: 640 })
  const [hoverPoint, setHoverPoint] = useState<Point | null>(null)

  useEffect(() => {
    const element = wrapperRef.current
    if (!element) {
      return undefined
    }

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0]
      if (!entry) {
        return
      }
      setViewportSize({
        width: entry.contentRect.width,
        height: entry.contentRect.height,
      })
    })

    observer.observe(element)
    return () => observer.disconnect()
  }, [])

  const padding = 26
  const fitScaleBase = Math.min(
    (viewportSize.width - padding * 2) / snapshot.matrix.width,
    (viewportSize.height - padding * 2) / snapshot.matrix.height,
  )
  const fitScale = fitScaleBase >= 1 ? Math.max(1, Math.floor(fitScaleBase)) : Math.max(0.35, fitScaleBase)
  const effectiveZoom = session.viewportMode === 'fit' ? 1 : session.viewportZoom
  const displayScale = fitScale * effectiveZoom
  const stageWidth = snapshot.matrix.width * displayScale
  const stageHeight = snapshot.matrix.height * displayScale
  const offsetX = (viewportSize.width - stageWidth) / 2 + session.viewportOffset.x
  const offsetY = (viewportSize.height - stageHeight) / 2 + session.viewportOffset.y
  const activeTool = session.temporaryPan ? 'pan' : session.currentTool

  const hoveredBackground = snapshot.backgrounds.find((item) => item.id === session.hoverEntityId) ?? null
  const hoveredObject = snapshot.objects.find((item) => item.id === session.hoverEntityId) ?? null
  const hoveredWidget = snapshot.widgets.find((item) => item.id === session.hoverEntityId) ?? null
  const hoverOverlay =
    hoveredBackground
      ? {
          kind: 'background',
          label: hoveredBackground.name,
          description: `${hoveredBackground.state.behavior} · ${Math.round(hoveredBackground.transform.x)}, ${Math.round(hoveredBackground.transform.y)}`,
          rect: {
            x: hoveredBackground.transform.x,
            y: hoveredBackground.transform.y,
            width: hoveredBackground.transform.width,
            height: hoveredBackground.transform.height,
          },
        }
      : hoveredObject && hoveredObject.bounds
        ? {
            kind: 'object',
            label: hoveredObject.name,
            description: `${hoveredObject.state.behavior} · ${Math.round(hoveredObject.transform.x)}, ${Math.round(hoveredObject.transform.y)}`,
            rect: hoveredObject.bounds,
          }
        : hoveredWidget
          ? {
              kind: 'time',
              label: hoveredWidget.label,
              description: `${hoveredWidget.type} · ${Math.round(hoveredWidget.x)}, ${Math.round(hoveredWidget.y)}`,
              rect: hoveredWidget.bounds,
            }
          : null

  const selectionSummary =
    session.selection.kind === 'scene'
      ? '场景'
      : `${session.selection.kind === 'background' ? '背景层' : session.selection.kind === 'object' ? '对象组' : '时间组件'} ${session.selection.ids.length}`

  const toWorldPoint = (event: ReactPointerEvent<HTMLDivElement>): Point => {
    const rect = wrapperRef.current?.getBoundingClientRect()
    if (!rect) {
      return { x: 0, y: 0 }
    }
    return {
      x: clamp(Math.floor((event.clientX - rect.left - offsetX) / displayScale), 0, snapshot.matrix.width - 1),
      y: clamp(Math.floor((event.clientY - rect.top - offsetY) / displayScale), 0, snapshot.matrix.height - 1),
    }
  }

  const applyPlacement = (point: Point) => {
    const selection = useEditorStore.getState().session.selection
    if (selection.kind === 'background') {
      commandBus.execute({
        type: 'background.updateTransform',
        id: selection.ids[0],
        patch: { x: point.x, y: point.y },
      })
    }
    if (selection.kind === 'object') {
      commandBus.execute({
        type: 'object.updateTransform',
        id: selection.ids[0],
        patch: { x: point.x, y: point.y },
      })
    }
    if (selection.kind === 'time') {
      commandBus.execute({
        type: 'time.update',
        id: selection.ids[0],
        patch: { x: point.x, y: point.y },
      })
    }
  }

  const handlePointerDown = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = toWorldPoint(event)
    const hit = hitTestSnapshot(snapshot, point)
    const tool = activeTool

    event.currentTarget.setPointerCapture(event.pointerId)

    if (tool === 'pan') {
      useEditorStore.getState().setDragState({
        kind: 'pan',
        originPoint: { x: event.clientX, y: event.clientY },
        originOffset: session.viewportOffset,
      })
      return
    }

    if (tool === 'path' && session.selection.kind === 'object' && session.selection.ids[0]) {
      if (hit.kind === 'object' && hit.pointId) {
        useEditorStore.getState().setDragState({
          kind: 'path-point',
          objectId: hit.id,
          pointId: hit.pointId,
          originPoint: point,
          originDocument: cloneDocument(document),
        })
        return
      }

      commandBus.execute({
        type: 'object.addPathPoint',
        id: session.selection.ids[0],
        point,
      })
      return
    }

    if (tool === 'path') {
      commandBus.execute({
        type: 'session.setNotice',
        notice: {
          text: '路径工具需要先选中一个对象组。',
          tone: 'warning',
          source: 'viewport',
        },
      })
      return
    }

    if (tool === 'place') {
      if (session.selection.kind === 'scene' || session.selection.ids.length === 0) {
        commandBus.execute({
          type: 'session.setNotice',
          notice: {
            text: '摆放工具需要先选中一个背景、对象或时间组件。',
            tone: 'warning',
            source: 'viewport',
          },
        })
        return
      }
      applyPlacement(point)
      return
    }

    if (hit.kind) {
      commandBus.execute({
        type: 'selection.set',
        kind: hit.kind,
        ids: [hit.id],
        additive: event.shiftKey,
      })

      const selectionIds =
        session.selection.kind === hit.kind && session.selection.ids.includes(hit.id)
          ? session.selection.ids
          : [hit.id]

      useEditorStore.getState().setDragState({
        kind: 'move',
        entityKind: hit.kind,
        ids: selectionIds,
        originPoint: point,
        originDocument: cloneDocument(document),
      })
      return
    }

    commandBus.execute({ type: 'selection.clear' })
  }

  const handlePointerMove = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = toWorldPoint(event)
    setHoverPoint(point)
    const dragState = useEditorStore.getState().session.dragState

    if (!dragState) {
      const hit = hitTestSnapshot(snapshot, point)
      commandBus.execute({ type: 'session.setHoverEntity', entityId: hit.id })
      return
    }

    if (dragState.kind === 'pan') {
      commandBus.execute({
        type: 'session.panViewport',
        offset: {
          x: dragState.originOffset.x + (event.clientX - dragState.originPoint.x),
          y: dragState.originOffset.y + (event.clientY - dragState.originPoint.y),
        },
      })
      return
    }

    if (dragState.kind === 'path-point') {
      const previewDocument = cloneDocument(dragState.originDocument)
      const objectGroup = previewDocument.objectGroups.find((item) => item.id === dragState.objectId)
      const pathPoint = objectGroup?.motion.points.find((item) => item.id === dragState.pointId)
      if (pathPoint) {
        pathPoint.x = point.x
        pathPoint.y = point.y
      }
      useEditorStore.getState().replaceDocument(previewDocument)
      useEditorStore.getState().markDirty({ renderDirty: true })
      return
    }

    const delta = {
      x: point.x - dragState.originPoint.x,
      y: point.y - dragState.originPoint.y,
    }
    const previewDocument = cloneDocument(dragState.originDocument)
    applyPreviewMove(previewDocument, dragState.entityKind, dragState.ids, session.currentFrame, delta)
    useEditorStore.getState().replaceDocument(previewDocument)
    useEditorStore.getState().markDirty({ renderDirty: true })
  }

  const handlePointerUp = (event: ReactPointerEvent<HTMLDivElement>) => {
    const point = toWorldPoint(event)
    setHoverPoint(point)
    const dragState = useEditorStore.getState().session.dragState
    if (!dragState) {
      return
    }

    if (dragState.kind === 'pan') {
      useEditorStore.getState().setDragState(null)
      return
    }

    if (dragState.kind === 'path-point') {
      useEditorStore.getState().replaceDocument(dragState.originDocument)
      commandBus.execute({
        type: 'object.updatePathPoint',
        id: dragState.objectId,
        pointId: dragState.pointId,
        patch: point,
      })
      useEditorStore.getState().setDragState(null)
      return
    }

    const delta = {
      x: point.x - dragState.originPoint.x,
      y: point.y - dragState.originPoint.y,
    }

    useEditorStore.getState().replaceDocument(dragState.originDocument)

    for (const id of dragState.ids) {
      if (dragState.entityKind === 'background') {
        const original = dragState.originDocument.backgroundLayers.find((item) => item.id === id)
        const current = original?.transformKeys.slice().reverse().find((key) => key.frame <= session.currentFrame)
        if (current) {
          commandBus.execute({
            type: 'background.updateTransform',
            id,
            patch: {
              x: Math.round(current.x + delta.x),
              y: Math.round(current.y + delta.y),
            },
          })
        }
      }

      if (dragState.entityKind === 'object') {
        const original = dragState.originDocument.objectGroups.find((item) => item.id === id)
        const current = original?.transformKeys.slice().reverse().find((key) => key.frame <= session.currentFrame)
        if (current) {
          commandBus.execute({
            type: 'object.updateTransform',
            id,
            patch: {
              x: Math.round(current.x + delta.x),
              y: Math.round(current.y + delta.y),
            },
          })
        }
      }

      if (dragState.entityKind === 'time') {
        const widget = dragState.originDocument.timeWidgets.find((item) => item.id === id)
        if (widget) {
          commandBus.execute({
            type: 'time.update',
            id,
            patch: {
              x: Math.round(widget.x + delta.x),
              y: Math.round(widget.y + delta.y),
            },
          })
        }
      }
    }

    useEditorStore.getState().setDragState(null)
  }

  const handlePointerLeave = () => {
    if (!useEditorStore.getState().session.dragState) {
      commandBus.execute({ type: 'session.setHoverEntity', entityId: '' })
    }
    setHoverPoint(null)
  }

  return (
    <section className="viewport-pane">
      <div className="viewport-meta">
        <div className="viewport-meta-group">
          <span>矩阵</span>
          <strong>{snapshot.matrix.width}×{snapshot.matrix.height}</strong>
        </div>
        <div className="viewport-meta-group">
          <span>工具</span>
          <strong>{activeTool === 'select' ? '选择' : activeTool === 'pan' ? '平移' : activeTool === 'place' ? '摆放' : '路径'}</strong>
        </div>
        <div className="viewport-meta-group">
          <span>缩放</span>
          <strong>{session.viewportMode === 'fit' ? '适配' : `${session.viewportZoom}x`}</strong>
        </div>
        <div className="viewport-meta-group">
          <span>预演</span>
          <strong>{document.meta.previewMode === 'loop' ? '常规循环' : document.meta.previewMode === 'minute' ? '分钟触发' : '整点触发'}</strong>
        </div>
      </div>

      <div
        className={session.dragState ? `viewport-surface tool-${activeTool} dragging` : `viewport-surface tool-${activeTool}`}
        onPointerDown={handlePointerDown}
        onPointerLeave={handlePointerLeave}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        ref={wrapperRef}
      >
        <div className="viewport-hud viewport-hud-top-left">
          <span className="viewport-hud-pill emphasized">选择 {selectionSummary}</span>
          <span className="viewport-hud-pill">帧 {session.currentFrame + 1}</span>
          <span className="viewport-hud-pill">光标 {hoverPoint ? `${hoverPoint.x}, ${hoverPoint.y}` : '--'}</span>
        </div>

        <div className="viewport-hud viewport-hud-top-right">
          {hoverOverlay ? (
            <div className="viewport-inspect-card">
              <span>{hoverOverlay.kind === 'background' ? '背景层' : hoverOverlay.kind === 'object' ? '对象组' : '时间组件'}</span>
              <strong>{hoverOverlay.label}</strong>
              <small>{hoverOverlay.description}</small>
            </div>
          ) : (
            <div className="viewport-inspect-card muted">
              <span>工作台提示</span>
              <strong>{activeTool === 'path' ? '路径模式已就绪' : activeTool === 'place' ? '点击舞台写入位置' : '拖拽或点选舞台对象'}</strong>
              <small>Shift 多选 · Space 临时平移</small>
            </div>
          )}
        </div>

        <Application antialias={false} backgroundAlpha={0} height={viewportSize.height} resolution={window.devicePixelRatio || 1} width={viewportSize.width}>
          <pixiContainer x={offsetX} y={offsetY}>
            <pixiContainer scale={displayScale}>
              {snapshot.backgrounds.map((background) => (
                <BackgroundLayerNode background={background} currentFrame={session.currentFrame} key={background.id} />
              ))}
              {snapshot.objects.map((group) => (
                <ObjectLayerNode currentFrame={session.currentFrame} group={group} key={group.id} />
              ))}
              <TimeLayerNode widgets={snapshot.widgets} />
            </pixiContainer>
          </pixiContainer>
        </Application>

        <svg className="viewport-overlay">
          <g transform={`translate(${offsetX} ${offsetY}) scale(${displayScale})`}>
            <rect className="stage-boundary" height={snapshot.matrix.height} width={snapshot.matrix.width} x={0} y={0} />

            {displayScale >= 12
              ? Array.from({ length: snapshot.matrix.width + 1 }, (_, index) => (
                  <line key={`grid-x-${index}`} className="grid-line" x1={index} x2={index} y1={0} y2={snapshot.matrix.height} />
                ))
              : null}
            {displayScale >= 12
              ? Array.from({ length: snapshot.matrix.height + 1 }, (_, index) => (
                  <line key={`grid-y-${index}`} className="grid-line" x1={0} x2={snapshot.matrix.width} y1={index} y2={index} />
                ))
              : null}

            {hoverOverlay ? (
              <rect
                className={`hover-rect ${hoverOverlay.kind}`}
                height={hoverOverlay.rect.height}
                width={hoverOverlay.rect.width}
                x={hoverOverlay.rect.x}
                y={hoverOverlay.rect.y}
              />
            ) : null}

            {snapshot.backgrounds.map((background) =>
              session.selection.kind === 'background' && session.selection.ids.includes(background.id) ? (
                <rect
                  className="selection-rect background"
                  height={background.transform.height}
                  key={`bg-${background.id}`}
                  width={background.transform.width}
                  x={background.transform.x}
                  y={background.transform.y}
                />
              ) : null,
            )}

            {snapshot.objects.map((group) => (
              <g key={`object-overlay-${group.id}`}>
                {session.selection.kind === 'object' && session.selection.ids.includes(group.id) && group.bounds ? (
                  <rect className="selection-rect object" height={group.bounds.height} width={group.bounds.width} x={group.bounds.x} y={group.bounds.y} />
                ) : null}
                {session.selection.kind === 'object' && session.selection.ids.includes(group.id) && group.motion.points.length > 0 ? (
                  <>
                    <polyline
                      className="path-line"
                      points={group.motion.points.map((point) => `${point.x},${point.y}`).join(' ')}
                    />
                    {group.motion.points.map((point) => (
                      <circle className="path-point" cx={point.x} cy={point.y} key={point.id} r={1.2} />
                    ))}
                  </>
                ) : null}
              </g>
            ))}

            {snapshot.widgets.map((widget) =>
              session.selection.kind === 'time' && session.selection.ids.includes(widget.id) ? (
                <rect
                  className="selection-rect time"
                  height={widget.bounds.height}
                  key={`time-${widget.id}`}
                  width={widget.bounds.width}
                  x={widget.bounds.x}
                  y={widget.bounds.y}
                />
              ) : null,
            )}
          </g>
        </svg>
      </div>
    </section>
  )
}

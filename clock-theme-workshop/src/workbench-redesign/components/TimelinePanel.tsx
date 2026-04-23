import { useEffect, useRef, useState } from 'react'
import { commandBus } from '../../editor/core/commandBus'
import {
  clampLayerTiming,
  clampSceneDuration,
  getActiveScene,
  getDocumentFrameCount,
} from '../../editor/core/themeDocument'
import { useEditorStore } from '../../editor/state/editorStore'
import type { LayerTiming, Scene, TimelineLayerKind } from '../../editor/types'

function getTransitionLabel(value: 'cut' | 'fade'): string {
  return value === 'cut' ? '直接切换' : '淡入淡出'
}

type SceneResizeDraft = {
  kind: 'scene-resize'
  id: string
  originX: number
  laneWidth: number
  frameCount: number
  durationFrames: number
  deltaFrames: number
}

type LayerDraftMode = 'move' | 'trim-start' | 'trim-end'

type LayerTimingDraft = {
  kind: 'layer-timing'
  id: string
  layerKind: TimelineLayerKind
  mode: LayerDraftMode
  originX: number
  laneWidth: number
  durationFrames: number
  timing: LayerTiming
  deltaFrames: number
}

function sumSceneDurations(scenes: Scene[]): number {
  return scenes.reduce((total, scene) => total + scene.durationFrames, 0)
}

function clampFrameDelta(originX: number, currentX: number, laneWidth: number, frameCount: number): number {
  if (laneWidth <= 0) {
    return 0
  }
  if (frameCount <= 0) {
    return 0
  }
  return Math.round(((currentX - originX) / laneWidth) * frameCount)
}

function buildSceneTimingMap(scenes: Scene[], draft: SceneResizeDraft | null): Record<string, LayerTiming> {
  let startFrame = 0
  const map: Record<string, LayerTiming> = {}

  for (const scene of scenes) {
    let durationFrames = scene.durationFrames
    if (draft && draft.id === scene.id) {
      durationFrames = clampSceneDuration(draft.durationFrames + draft.deltaFrames)
    }

    map[scene.id] = {
      startFrame,
      endFrame: startFrame + durationFrames - 1,
    }
    startFrame += durationFrames
  }

  return map
}

function buildMovedTiming(timing: LayerTiming, deltaFrames: number, durationFrames: number): LayerTiming {
  const spanFrames = timing.endFrame - timing.startFrame
  const maxStartFrame = Math.max(0, durationFrames - spanFrames - 1)
  const startFrame = Math.max(0, Math.min(maxStartFrame, timing.startFrame + deltaFrames))
  return {
    startFrame,
    endFrame: startFrame + spanFrames,
  }
}

function buildTrimmedTiming(
  timing: LayerTiming,
  deltaFrames: number,
  durationFrames: number,
  mode: LayerDraftMode,
): LayerTiming {
  if (mode === 'move') {
    return buildMovedTiming(timing, deltaFrames, durationFrames)
  }

  if (mode === 'trim-start') {
    return clampLayerTiming(
      {
        startFrame: timing.startFrame + deltaFrames,
        endFrame: timing.endFrame,
      },
      durationFrames,
    )
  }

  return clampLayerTiming(
    {
      startFrame: timing.startFrame,
      endFrame: timing.endFrame + deltaFrames,
    },
    durationFrames,
  )
}

function SceneTrack({
  activeSceneId,
  frameCount,
  scenes,
}: {
  activeSceneId: string
  frameCount: number
  scenes: Scene[]
}) {
  const laneRef = useRef<HTMLDivElement | null>(null)
  const [draft, setDraft] = useState<SceneResizeDraft | null>(null)

  useEffect(() => {
    if (!draft) {
      return
    }

    const handlePointerMove = (event: PointerEvent) => {
      setDraft((current) => {
        if (!current) {
          return current
        }
        return {
          ...current,
          deltaFrames: clampFrameDelta(
            current.originX,
            event.clientX,
            current.laneWidth,
            current.frameCount,
          ),
        }
      })
    }

    const handlePointerUp = () => {
      setDraft((current) => {
        if (!current) {
          return current
        }
        const durationFrames = clampSceneDuration(current.durationFrames + current.deltaFrames)
        commandBus.execute({
          type: 'scene.trim',
          id: current.id,
          durationFrames,
        })
        return null
      })
    }

    window.addEventListener('pointermove', handlePointerMove)
    window.addEventListener('pointerup', handlePointerUp)

    return () => {
      window.removeEventListener('pointermove', handlePointerMove)
      window.removeEventListener('pointerup', handlePointerUp)
    }
  }, [draft])

  const sceneTimingMap = buildSceneTimingMap(scenes, draft)
  const resolvedFrameCount = sumSceneDurations(
    scenes.map((scene) => {
      if (!draft || draft.id !== scene.id) {
        return scene
      }
      return {
        ...scene,
        durationFrames: clampSceneDuration(draft.durationFrames + draft.deltaFrames),
      }
    }),
  )

  return (
    <div className="mx-track-row">
      <div className="mx-track-meta">
        <div className="mx-track-name">场景片段轨</div>
      </div>

      <div ref={laneRef} className="mx-track-lane is-absolute">
        {scenes.map((scene, index) => {
          const timing = sceneTimingMap[scene.id]
          const durationFrames = timing.endFrame - timing.startFrame + 1

          return (
            <div
              key={scene.id}
              className={activeSceneId === scene.id ? 'mx-track-clip is-scene is-selected' : 'mx-track-clip is-scene'}
              draggable
              onDragOver={(event) => event.preventDefault()}
              onDragStart={(event) => event.dataTransfer.setData('text/plain', scene.id)}
              onDrop={(event) => {
                event.preventDefault()
                const draggedId = event.dataTransfer.getData('text/plain')
                if (draggedId.length > 0) {
                  commandBus.execute({ type: 'scene.reorder', id: draggedId, toIndex: index })
                }
              }}
              style={{
                left: `${(timing.startFrame / resolvedFrameCount) * 100}%`,
                width: `${(durationFrames / resolvedFrameCount) * 100}%`,
              }}
            >
              <button
                className="mx-track-clip-main"
                onClick={() => commandBus.execute({ type: 'scene.select', id: scene.id })}
                type="button"
              >
                <span>{scene.name}</span>
                <small>{durationFrames}f</small>
              </button>
              <span
                className="mx-track-handle is-end"
                onPointerDown={(event) => {
                  event.preventDefault()
                  event.stopPropagation()
                  const lane = laneRef.current
                  if (!lane) {
                    return
                  }
                  setDraft({
                    kind: 'scene-resize',
                    id: scene.id,
                    originX: event.clientX,
                    laneWidth: lane.getBoundingClientRect().width,
                    frameCount,
                    durationFrames: scene.durationFrames,
                    deltaFrames: 0,
                  })
                }}
              />
            </div>
          )
        })}
      </div>
    </div>
  )
}

function LayerTrackRow({
  items,
  kind,
  label,
  selectedId,
  sceneDuration,
}: {
  kind: TimelineLayerKind
  label: string
  selectedId: string
  sceneDuration: number
  items: Array<{ id: string; label: string; timing: LayerTiming }>
}) {
  const laneRef = useRef<HTMLDivElement | null>(null)
  const [draft, setDraft] = useState<LayerTimingDraft | null>(null)

  useEffect(() => {
    if (!draft) {
      return
    }

    const handlePointerMove = (event: PointerEvent) => {
      setDraft((current) => {
        if (!current) {
          return current
        }
        return {
          ...current,
          deltaFrames: clampFrameDelta(
            current.originX,
            event.clientX,
            current.laneWidth,
            current.durationFrames,
          ),
        }
      })
    }

    const handlePointerUp = () => {
      setDraft((current) => {
        if (!current) {
          return current
        }
        commandBus.execute({
          type: 'layer.setTiming',
          kind: current.layerKind,
          id: current.id,
          timing: buildTrimmedTiming(
            current.timing,
            current.deltaFrames,
            current.durationFrames,
            current.mode,
          ),
        })
        return null
      })
    }

    window.addEventListener('pointermove', handlePointerMove)
    window.addEventListener('pointerup', handlePointerUp)

    return () => {
      window.removeEventListener('pointermove', handlePointerMove)
      window.removeEventListener('pointerup', handlePointerUp)
    }
  }, [draft])

  return (
    <div className="mx-track-row">
      <div className="mx-track-meta">
        <div className="mx-track-name">{label}</div>
      </div>

      <div ref={laneRef} className="mx-track-lane is-absolute">
        {items.length === 0 ? <div className="mx-track-empty">当前场景暂无内容</div> : null}
        {items.map((item) => {
          let timing = item.timing
          if (draft && draft.id === item.id) {
            timing = buildTrimmedTiming(draft.timing, draft.deltaFrames, draft.durationFrames, draft.mode)
          }
          const blockWidth = timing.endFrame - timing.startFrame + 1

          return (
            <div
              key={item.id}
              className={item.id === selectedId ? 'mx-track-clip is-selected' : 'mx-track-clip'}
              style={{
                left: `${(timing.startFrame / sceneDuration) * 100}%`,
                width: `${(blockWidth / sceneDuration) * 100}%`,
              }}
            >
              <span
                className="mx-track-handle is-start"
                onPointerDown={(event) => {
                  event.preventDefault()
                  event.stopPropagation()
                  const lane = laneRef.current
                  if (!lane) {
                    return
                  }
                  commandBus.execute({ type: 'selection.set', kind, ids: [item.id], additive: false })
                  setDraft({
                    kind: 'layer-timing',
                    id: item.id,
                    layerKind: kind,
                    mode: 'trim-start',
                    originX: event.clientX,
                    laneWidth: lane.getBoundingClientRect().width,
                    durationFrames: sceneDuration,
                    timing: item.timing,
                    deltaFrames: 0,
                  })
                }}
              />
              <button
                className="mx-track-clip-main"
                onClick={() => commandBus.execute({ type: 'selection.set', kind, ids: [item.id], additive: false })}
                onPointerDown={(event) => {
                  const lane = laneRef.current
                  if (!lane) {
                    return
                  }
                  commandBus.execute({ type: 'selection.set', kind, ids: [item.id], additive: false })
                  setDraft({
                    kind: 'layer-timing',
                    id: item.id,
                    layerKind: kind,
                    mode: 'move',
                    originX: event.clientX,
                    laneWidth: lane.getBoundingClientRect().width,
                    durationFrames: sceneDuration,
                    timing: item.timing,
                    deltaFrames: 0,
                  })
                }}
                type="button"
              >
                <span>{item.label}</span>
                <small>
                  {timing.startFrame}-{timing.endFrame}
                </small>
              </button>
              <span
                className="mx-track-handle is-end"
                onPointerDown={(event) => {
                  event.preventDefault()
                  event.stopPropagation()
                  const lane = laneRef.current
                  if (!lane) {
                    return
                  }
                  commandBus.execute({ type: 'selection.set', kind, ids: [item.id], additive: false })
                  setDraft({
                    kind: 'layer-timing',
                    id: item.id,
                    layerKind: kind,
                    mode: 'trim-end',
                    originX: event.clientX,
                    laneWidth: lane.getBoundingClientRect().width,
                    durationFrames: sceneDuration,
                    timing: item.timing,
                    deltaFrames: 0,
                  })
                }}
              />
            </div>
          )
        })}
      </div>
    </div>
  )
}

function TransitionTrack({
  activeSceneId,
  scenes,
}: {
  activeSceneId: string
  scenes: Scene[]
}) {
  let startFrame = 0
  const frameCount = sumSceneDurations(scenes)

  return (
    <div className="mx-track-row">
      <div className="mx-track-meta">
        <div className="mx-track-name">转场轨</div>
      </div>
      <div className="mx-track-lane is-absolute">
        {scenes.map((scene) => {
          const timing = {
            startFrame,
            endFrame: startFrame + scene.durationFrames - 1,
          }
          startFrame += scene.durationFrames

          return (
            <button
              key={scene.id}
              className={scene.id === activeSceneId ? 'mx-track-clip-main is-transition is-selected' : 'mx-track-clip-main is-transition'}
              onClick={() =>
                commandBus.execute({
                  type: 'scene.update',
                  id: scene.id,
                  patch: { transitionOut: scene.transitionOut === 'cut' ? 'fade' : 'cut' },
                })
              }
              style={{
                position: 'absolute',
                left: `${(timing.startFrame / frameCount) * 100}%`,
                width: `${(scene.durationFrames / frameCount) * 100}%`,
              }}
              type="button"
            >
              {getTransitionLabel(scene.transitionOut)}
            </button>
          )
        })}
      </div>
    </div>
  )
}

export function TimelinePanel() {
  const document = useEditorStore((state) => state.document)
  const session = useEditorStore((state) => state.session)
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const frameCount = getDocumentFrameCount(document)
  const playheadPercent = frameCount <= 1 ? 0 : (session.currentFrame / (frameCount - 1)) * 100

  let localFrame = session.currentFrame
  for (const scene of document.scenes) {
    if (scene.id === activeScene.id) {
      break
    }
    localFrame -= scene.durationFrames
  }

  const selectedSceneId = session.selection.kind === 'scene' ? session.selection.ids[0] : activeScene.id
  const supportsKeyframeTools =
    session.selection.kind === 'background' || session.selection.kind === 'object'

  return (
    <section className="mx-pane mx-timeline-pane">
      <div className="mx-timeline-toolbar">
        {supportsKeyframeTools ? (
          <div className="mx-track-actions">
            <button
              className="mx-track-mini"
              onClick={() => commandBus.execute({ type: 'keyframe.seedCurrent' })}
              type="button"
            >
              写入当前帧
            </button>
            <button
              className="mx-track-mini"
              onClick={() => commandBus.execute({ type: 'keyframe.copyPrevious' })}
              type="button"
            >
              复制上一帧
            </button>
            <button
              className="mx-track-mini"
              onClick={() => commandBus.execute({ type: 'keyframe.clearCurrent' })}
              type="button"
            >
              清除当前帧
            </button>
          </div>
        ) : null}

        <div className="mx-timeline-readout">
          <span>场景 {activeScene.name}</span>
          <span>局部 {localFrame}</span>
          <span>全局 {session.currentFrame} / {frameCount - 1}</span>
        </div>
      </div>

      <div className="mx-timeline-body" style={{ ['--mx-playhead-left' as string]: `${playheadPercent}%` }}>
        <div className="mx-timeline-ruler">
          <input
            max={Math.max(0, frameCount - 1)}
            min={0}
            onChange={(event) => commandBus.execute({ type: 'timeline.setPlayhead', frame: Number(event.target.value) })}
            type="range"
            value={session.currentFrame}
          />
          <div className="mx-ruler-ticks">
            {Array.from({ length: Math.min(frameCount, 8) }, (_, index) => {
              const divisor = Math.max(1, Math.min(frameCount, 8) - 1)
              const tick = Math.round((index / divisor) * Math.max(0, frameCount - 1))
              return <span key={`tick-${tick}`}>{tick}</span>
            })}
          </div>
        </div>

        <SceneTrack activeSceneId={selectedSceneId} frameCount={frameCount} scenes={document.scenes} />

        <LayerTrackRow
          kind="pixel"
          label="像素层轨"
          sceneDuration={activeScene.durationFrames}
          selectedId={session.selection.kind === 'pixel' ? session.selection.ids[0] : ''}
          items={activeScene.pixelLayers.map((layer) => ({
            id: layer.id,
            label: layer.name,
            timing: layer.timing,
          }))}
        />

        <LayerTrackRow
          kind="object"
          label="对象层轨"
          sceneDuration={activeScene.durationFrames}
          selectedId={session.selection.kind === 'object' ? session.selection.ids[0] : ''}
          items={activeScene.objectGroups.map((group) => ({
            id: group.id,
            label: group.name,
            timing: group.timing,
          }))}
        />

        <LayerTrackRow
          kind="time"
          label="时间层轨"
          sceneDuration={activeScene.durationFrames}
          selectedId={session.selection.kind === 'time' ? session.selection.ids[0] : ''}
          items={activeScene.timeWidgets.map((widget) => ({
            id: widget.id,
            label: widget.label,
            timing: widget.timing,
          }))}
        />

        <TransitionTrack activeSceneId={activeScene.id} scenes={document.scenes} />

        <div className="mx-track-row">
          <div className="mx-track-meta">
            <div className="mx-track-name">主题规则轨</div>
          </div>
          <div className="mx-track-lane">
            <div className="mx-rule-segment">
              <button
                className={document.themeRules.paletteLock ? 'mx-segment is-active' : 'mx-segment'}
                onClick={() =>
                  commandBus.execute({
                    type: 'themeRule.update',
                    patch: { paletteLock: !document.themeRules.paletteLock },
                  })
                }
                type="button"
              >
                调色板 {document.themeRules.paletteLock ? '锁定' : '未锁定'}
              </button>
              <span>
                矩阵 {document.themeRules.matrixBound.width}x{document.themeRules.matrixBound.height}
              </span>
              <span>网格 {document.themeRules.pixelGridSize}</span>
            </div>
          </div>
        </div>

        <div className="mx-playhead" />
      </div>
    </section>
  )
}

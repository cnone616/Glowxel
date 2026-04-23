import { hasKeyframe } from '../core/themeDocument'
import { commandBus } from '../core/commandBus'
import { useEditorStore } from '../state/editorStore'

function FrameChip({
  active,
  frameIndex,
  hasMarker,
  onClick,
  state,
}: {
  active: boolean
  frameIndex: number
  hasMarker: boolean
  onClick: () => void
  state?: boolean
}) {
  const labelVisible = frameIndex % 4 === 0 || active

  return (
    <button
      className={active ? `frame-chip ${state ? 'state' : ''} active` : `frame-chip ${state ? 'state' : ''}`}
      onClick={onClick}
      type="button"
    >
      <span className="frame-index">{labelVisible ? frameIndex + 1 : ''}</span>
      <i className={hasMarker ? `frame-dot ${state ? 'state' : ''} active` : `frame-dot ${state ? 'state' : ''}`} />
    </button>
  )
}

export function TimelinePane() {
  const document = useEditorStore((state) => state.document)
  const session = useEditorStore((state) => state.session)

  const selectedBackground =
    session.selection.kind === 'background'
      ? document.backgroundLayers.find((item) => item.id === session.selection.ids[0]) ?? null
      : null
  const selectedObject =
    session.selection.kind === 'object'
      ? document.objectGroups.find((item) => item.id === session.selection.ids[0]) ?? null
      : null

  const timelineTarget = selectedBackground
    ? {
        kind: 'background',
        label: selectedBackground.name,
        subtitle: '背景层关键帧',
        transformKeys: selectedBackground.transformKeys,
        stateKeys: selectedBackground.stateKeys,
      }
    : selectedObject
      ? {
          kind: 'object',
          label: selectedObject.name,
          subtitle: '对象关键帧',
          transformKeys: selectedObject.transformKeys,
          stateKeys: selectedObject.stateKeys,
        }
      : null

  const selectionEmptyMessage =
    session.selection.kind === 'time' && session.selection.ids.length > 0
      ? '时间组件当前版本还没有独立轨道视图，先选背景或对象来写关键帧。'
      : '先从左侧或舞台选中一个背景层或对象，时间轴才会进入可编辑状态。'

  const canEditTimeline = timelineTarget !== null

  return (
    <section className="timeline-pane">
      <div className="pane-head timeline-head">
        <div>
          <span className="eyebrow-label">Timeline</span>
          <h2>关键帧时间轴</h2>
        </div>
        <div className="timeline-actions">
          <button
            className="ghost-inline"
            disabled={!canEditTimeline}
            onClick={() => commandBus.execute({ type: 'keyframe.seedCurrent' })}
            type="button"
          >
            写入当前帧
          </button>
          <button
            className="ghost-inline"
            disabled={!canEditTimeline}
            onClick={() => commandBus.execute({ type: 'keyframe.copyPrevious' })}
            type="button"
          >
            复制上一帧
          </button>
          <button
            className="ghost-inline"
            disabled={!canEditTimeline}
            onClick={() => commandBus.execute({ type: 'keyframe.clearCurrent' })}
            type="button"
          >
            清除当前帧
          </button>
        </div>
      </div>

      <div className="timeline-context-bar">
        <div className="timeline-context-main">
          <span className="timeline-context-kicker">{timelineTarget ? timelineTarget.subtitle : '待选择轨道'}</span>
          <strong>{timelineTarget ? timelineTarget.label : '没有激活的关键帧对象'}</strong>
          <span>{timelineTarget ? `当前编辑 ${timelineTarget.kind === 'background' ? '背景层' : '对象组'} 的变换与状态轨。` : selectionEmptyMessage}</span>
        </div>

        <div className="timeline-summary-strip">
          <div className="summary-pill">
            <strong>当前帧</strong>
            <span>{session.currentFrame + 1}</span>
          </div>
          <div className="summary-pill">
            <strong>总帧数</strong>
            <span>{document.meta.frameCount}</span>
          </div>
          <div className="summary-pill">
            <strong>变换键</strong>
            <span>{timelineTarget ? timelineTarget.transformKeys.length : 0}</span>
          </div>
          <div className="summary-pill">
            <strong>状态键</strong>
            <span>{timelineTarget ? timelineTarget.stateKeys.length : 0}</span>
          </div>
        </div>
      </div>

      <div className="timeline-playhead">
        <input
          max={Math.max(0, document.meta.frameCount - 1)}
          min={0}
          onChange={(event) =>
            commandBus.execute({
              type: 'session.setFrame',
              frame: Number(event.target.value),
            })
          }
          type="range"
          value={session.currentFrame}
        />
        <span>
          第 {session.currentFrame + 1} / {document.meta.frameCount} 帧
        </span>
      </div>

      <div className="timeline-ruler">
        <div className="timeline-ruler-label">
          <strong>帧标尺</strong>
          <span>点击任意帧跳转播放头</span>
        </div>
        <div className="timeline-ruler-strip">
          {Array.from({ length: document.meta.frameCount }, (_, frameIndex) => (
            <button
              key={`ruler-${frameIndex}`}
              className={frameIndex === session.currentFrame ? 'timeline-ruler-chip active' : 'timeline-ruler-chip'}
              onClick={() => commandBus.execute({ type: 'session.setFrame', frame: frameIndex })}
              type="button"
            >
              {frameIndex + 1}
            </button>
          ))}
        </div>
      </div>

      <div className="timeline-track-list">
        {timelineTarget ? (
          <>
            <div className="timeline-track-row">
              <div className="timeline-track-meta">
                <strong>变换轨</strong>
                <span>位置、尺寸、透明度</span>
              </div>
              <div className="timeline-frame-strip">
                {Array.from({ length: document.meta.frameCount }, (_, frameIndex) => (
                  <FrameChip
                    active={frameIndex === session.currentFrame}
                    frameIndex={frameIndex}
                    hasMarker={hasKeyframe(timelineTarget.transformKeys as Array<{ frame: number }>, frameIndex)}
                    key={`transform-${frameIndex}`}
                    onClick={() => commandBus.execute({ type: 'session.setFrame', frame: frameIndex })}
                  />
                ))}
              </div>
            </div>

            <div className="timeline-track-row">
              <div className="timeline-track-meta">
                <strong>状态轨</strong>
                <span>显隐、行为与节奏</span>
              </div>
              <div className="timeline-frame-strip">
                {Array.from({ length: document.meta.frameCount }, (_, frameIndex) => (
                  <FrameChip
                    active={frameIndex === session.currentFrame}
                    frameIndex={frameIndex}
                    hasMarker={hasKeyframe(timelineTarget.stateKeys as Array<{ frame: number }>, frameIndex)}
                    key={`state-${frameIndex}`}
                    onClick={() => commandBus.execute({ type: 'session.setFrame', frame: frameIndex })}
                    state
                  />
                ))}
              </div>
            </div>
          </>
        ) : (
          <div className="timeline-empty-state">
            <strong>时间轴等待绑定对象</strong>
            <span>{selectionEmptyMessage}</span>
          </div>
        )}
      </div>
    </section>
  )
}

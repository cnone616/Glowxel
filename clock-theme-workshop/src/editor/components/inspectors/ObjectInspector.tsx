import { commandBus } from '../../core/commandBus'
import { useEditorStore } from '../../state/editorStore'

export function ObjectInspector() {
  const document = useEditorStore((state) => state.document)
  const snapshot = useEditorStore((state) => state.snapshot)
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])

  const objectGroup = document.objectGroups.find((item) => item.id === selectionId) ?? null
  const view = snapshot.objects.find((item) => item.id === selectionId) ?? null

  if (!objectGroup || !view) {
    return <div className="empty-state">先选中一个对象。</div>
  }

  return (
    <div className="inspector-panel">
      <div className="inspector-head">
        <span className="eyebrow-label">Object</span>
        <h3>{objectGroup.name}</h3>
      </div>

      <label className="field-label">
        <span>名称</span>
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'outline.rename',
              kind: 'object',
              id: objectGroup.id,
              name: event.target.value,
            })
          }
          type="text"
          value={objectGroup.name}
        />
      </label>

      <div className="segmented-row">
        {[
          { id: 'fixed', label: '静止' },
          { id: 'blink', label: '闪烁' },
          { id: 'path', label: '路径' },
        ].map((behavior) => (
          <button
            key={behavior.id}
            className={view.state.behavior === behavior.id ? 'segment-button active' : 'segment-button'}
            onClick={() =>
              commandBus.execute({
                type: 'object.updateState',
                id: objectGroup.id,
                patch: {
                  behavior: behavior.id as 'fixed' | 'blink' | 'path',
                },
              })
            }
            type="button"
          >
            {behavior.label}
          </button>
        ))}
      </div>

      <div className="number-grid">
        <NumberField label="X" value={Math.round(view.transform.x)} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { x: value } })} />
        <NumberField label="Y" value={Math.round(view.transform.y)} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { y: value } })} />
        <NumberField label="缩放" step={0.05} value={view.transform.scale} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { scale: Math.max(0.2, value) } })} />
        <NumberField label="透明度" step={0.05} value={view.transform.opacity} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { opacity: value } })} />
        <NumberField label="序列步长" value={view.state.sequenceStepFrames} onChange={(value) => commandBus.execute({ type: 'object.updateState', id: objectGroup.id, patch: { sequenceStepFrames: Math.max(1, value) } })} />
        <NumberField label="闪烁节奏" value={view.state.rhythmFrames} onChange={(value) => commandBus.execute({ type: 'object.updateState', id: objectGroup.id, patch: { rhythmFrames: Math.max(1, value) } })} />
      </div>

      <label className="field-label">
        <span>路径模式</span>
        <div className="segmented-row">
          {[
            { id: 'loop', label: '循环' },
            { id: 'pingpong', label: '往返' },
            { id: 'once', label: '单次' },
          ].map((mode) => (
            <button
              key={mode.id}
              className={objectGroup.motion.mode === mode.id ? 'segment-button active' : 'segment-button'}
              onClick={() =>
                commandBus.execute({
                  type: 'object.updateMotion',
                  id: objectGroup.id,
                  patch: { mode: mode.id as 'loop' | 'pingpong' | 'once' },
                })
              }
              type="button"
            >
              {mode.label}
            </button>
          ))}
        </div>
      </label>

      <label className="field-label">
        <span>路径持续帧数</span>
        <input
          min={4}
          onChange={(event) =>
            commandBus.execute({
              type: 'object.updateMotion',
              id: objectGroup.id,
              patch: { durationFrames: Math.max(4, Number(event.target.value)) },
            })
          }
          step={1}
          type="number"
          value={objectGroup.motion.durationFrames}
        />
      </label>

      <div className="path-point-list">
        <div className="section-inline-head">
          <strong>路径点</strong>
          <button className="ghost-inline small" onClick={() => commandBus.execute({ type: 'object.clearPath', id: objectGroup.id })} type="button">
            清空
          </button>
        </div>
        {objectGroup.motion.points.length === 0 ? (
          <div className="empty-state compact">切到路径模式后在舞台上点击即可添加路径点。</div>
        ) : (
          objectGroup.motion.points.map((point, index) => (
            <div className="path-point-row" key={point.id}>
              <span>P{index + 1}</span>
              <span>
                {Math.round(point.x)}, {Math.round(point.y)}
              </span>
              <button
                className="ghost-inline small"
                onClick={() =>
                  commandBus.execute({
                    type: 'object.removePathPoint',
                    id: objectGroup.id,
                    pointId: point.id,
                  })
                }
                type="button"
              >
                删除
              </button>
            </div>
          ))
        )}
      </div>
    </div>
  )
}

function NumberField({
  label,
  onChange,
  step,
  value,
}: {
  label: string
  value: number
  onChange: (value: number) => void
  step?: number
}) {
  return (
    <label className="field-label">
      <span>{label}</span>
      <input
        onChange={(event) => onChange(Number(event.target.value))}
        step={step ?? 1}
        type="number"
        value={value}
      />
    </label>
  )
}

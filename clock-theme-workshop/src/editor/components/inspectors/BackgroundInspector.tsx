import { commandBus } from '../../core/commandBus'
import { useEditorStore } from '../../state/editorStore'

export function BackgroundInspector() {
  const document = useEditorStore((state) => state.document)
  const snapshot = useEditorStore((state) => state.snapshot)
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])

  const layer = document.backgroundLayers.find((item) => item.id === selectionId) ?? null
  const view = snapshot.backgrounds.find((item) => item.id === selectionId) ?? null

  if (!layer || !view) {
    return <div className="empty-state">先选中一个背景层。</div>
  }

  return (
    <div className="inspector-panel">
      <div className="inspector-head">
        <span className="eyebrow-label">Background</span>
        <h3>{layer.name}</h3>
      </div>

      <label className="field-label">
        <span>名称</span>
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'outline.rename',
              kind: 'background',
              id: layer.id,
              name: event.target.value,
            })
          }
          type="text"
          value={layer.name}
        />
      </label>

      <label className="field-label">
        <span>绑定素材</span>
        <select
          onChange={(event) =>
            commandBus.execute({
              type: 'background.updateBase',
              id: layer.id,
              patch: { assetId: event.target.value },
            })
          }
          value={layer.assetId}
        >
          <option value="">纯色底板</option>
          {document.assets.map((asset) => (
            <option key={asset.id} value={asset.id}>
              {asset.label}
            </option>
          ))}
        </select>
      </label>

      <label className="field-label">
        <span>底色</span>
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'background.updateBase',
              id: layer.id,
              patch: { baseColor: event.target.value },
            })
          }
          type="color"
          value={layer.baseColor}
        />
      </label>

      <div className="segmented-row">
        {['fixed', 'loop', 'blink'].map((behavior) => (
          <button
            key={behavior}
            className={view.state.behavior === behavior ? 'segment-button active' : 'segment-button'}
            onClick={() =>
              commandBus.execute({
                type: 'background.updateState',
                id: layer.id,
                patch: {
                  behavior: behavior as 'fixed' | 'loop' | 'blink',
                },
              })
            }
            type="button"
          >
            {behavior === 'fixed' ? '固定' : behavior === 'loop' ? '循环' : '闪烁'}
          </button>
        ))}
      </div>

      <div className="number-grid">
        <NumberField label="X" value={Math.round(view.transform.x)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { x: value } })} />
        <NumberField label="Y" value={Math.round(view.transform.y)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { y: value } })} />
        <NumberField label="宽" value={Math.round(view.transform.width)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { width: Math.max(1, value) } })} />
        <NumberField label="高" value={Math.round(view.transform.height)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { height: Math.max(1, value) } })} />
        <NumberField label="透明度" step={0.05} value={view.transform.opacity} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { opacity: value } })} />
        <NumberField label="序列步长" value={view.state.sequenceStepFrames} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { sequenceStepFrames: Math.max(1, value) } })} />
        <NumberField label="闪烁节奏" value={view.state.rhythmFrames} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { rhythmFrames: Math.max(1, value) } })} />
        <NumberField label="速度 X" value={view.state.speedX} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { speedX: value } })} />
        <NumberField label="速度 Y" value={view.state.speedY} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { speedY: value } })} />
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

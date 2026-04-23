import { commandBus } from '../../core/commandBus'
import { useEditorStore } from '../../state/editorStore'

export function TimeInspector() {
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const widget = useEditorStore((state) => state.document.timeWidgets.find((item) => item.id === selectionId) ?? null)

  if (!widget) {
    return <div className="empty-state">先选中一个时间组件。</div>
  }

  return (
    <div className="inspector-panel">
      <div className="inspector-head">
        <span className="eyebrow-label">Time</span>
        <h3>{widget.label}</h3>
      </div>

      <label className="field-label">
        <span>名称</span>
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'outline.rename',
              kind: 'time',
              id: widget.id,
              name: event.target.value,
            })
          }
          type="text"
          value={widget.label}
        />
      </label>

      <label className="field-label">
        <span>类型</span>
        <select
          onChange={(event) =>
            commandBus.execute({
              type: 'time.update',
              id: widget.id,
              patch: {
                type: event.target.value as 'time' | 'date' | 'week' | 'year',
              },
            })
          }
          value={widget.type}
        >
          <option value="time">时间</option>
          <option value="date">日期</option>
          <option value="week">星期</option>
          <option value="year">年份</option>
        </select>
      </label>

      <div className="number-grid">
        <NumberField label="X" value={widget.x} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { x: value } })} />
        <NumberField label="Y" value={widget.y} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { y: value } })} />
        <NumberField label="缩放" value={widget.scale} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { scale: Math.max(1, value) } })} />
        <NumberField label="字间距" value={widget.letterSpacing} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { letterSpacing: Math.max(0, value) } })} />
      </div>

      <label className="field-label">
        <span>颜色</span>
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'time.update',
              id: widget.id,
              patch: { color: event.target.value },
            })
          }
          type="color"
          value={widget.color}
        />
      </label>

      <div className="segmented-row">
        {[24, 12].map((hourFormat) => (
          <button
            key={hourFormat}
            className={widget.hourFormat === hourFormat ? 'segment-button active' : 'segment-button'}
            onClick={() =>
              commandBus.execute({
                type: 'time.update',
                id: widget.id,
                patch: { hourFormat: hourFormat as 12 | 24 },
              })
            }
            type="button"
          >
            {hourFormat}h
          </button>
        ))}
        <button
          className={widget.showSeconds ? 'segment-button active' : 'segment-button'}
          onClick={() =>
            commandBus.execute({
              type: 'time.update',
              id: widget.id,
              patch: { showSeconds: !widget.showSeconds },
            })
          }
          type="button"
        >
          {widget.showSeconds ? '隐藏秒' : '显示秒'}
        </button>
      </div>
    </div>
  )
}

function NumberField({
  label,
  onChange,
  value,
}: {
  label: string
  value: number
  onChange: (value: number) => void
}) {
  return (
    <label className="field-label">
      <span>{label}</span>
      <input
        onChange={(event) => onChange(Number(event.target.value))}
        step={1}
        type="number"
        value={value}
      />
    </label>
  )
}

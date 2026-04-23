import { commandBus } from '../../core/commandBus'
import { useEditorStore } from '../../state/editorStore'

export function SceneInspector() {
  const document = useEditorStore((state) => state.document)

  return (
    <div className="inspector-panel">
      <div className="inspector-head">
        <span className="eyebrow-label">Scene</span>
        <h3>场景总览</h3>
      </div>

      <div className="metric-grid">
        <div>
          <span>背景层</span>
          <strong>{document.backgroundLayers.length}</strong>
        </div>
        <div>
          <span>对象</span>
          <strong>{document.objectGroups.length}</strong>
        </div>
        <div>
          <span>时间组件</span>
          <strong>{document.timeWidgets.length}</strong>
        </div>
        <div>
          <span>素材</span>
          <strong>{document.assets.length}</strong>
        </div>
      </div>

      <label className="field-label">
        <span>项目名</span>
        <input
          onChange={(event) => commandBus.execute({ type: 'document.rename', name: event.target.value })}
          type="text"
          value={document.meta.name}
        />
      </label>

      <label className="field-label">
        <span>帧数</span>
        <input
          min={8}
          onChange={(event) =>
            commandBus.execute({
              type: 'document.setFrameCount',
              value: Number(event.target.value),
            })
          }
          step={1}
          type="number"
          value={document.meta.frameCount}
        />
      </label>

      <label className="field-label">
        <span>帧延时</span>
        <input
          min={40}
          onChange={(event) =>
            commandBus.execute({
              type: 'document.setFrameDelay',
              value: Number(event.target.value),
            })
          }
          step={10}
          type="number"
          value={document.meta.frameDelay}
        />
      </label>

      <label className="field-label">
        <span>预演日期</span>
        <input
          onChange={(event) => commandBus.execute({ type: 'document.setPreviewDate', value: event.target.value })}
          type="date"
          value={document.meta.previewDate}
        />
      </label>

      <label className="field-label">
        <span>预演时间</span>
        <input
          onChange={(event) => commandBus.execute({ type: 'document.setPreviewTime', value: event.target.value })}
          step={1}
          type="time"
          value={document.meta.previewTime}
        />
      </label>
    </div>
  )
}

import { useEditorStore } from '../state/editorStore'
import { BackgroundInspector } from './inspectors/BackgroundInspector'
import { ObjectInspector } from './inspectors/ObjectInspector'
import { SceneInspector } from './inspectors/SceneInspector'
import { TimeInspector } from './inspectors/TimeInspector'

export function InspectorDock() {
  const selection = useEditorStore((state) => state.session.selection)

  return (
    <section className="pane-section inspector-dock">
      <div className="pane-head">
        <div>
          <span className="eyebrow-label">Inspector</span>
          <h2>属性检查器</h2>
        </div>
      </div>

      {selection.kind === 'background' ? <BackgroundInspector /> : null}
      {selection.kind === 'object' ? <ObjectInspector /> : null}
      {selection.kind === 'time' ? <TimeInspector /> : null}
      {selection.kind === 'scene' ? <SceneInspector /> : null}
      {selection.kind !== 'scene' && selection.ids.length === 0 ? <SceneInspector /> : null}
    </section>
  )
}

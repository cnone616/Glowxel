import './styles.css'
import { useEditorStore } from '../editor/state/editorStore'
import { EditorShell } from './components/EditorShell'
import { InspectorPanel } from './components/InspectorPanel'
import { PreviewStage } from './components/PreviewStage'
import { SceneBin } from './components/SceneBin'
import { TimelinePanel } from './components/TimelinePanel'
import { useWorkbenchRuntime } from './useWorkbenchRuntime'

export function MatrixWorkbench() {
  const loaded = useEditorStore((state) => state.runtime.loaded)

  useWorkbenchRuntime()

  if (!loaded) {
    return (
      <div className="matrix-workbench-page">
        <div className="mx-loading-panel">
          <span>Glowxel Theme Editor</span>
          <strong>正在初始化主题编辑器…</strong>
          <small>恢复本地文档、读取素材目录并构建 64×64 渲染工作台。</small>
        </div>
      </div>
    )
  }

  return (
    <EditorShell
      sidebar={<SceneBin />}
      stage={<PreviewStage />}
      inspector={<InspectorPanel />}
      timeline={<TimelinePanel />}
    />
  )
}

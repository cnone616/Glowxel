import { useEditorStore } from '../state/editorStore'

const TOOL_LABELS = {
  select: '选择',
  pan: '平移',
  place: '摆放',
  path: '路径',
} as const

const PREVIEW_LABELS = {
  loop: '常规循环',
  minute: '分钟触发',
  hour: '整点触发',
} as const

function getSelectionLabel(kind: string, count: number): string {
  if (kind === 'scene') {
    return '场景'
  }

  const labelMap: Record<string, string> = {
    background: '背景层',
    object: '对象',
    time: '时间组件',
  }

  return `${labelMap[kind] ?? kind} ${count}`
}

export function EditorStatusBar() {
  const session = useEditorStore((state) => state.session)
  const document = useEditorStore((state) => state.document)

  return (
    <footer className="editor-status-bar">
      <div className="status-cluster">
        <span className="status-pill emphasized">选择 {getSelectionLabel(session.selection.kind, session.selection.ids.length)}</span>
        <span className="status-pill">工具 {TOOL_LABELS[session.currentTool]}</span>
        <span className="status-pill">帧 {session.currentFrame + 1} / {document.meta.frameCount}</span>
        <span className="status-pill">缩放 {session.viewportMode === 'fit' ? '适配' : `${session.viewportZoom}x`}</span>
        <span className="status-pill">预演 {PREVIEW_LABELS[document.meta.previewMode]}</span>
        <span className="status-pill">渲染 {session.performanceMs.toFixed(1)}ms / {session.performanceFps}fps</span>
      </div>

      <div className="status-cluster status-cluster-right">
        {session.notice ? (
          <span className={`status-notice ${session.notice.tone}`}>
            {session.notice.text}
          </span>
        ) : (
          <span className="status-hint">Space 临时平移 · 0 适配视图 · 1/2/4 切换倍率 · Shift 多选</span>
        )}
      </div>
    </footer>
  )
}

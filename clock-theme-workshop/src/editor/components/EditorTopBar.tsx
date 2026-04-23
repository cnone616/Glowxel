import { useRef } from 'react'
import type { ChangeEvent, ReactNode } from 'react'
import { useNavigate } from 'react-router-dom'
import { commandBus } from '../core/commandBus'
import { cloneDocument } from '../core/clone'
import { saveStoredDocument } from '../core/storage'
import { ensureThemeDocument } from '../core/themeDocument'
import { useEditorStore } from '../state/editorStore'

function formatSaveLabel(saveState: string, lastSavedAt: string): string {
  if (saveState === 'saving') {
    return '正在保存'
  }
  if (saveState === 'saved') {
    return lastSavedAt.length > 0 ? `已保存 ${lastSavedAt}` : '已保存'
  }
  if (saveState === 'dirty') {
    return '等待保存'
  }
  if (saveState === 'error') {
    return '保存失败'
  }
  return '未保存'
}

function TopBarActionGroup({
  children,
  label,
}: {
  label: string
  children: ReactNode
}) {
  return (
    <div className="topbar-action-group">
      <span className="action-group-label">{label}</span>
      <div className="topbar-action-row">{children}</div>
    </div>
  )
}

export function EditorTopBar() {
  const navigate = useNavigate()
  const inputRef = useRef<HTMLInputElement | null>(null)
  const themeDocument = useEditorStore((state) => state.document)
  const session = useEditorStore((state) => state.session)
  const history = useEditorStore((state) => state.history)

  const emitNotice = (
    text: string,
    tone: 'info' | 'success' | 'warning' | 'error' = 'info',
    source: 'topbar' | 'timeline' | 'viewport' | 'system' = 'topbar',
  ) => {
    commandBus.execute({
      type: 'session.setNotice',
      notice: {
        text,
        tone,
        source,
      },
    })
  }

  const handleImport = async (event: ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0]
    if (!file) {
      return
    }

    try {
      const text = await file.text()
      const parsed = JSON.parse(text)
      const nextDocument = ensureThemeDocument(parsed)
      commandBus.execute({ type: 'document.import', document: nextDocument })
      emitNotice(`已导入 ${file.name}`, 'success')
    } catch (error) {
      emitNotice(error instanceof Error ? error.message : '导入失败', 'error')
    } finally {
      event.target.value = ''
    }
  }

  const handleExport = () => {
    const blob = new Blob([JSON.stringify(themeDocument, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const anchor = window.document.createElement('a')
    anchor.href = url
    anchor.download = `${themeDocument.meta.name || 'glowxel-theme'}.json`
    anchor.click()
    URL.revokeObjectURL(url)
    emitNotice('已导出主题 JSON', 'success')
  }

  const handleSaveNow = async () => {
    try {
      useEditorStore.getState().updateSession({ saveState: 'saving' })
      await saveStoredDocument(cloneDocument(themeDocument))
      const savedAt = new Date().toLocaleTimeString('zh-CN', { hour12: false })
      commandBus.execute({
        type: 'persistence.markSaved',
        at: savedAt,
      })
      emitNotice(`已手动保存 ${savedAt}`, 'success')
    } catch {
      commandBus.execute({ type: 'persistence.markError' })
      emitNotice('保存失败，请检查本地存储或文档格式', 'error')
    }
  }

  const saveToneClass =
    session.saveState === 'error'
      ? 'error'
      : session.saveState === 'saved'
        ? 'success'
        : session.saveState === 'saving'
          ? 'warning'
          : ''

  return (
    <header className="editor-top-bar">
      <div className="brand-cluster">
        <span className="eyebrow-label">Glowxel Workshop</span>
        <strong>{themeDocument.meta.name || '未命名主题'}</strong>
        <div className="brand-meta-row">
          <span>{themeDocument.meta.matrix.width}×{themeDocument.meta.matrix.height} 矩阵</span>
          <span>{themeDocument.meta.frameCount} 帧</span>
          <span>{themeDocument.meta.frameDelay}ms / 帧</span>
        </div>
      </div>

      <div className="topbar-main">
        <div className="topbar-stack">
          <label className="topbar-field project-name">
            <span>项目名</span>
            <input
              onChange={(event) => commandBus.execute({ type: 'document.rename', name: event.target.value })}
              type="text"
              value={themeDocument.meta.name}
            />
          </label>

          <div className="topbar-field">
            <span>工作台摘要</span>
            <div className="topbar-summary-strip">
              <div className="summary-pill">
                <strong>撤销</strong>
                <span>{history.past.length}</span>
              </div>
              <div className="summary-pill">
                <strong>重做</strong>
                <span>{history.future.length}</span>
              </div>
              <div className="summary-pill">
                <strong>素材</strong>
                <span>{themeDocument.assets.length}</span>
              </div>
              <div className="summary-pill">
                <strong>对象</strong>
                <span>{themeDocument.objectGroups.length + themeDocument.backgroundLayers.length + themeDocument.timeWidgets.length}</span>
              </div>
            </div>
          </div>
        </div>

        <div className="topbar-stack">
          <div className="topbar-field">
            <span>预演模式</span>
            <div className="segmented-row">
              {[
                { id: 'loop', label: '常规' },
                { id: 'minute', label: '分钟' },
                { id: 'hour', label: '整点' },
              ].map((item) => (
                <button
                  key={item.id}
                  className={themeDocument.meta.previewMode === item.id ? 'segment-button active' : 'segment-button'}
                  onClick={() =>
                    commandBus.execute({
                      type: 'session.setPreviewMode',
                      previewMode: item.id as 'loop' | 'minute' | 'hour',
                    })
                  }
                  type="button"
                >
                  {item.label}
                </button>
              ))}
            </div>
          </div>

          <div className="topbar-field">
            <span>交付路径</span>
            <div className="topbar-summary-strip">
              <div className="summary-pill">
                <strong>导入 / 导出</strong>
                <span>主题合同</span>
              </div>
              <div className="summary-pill">
                <strong>验包</strong>
                <span>发布前检查</span>
              </div>
              <div className="summary-pill">
                <strong>发布</strong>
                <span>工作流出口</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div className="topbar-actions">
        <TopBarActionGroup label="历史">
          <button
            className="ghost-inline"
            disabled={history.past.length === 0}
            onClick={() => commandBus.execute({ type: 'history.undo' })}
            type="button"
          >
            撤销
          </button>
          <button
            className="ghost-inline"
            disabled={history.future.length === 0}
            onClick={() => commandBus.execute({ type: 'history.redo' })}
            type="button"
          >
            重做
          </button>
          <button className="solid-inline" onClick={handleSaveNow} type="button">
            保存
          </button>
        </TopBarActionGroup>

        <TopBarActionGroup label="文件">
          <button className="ghost-inline" onClick={() => inputRef.current?.click()} type="button">
            导入
          </button>
          <button className="ghost-inline" onClick={handleExport} type="button">
            导出
          </button>
        </TopBarActionGroup>

        <TopBarActionGroup label="流程">
          <button className="ghost-inline" onClick={() => navigate('/validate')} type="button">
            验包
          </button>
          <button className="ghost-inline" onClick={() => navigate('/publish')} type="button">
            发布
          </button>
        </TopBarActionGroup>

        <div className={`save-badge ${saveToneClass}`}>{formatSaveLabel(session.saveState, session.lastSavedAt)}</div>
      </div>

      <input ref={inputRef} accept="application/json" className="hidden-input" onChange={handleImport} type="file" />
    </header>
  )
}

import { useRef } from 'react'
import type { ChangeEvent, ReactNode } from 'react'
import { useNavigate } from 'react-router-dom'
import { commandBus } from '../../editor/core/commandBus'
import { cloneDocument } from '../../editor/core/clone'
import { saveStoredDocument } from '../../editor/core/storage'
import { ensureThemeDocument, getDocumentFrameCount } from '../../editor/core/themeDocument'
import { useEditorStore } from '../../editor/state/editorStore'
import { Icon } from './icons'

function formatSaveLabel(saveState: string, lastSavedAt: string): string {
  if (saveState === 'saving') {
    return '保存中'
  }
  if (saveState === 'saved') {
    return lastSavedAt.length > 0 ? `已保存 ${lastSavedAt}` : '已保存'
  }
  if (saveState === 'dirty') {
    return '待保存'
  }
  if (saveState === 'error') {
    return '保存失败'
  }
  return '未保存'
}

function getSaveStateClass(saveState: string): string {
  if (saveState === 'saving') {
    return 'mx-save-pill is-saving'
  }
  if (saveState === 'saved') {
    return 'mx-save-pill is-saved'
  }
  if (saveState === 'dirty') {
    return 'mx-save-pill is-dirty'
  }
  if (saveState === 'error') {
    return 'mx-save-pill is-error'
  }
  return 'mx-save-pill'
}

export function EditorShell({
  inspector,
  sidebar,
  stage,
  timeline,
}: {
  inspector: ReactNode
  sidebar: ReactNode
  stage: ReactNode
  timeline: ReactNode
}) {
  const navigate = useNavigate()
  const inputRef = useRef<HTMLInputElement | null>(null)
  const document = useEditorStore((state) => state.document)
  const session = useEditorStore((state) => state.session)
  const history = useEditorStore((state) => state.history)
  const notice = useEditorStore((state) => state.session.notice)
  const frameCount = getDocumentFrameCount(document)

  const emitNotice = (text: string, tone: 'info' | 'success' | 'warning' | 'error' = 'info') => {
    commandBus.execute({
      type: 'session.setNotice',
      notice: {
        text,
        tone,
        source: 'topbar',
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
    const blob = new Blob([JSON.stringify(document, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const anchor = window.document.createElement('a')
    anchor.href = url
    anchor.download = `${document.meta.name || 'glowxel-theme'}.json`
    anchor.click()
    URL.revokeObjectURL(url)
    emitNotice('已导出主题 JSON', 'success')
  }

  const handleSaveNow = async () => {
    try {
      useEditorStore.getState().updateSession({ saveState: 'saving' })
      await saveStoredDocument(cloneDocument(document))
      commandBus.execute({
        type: 'persistence.markSaved',
        at: new Date().toLocaleTimeString('zh-CN', { hour12: false }),
      })
      emitNotice('已手动保存', 'success')
    } catch {
      commandBus.execute({ type: 'persistence.markError' })
      emitNotice('保存失败，请检查本地存储', 'error')
    }
  }

  return (
    <div className="matrix-workbench-page">
      <div className="mx-shell">
        <header className="mx-topbar">
          <div className="mx-brand">
            <div className="mx-brand-mark">
              <Icon name="timeline" />
            </div>
            <div className="mx-brand-copy">
              <div className="mx-brand-title">{document.meta.name}</div>
              <div className="mx-brand-subtitle">
                {document.scenes.length} 场景 / {frameCount} 帧 / {document.meta.matrix.width}x
                {document.meta.matrix.height}
              </div>
            </div>
          </div>

          <div className="mx-topbar-center">
            <button
              className="mx-icon-button"
              disabled={history.past.length === 0}
              onClick={() => commandBus.execute({ type: 'history.undo' })}
              title="撤销"
              type="button"
            >
              <Icon name="undo" />
            </button>
            <button
              className="mx-icon-button"
              disabled={history.future.length === 0}
              onClick={() => commandBus.execute({ type: 'history.redo' })}
              title="重做"
              type="button"
            >
              <Icon name="redo" />
            </button>
            <button
              className={session.isPlaying ? 'mx-primary-button is-live' : 'mx-primary-button'}
              onClick={() => commandBus.execute({ type: 'session.togglePlay' })}
              type="button"
            >
              {session.isPlaying ? '暂停预览' : '播放预览'}
            </button>
          </div>

          <div className="mx-topbar-actions">
            <button className="mx-icon-button" onClick={() => inputRef.current?.click()} title="导入" type="button">
              <Icon name="add" />
            </button>
            <button className="mx-icon-button" onClick={handleExport} title="导出" type="button">
              <Icon name="export" />
            </button>
            <button className={getSaveStateClass(session.saveState)} onClick={handleSaveNow} type="button">
              {formatSaveLabel(session.saveState, session.lastSavedAt)}
            </button>
            <button className="mx-ghost-button" onClick={() => navigate('/validate')} type="button">
              验包
            </button>
            <button className="mx-primary-button" onClick={() => navigate('/publish')} type="button">
              发布
            </button>
          </div>

          <input
            ref={inputRef}
            accept="application/json"
            className="hidden-input"
            onChange={handleImport}
            type="file"
          />
        </header>

        <aside className="mx-slot mx-sidebar-slot">{sidebar}</aside>
        <main className="mx-slot mx-stage-slot">{stage}</main>
        <aside className="mx-slot mx-inspector-slot">{inspector}</aside>
        <section className="mx-slot mx-timeline-slot">{timeline}</section>

        {notice ? (
          <div className={`mx-notice is-${notice.tone}`} aria-live="polite" role="status">
            {notice.text}
          </div>
        ) : null}
      </div>
    </div>
  )
}

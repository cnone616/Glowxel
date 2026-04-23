import { useEffect } from 'react'
import { commandBus } from '../core/commandBus'
import { cloneDocument } from '../core/clone'
import { saveStoredDocument } from '../core/storage'
import { useEditorStore } from '../state/editorStore'

export function useAutosaveService(): void {
  const persistDirty = useEditorStore((state) => state.runtime.dirtyFlags.persistDirty)
  const dragState = useEditorStore((state) => state.session.dragState)

  useEffect(() => {
    if (!persistDirty || dragState) {
      return undefined
    }

    useEditorStore.getState().updateSession({ saveState: 'saving' })
    const timerId = window.setTimeout(async () => {
      try {
        const document = cloneDocument(useEditorStore.getState().document)
        await saveStoredDocument(document)
        commandBus.execute({
          type: 'persistence.markSaved',
          at: new Date().toLocaleTimeString('zh-CN', { hour12: false }),
        })
      } catch {
        commandBus.execute({ type: 'persistence.markError' })
        commandBus.execute({
          type: 'session.setNotice',
          notice: {
            text: '自动保存失败，请检查本地存储。',
            tone: 'error',
            source: 'system',
          },
        })
      }
    }, 1200)

    return () => window.clearTimeout(timerId)
  }, [dragState, persistDirty])
}

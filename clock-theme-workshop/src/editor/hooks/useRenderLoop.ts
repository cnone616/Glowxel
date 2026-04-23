import { useEffect } from 'react'
import { RenderEngine } from '../core/renderEngine'
import { useEditorStore } from '../state/editorStore'

const renderEngine = new RenderEngine()

export function useRenderLoop(): void {
  const renderDirty = useEditorStore((state) => state.runtime.dirtyFlags.renderDirty)
  const document = useEditorStore((state) => state.document)
  const currentFrame = useEditorStore((state) => state.session.currentFrame)

  useEffect(() => {
    if (!renderDirty) {
      return undefined
    }

    const frameId = window.requestAnimationFrame(() => {
      const start = performance.now()
      const snapshot = renderEngine.buildSnapshot(document, currentFrame)
      useEditorStore.getState().setSnapshot(snapshot, performance.now() - start)
    })

    return () => window.cancelAnimationFrame(frameId)
  }, [currentFrame, document, renderDirty])
}

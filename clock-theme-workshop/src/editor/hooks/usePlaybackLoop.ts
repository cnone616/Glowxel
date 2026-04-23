import { useEffect } from 'react'
import { useEditorStore } from '../state/editorStore'
import { commandBus } from '../core/commandBus'
import { getDocumentFrameCount } from '../core/themeDocument'

export function usePlaybackLoop(): void {
  const isPlaying = useEditorStore((state) => state.session.isPlaying)
  const frameDelay = useEditorStore((state) => state.document.meta.frameDelay)
  const frameCount = useEditorStore((state) => getDocumentFrameCount(state.document))

  useEffect(() => {
    if (!isPlaying) {
      return undefined
    }

    const timerId = window.setInterval(() => {
      const nextFrame = (useEditorStore.getState().session.currentFrame + 1) % Math.max(1, frameCount)
      commandBus.execute({ type: 'timeline.setPlayhead', frame: nextFrame })
    }, frameDelay)

    return () => window.clearInterval(timerId)
  }, [frameCount, frameDelay, isPlaying])
}

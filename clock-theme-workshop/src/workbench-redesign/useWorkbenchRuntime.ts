import { useEffect } from 'react'
import { loadCatalogSourceAsset } from '../editor/core/catalog'
import { commandBus } from '../editor/core/commandBus'
import { useEditorBootstrap } from '../editor/hooks/useEditorBootstrap'
import { useAutosaveService } from '../editor/hooks/useAutosaveService'
import { usePlaybackLoop } from '../editor/hooks/usePlaybackLoop'
import { useRenderLoop } from '../editor/hooks/useRenderLoop'
import { useEditorStore } from '../editor/state/editorStore'

function useCurrentSourceLoader() {
  const manifest = useEditorStore((state) => state.catalog.manifest)
  const selectedCollectionId = useEditorStore((state) => state.session.selectedCollectionId)
  const selectedSourceAssetId = useEditorStore((state) => state.session.selectedSourceAssetId)
  const cache = useEditorStore((state) => state.catalog.sourceCache)

  useEffect(() => {
    if (!manifest) {
      return
    }

    const collection = manifest.collections.find((entry) => entry.id === selectedCollectionId)
    if (!collection) {
      return
    }

    const currentSource = collection.sourceAssets.find((entry) => entry.id === selectedSourceAssetId)
    const fallbackSource = collection.sourceAssets[0]
    if (!currentSource && fallbackSource) {
      commandBus.execute({ type: 'session.setSourceAsset', sourceAssetId: fallbackSource.id })
      return
    }

    if (!currentSource) {
      return
    }

    const cacheKey = `${collection.id}:${currentSource.id}`
    if (cache[cacheKey]) {
      return
    }

    let cancelled = false

    const load = async () => {
      try {
        useEditorStore.getState().setCatalogState({ loading: true })
        const normalized = await loadCatalogSourceAsset(collection.id, currentSource)
        if (cancelled) {
          return
        }
        useEditorStore.getState().cacheCatalogSource(cacheKey, normalized)
        useEditorStore.getState().setCatalogState({ loading: false, error: '' })
      } catch (error) {
        if (cancelled) {
          return
        }
        useEditorStore.getState().setCatalogState({
          loading: false,
          error: error instanceof Error ? error.message : '素材源加载失败',
        })
      }
    }

    void load()

    return () => {
      cancelled = true
    }
  }, [cache, manifest, selectedCollectionId, selectedSourceAssetId])
}

function useWorkbenchHotkeys() {
  useEffect(() => {
    const isEditableTarget = (target: EventTarget | null): boolean => {
      if (!(target instanceof HTMLElement)) {
        return false
      }
      return target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.isContentEditable
    }

    const handleKeyDown = (event: KeyboardEvent) => {
      if (isEditableTarget(event.target)) {
        return
      }

      if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === 'z') {
        event.preventDefault()
        if (event.shiftKey) {
          commandBus.execute({ type: 'history.redo' })
        } else {
          commandBus.execute({ type: 'history.undo' })
        }
        return
      }

      if (event.key === 'Delete' || event.key === 'Backspace') {
        event.preventDefault()
        commandBus.execute({ type: 'document.deleteSelection' })
        return
      }

      if (event.key === ' ') {
        event.preventDefault()
        commandBus.execute({ type: 'session.togglePlay' })
      }
    }

    window.addEventListener('keydown', handleKeyDown)

    return () => {
      window.removeEventListener('keydown', handleKeyDown)
    }
  }, [])
}

function useNoticeLifecycle() {
  const notice = useEditorStore((state) => state.session.notice)

  useEffect(() => {
    if (!notice) {
      return
    }

    const timer = window.setTimeout(() => {
      commandBus.execute({ type: 'session.clearNotice' })
    }, 2800)

    return () => window.clearTimeout(timer)
  }, [notice])
}

export function useWorkbenchRuntime(): void {
  useEditorBootstrap()
  useCurrentSourceLoader()
  useAutosaveService()
  usePlaybackLoop()
  useRenderLoop()
  useWorkbenchHotkeys()
  useNoticeLifecycle()
}

import { useEffect } from 'react'
import { Group, Panel, Separator } from 'react-resizable-panels'
import { AssetBrowserPane } from './AssetBrowserPane'
import { EditorStatusBar } from './EditorStatusBar'
import { EditorTopBar } from './EditorTopBar'
import { InspectorDock } from './InspectorDock'
import { SceneOutlinePane } from './SceneOutlinePane'
import { TimelinePane } from './TimelinePane'
import { ToolModeBar } from './ToolModeBar'
import { UploadDialog } from './UploadDialog'
import { ViewportPane } from './ViewportPane'
import { useEditorBootstrap } from '../hooks/useEditorBootstrap'
import { useAutosaveService } from '../hooks/useAutosaveService'
import { usePlaybackLoop } from '../hooks/usePlaybackLoop'
import { useRenderLoop } from '../hooks/useRenderLoop'
import { useEditorStore } from '../state/editorStore'
import { loadCatalogSourceAsset } from '../core/catalog'
import { commandBus } from '../core/commandBus'

function useCurrentSourceLoader() {
  const manifest = useEditorStore((state) => state.catalog.manifest)
  const session = useEditorStore((state) => state.session)
  const cache = useEditorStore((state) => state.catalog.sourceCache)

  useEffect(() => {
    if (!manifest) {
      return
    }

    const collection = manifest.collections.find((entry) => entry.id === session.selectedCollectionId)
    if (!collection) {
      return
    }

    const currentSource = collection.sourceAssets.find((entry) => entry.id === session.selectedSourceAssetId)
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
  }, [cache, manifest, session.selectedCollectionId, session.selectedSourceAssetId])
}

function useEditorHotkeys() {
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

      const selection = useEditorStore.getState().session.selection
      const snapshot = useEditorStore.getState().snapshot
      const shiftStep = event.shiftKey ? 10 : 1

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

      if (event.key === '0') {
        commandBus.execute({ type: 'session.fitViewport' })
        return
      }

      if (event.key === '1' || event.key === '2' || event.key === '4') {
        commandBus.execute({ type: 'session.setZoomPreset', zoom: Number(event.key) })
        return
      }

      if (event.key === ' ') {
        event.preventDefault()
        useEditorStore.getState().updateSession({ temporaryPan: true })
        return
      }

      const horizontal = event.key === 'ArrowLeft' ? -shiftStep : event.key === 'ArrowRight' ? shiftStep : 0
      const vertical = event.key === 'ArrowUp' ? -shiftStep : event.key === 'ArrowDown' ? shiftStep : 0
      if (horizontal === 0 && vertical === 0) {
        return
      }

      event.preventDefault()

      if (selection.kind === 'background' && selection.ids[0]) {
        const target = snapshot.backgrounds.find((item) => item.id === selection.ids[0])
        if (target) {
          commandBus.execute({
            type: 'background.updateTransform',
            id: selection.ids[0],
            patch: {
              x: Math.round(target.transform.x + horizontal),
              y: Math.round(target.transform.y + vertical),
            },
          })
        }
      }

      if (selection.kind === 'object' && selection.ids[0]) {
        const target = snapshot.objects.find((item) => item.id === selection.ids[0])
        if (target) {
          commandBus.execute({
            type: 'object.updateTransform',
            id: selection.ids[0],
            patch: {
              x: Math.round(target.transform.x + horizontal),
              y: Math.round(target.transform.y + vertical),
            },
          })
        }
      }

      if (selection.kind === 'time' && selection.ids[0]) {
        const target = snapshot.widgets.find((item) => item.id === selection.ids[0])
        if (target) {
          commandBus.execute({
            type: 'time.update',
            id: selection.ids[0],
            patch: {
              x: target.x + horizontal,
              y: target.y + vertical,
            },
          })
        }
      }
    }

    const handleKeyUp = (event: KeyboardEvent) => {
      if (event.key === ' ') {
        useEditorStore.getState().updateSession({ temporaryPan: false })
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    window.addEventListener('keyup', handleKeyUp)

    return () => {
      window.removeEventListener('keydown', handleKeyDown)
      window.removeEventListener('keyup', handleKeyUp)
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

export function EditorWorkbench() {
  const loaded = useEditorStore((state) => state.runtime.loaded)

  useEditorBootstrap()
  useCurrentSourceLoader()
  useAutosaveService()
  usePlaybackLoop()
  useRenderLoop()
  useEditorHotkeys()
  useNoticeLifecycle()

  if (!loaded) {
    return (
      <div className="workbench-loading">
        <div className="loading-panel">
          <span className="eyebrow-label">Glowxel Workshop</span>
          <h1>正在准备新的专业工作台…</h1>
          <p>加载素材目录、恢复本地文档，并初始化编辑器渲染底座。</p>
        </div>
      </div>
    )
  }

  return (
    <div className="workbench-shell">
      <EditorTopBar />

      <div className="workbench-body">
        <Group id="clock-theme-workshop-shell" orientation="horizontal">
          <Panel defaultSize={24} minSize={19}>
            <Group id="clock-theme-workshop-left" orientation="vertical">
              <Panel defaultSize={54} minSize={30}>
                <AssetBrowserPane />
              </Panel>
              <Separator className="panel-resize-handle horizontal" />
              <Panel defaultSize={46} minSize={26}>
                <SceneOutlinePane />
              </Panel>
            </Group>
          </Panel>

          <Separator className="panel-resize-handle vertical" />

          <Panel defaultSize={50} minSize={34}>
            <Group id="clock-theme-workshop-center" orientation="vertical">
              <Panel defaultSize={74} minSize={48}>
                <section className="center-stage-panel">
                  <ToolModeBar />
                  <ViewportPane />
                </section>
              </Panel>
              <Separator className="panel-resize-handle horizontal" />
              <Panel collapsible defaultSize={26} minSize={18}>
                <TimelinePane />
              </Panel>
            </Group>
          </Panel>

          <Separator className="panel-resize-handle vertical" />

          <Panel defaultSize={26} minSize={20}>
            <InspectorDock />
          </Panel>
        </Group>
      </div>

      <EditorStatusBar />
      <UploadDialog />
    </div>
  )
}

import { create } from 'zustand'
import { subscribeWithSelector } from 'zustand/middleware'
import type {
  CatalogState,
  DragState,
  EditorRuntime,
  EditorSession,
  Point,
  RenderSnapshot,
  ThemeDocument,
} from '../types'
import { RenderEngine } from '../core/renderEngine'
import { createDefaultDocument, getInitialSceneId } from '../core/themeDocument'

export interface EditorHistoryState {
  past: ThemeDocument[]
  future: ThemeDocument[]
}

export interface EditorStoreState {
  document: ThemeDocument
  session: EditorSession
  runtime: EditorRuntime
  catalog: CatalogState
  history: EditorHistoryState
  snapshot: RenderSnapshot
  hydrateDocument: (document: ThemeDocument) => void
  updateSession: (patch: Partial<EditorSession>) => void
  setDragState: (dragState: DragState | null) => void
  setSnapshot: (snapshot: RenderSnapshot, durationMs: number) => void
  setCatalogState: (patch: Partial<CatalogState>) => void
  cacheCatalogSource: (key: string, source: CatalogState['sourceCache'][string]) => void
  replaceDocument: (document: ThemeDocument) => void
  setHistory: (history: EditorHistoryState) => void
  setLoaded: (loaded: boolean) => void
  markDirty: (patch: Partial<EditorRuntime['dirtyFlags']>) => void
}

function createDefaultSession(document: ThemeDocument): EditorSession {
  return {
    currentTool: 'select',
    currentPixelTool: 'brush',
    selection: {
      kind: 'scene',
      ids: [getInitialSceneId(document)],
    },
    currentFrame: 0,
    isPlaying: false,
    activeSceneId: getInitialSceneId(document),
    activeSidebarTab: 'materials',
    selectedMaterialId: '',
    viewportZoom: 1,
    viewportOffset: { x: 0, y: 0 },
    viewportMode: 'fit',
    saveState: 'idle',
    lastSavedAt: '',
    assetBrowserMode: 'entries',
    assetSearch: '',
    selectedCollectionId: 'adventure-island',
    selectedSourceAssetId: '',
    uploadDialogOpen: false,
    uploadChromaKey: true,
    hoverEntityId: '',
    performanceMs: 0,
    performanceFps: 0,
    dragState: null,
    temporaryPan: false,
    notice: null,
    drawColor: '#ffffff',
    pixelSelection: null,
  }
}

function createDefaultRuntime(): EditorRuntime {
  return {
    dirtyFlags: {
      documentDirty: false,
      renderDirty: true,
      selectionDirty: false,
      persistDirty: false,
    },
    stageTick: 0,
    loaded: false,
  }
}

function createDefaultCatalogState(): CatalogState {
  return {
    loading: true,
    error: '',
    manifest: null,
    sourceCache: {},
  }
}

const renderEngine = new RenderEngine()
const defaultDocument = createDefaultDocument()
const defaultSnapshot = renderEngine.buildSnapshot(defaultDocument, 0)

export const useEditorStore = create<EditorStoreState>()(
  subscribeWithSelector((set) => ({
    document: defaultDocument,
    session: createDefaultSession(defaultDocument),
    runtime: createDefaultRuntime(),
    catalog: createDefaultCatalogState(),
    history: {
      past: [],
      future: [],
    },
    snapshot: defaultSnapshot,
    hydrateDocument: (document) =>
      set((state) => ({
        document,
        session: {
          ...createDefaultSession(document),
          saveState: state.session.saveState,
          lastSavedAt: state.session.lastSavedAt,
          selectedCollectionId: state.session.selectedCollectionId,
          selectedSourceAssetId: state.session.selectedSourceAssetId,
          uploadChromaKey: state.session.uploadChromaKey,
        },
        snapshot: renderEngine.buildSnapshot(document, 0),
        runtime: {
          ...state.runtime,
          loaded: true,
          dirtyFlags: {
            ...state.runtime.dirtyFlags,
            renderDirty: false,
            documentDirty: false,
            persistDirty: false,
          },
        },
        history: {
          past: [],
          future: [],
        },
      })),
    updateSession: (patch) =>
      set((state) => ({
        session: {
          ...state.session,
          ...patch,
        },
      })),
    setDragState: (dragState) =>
      set((state) => ({
        session: {
          ...state.session,
          dragState,
        },
      })),
    setSnapshot: (snapshot, durationMs) =>
      set((state) => ({
        snapshot,
        session: {
          ...state.session,
          performanceMs: durationMs,
          performanceFps: durationMs > 0 ? Math.round(1000 / durationMs) : 0,
        },
        runtime: {
          ...state.runtime,
          stageTick: state.runtime.stageTick + 1,
          dirtyFlags: {
            ...state.runtime.dirtyFlags,
            renderDirty: false,
          },
        },
      })),
    setCatalogState: (patch) =>
      set((state) => ({
        catalog: {
          ...state.catalog,
          ...patch,
        },
      })),
    cacheCatalogSource: (key, source) =>
      set((state) => ({
        catalog: {
          ...state.catalog,
          sourceCache: {
            ...state.catalog.sourceCache,
            [key]: source,
          },
        },
      })),
    replaceDocument: (document) =>
      set((state) => ({
        document,
        runtime: {
          ...state.runtime,
          dirtyFlags: {
            ...state.runtime.dirtyFlags,
            renderDirty: true,
          },
        },
      })),
    setHistory: (history) => set({ history }),
    setLoaded: (loaded) =>
      set((state) => ({
        runtime: {
          ...state.runtime,
          loaded,
        },
      })),
    markDirty: (patch) =>
      set((state) => ({
        runtime: {
          ...state.runtime,
          dirtyFlags: {
            ...state.runtime.dirtyFlags,
            ...patch,
          },
        },
      })),
  })),
)

export function selectionMatches(selectionIds: string[], id: string): boolean {
  return selectionIds.includes(id)
}

export function setViewportOffset(offset: Point): void {
  useEditorStore.getState().updateSession({ viewportOffset: offset, viewportMode: 'manual' })
}

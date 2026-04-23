import { cloneDocument } from './clone'
import {
  DEFAULT_BACKGROUND_STATE,
  DEFAULT_GROUP_STATE,
  createBackgroundLayer,
  createObjectGroup,
  createPixelLayer,
  createScene,
  findSceneForSelection,
  getActiveScene,
  getDocumentFrameCount,
  getInitialSceneId,
  getSceneById,
  getSceneIndexById,
  normalizeDocument,
  removeKeyframe,
  resolveSceneFrame,
  sampleStepKeys,
  upsertKeyframe,
} from './themeDocument'
import type {
  BackgroundLayer,
  EditorCommand,
  LayerTiming,
  ObjectGroup,
  PixelLayer,
  Point,
  Scene,
  SelectionKind,
  ThemeDocument,
  TimeWidget,
} from '../types'
import { useEditorStore } from '../state/editorStore'
import { clamp } from './math'
import {
  buildLinePoints,
  buildRectOutlinePoints,
  cloneBitmap,
  erasePoints,
  floodFill,
  flipBitmap,
  moveSelection,
  paintPoints,
  readPixel,
  rgbaToHex,
} from './pixelBitmap'
import { uid } from './id'

const HISTORY_LIMIT = 80

function markDirtyDocument(): void {
  useEditorStore.getState().markDirty({
    documentDirty: true,
    renderDirty: true,
    persistDirty: true,
  })
  useEditorStore.getState().updateSession({ saveState: 'dirty' })
}

function pushHistory(previousDocument: ThemeDocument): void {
  const { history, setHistory } = useEditorStore.getState()
  const nextPast = history.past.concat(cloneDocument(previousDocument)).slice(-HISTORY_LIMIT)
  setHistory({
    past: nextPast,
    future: [],
  })
}

function getSceneStartFrame(document: ThemeDocument, sceneId: string): number {
  let frame = 0
  for (const scene of document.scenes) {
    if (scene.id === sceneId) {
      return frame
    }
    frame += scene.durationFrames
  }
  return 0
}

function getValidActiveSceneId(document: ThemeDocument, preferredSceneId: string): string {
  return getSceneById(document, preferredSceneId)?.id ?? getInitialSceneId(document)
}

function sanitizeSessionForDocument(document: ThemeDocument): void {
  const store = useEditorStore.getState()
  const activeSceneId = getValidActiveSceneId(document, store.session.activeSceneId)
  const documentFrameCount = getDocumentFrameCount(document)
  const currentFrame = clamp(store.session.currentFrame, 0, Math.max(0, documentFrameCount - 1))
  const selection = store.session.selection

  if (selection.kind === 'material') {
    const material = document.materials.find((item) => item.id === selection.ids[0])
    store.updateSession({
      activeSidebarTab: material ? 'materials' : 'scenes',
      activeSceneId,
      currentFrame,
      pixelSelection: null,
      selectedMaterialId: material?.id ?? '',
      selection: material
        ? selection
        : {
            kind: 'scene',
            ids: [activeSceneId],
          },
    })
    return
  }

  const selectedId = selection.ids[0]
  const selectedScene = selectedId ? findSceneForSelection(document, selection.kind, selectedId) : null
  store.updateSession({
    activeSidebarTab: 'scenes',
    activeSceneId: selectedScene?.id ?? activeSceneId,
    currentFrame,
    pixelSelection: null,
    selection: selectedScene
      ? selection
      : {
          kind: 'scene',
          ids: [activeSceneId],
        },
  })
}

function withDocumentMutation(mutator: (document: ThemeDocument) => void): void {
  const { document, replaceDocument } = useEditorStore.getState()
  const previousDocument = cloneDocument(document)
  const nextDocument = cloneDocument(document)
  mutator(nextDocument)
  const normalized = normalizeDocument(nextDocument)
  if (JSON.stringify(normalized) === JSON.stringify(previousDocument)) {
    return
  }
  normalized.meta.updatedAt = new Date().toISOString()
  pushHistory(previousDocument)
  replaceDocument(normalized)
  sanitizeSessionForDocument(normalized)
  markDirtyDocument()
}

function getSceneForEntity(
  document: ThemeDocument,
  kind: Exclude<SelectionKind, 'scene' | 'material'>,
  id: string,
): Scene | null {
  return findSceneForSelection(document, kind, id)
}

function setSceneSelection(sceneId: string): void {
  const document = useEditorStore.getState().document
  useEditorStore.getState().updateSession({
    activeSidebarTab: 'scenes',
    activeSceneId: sceneId,
    currentFrame: clamp(getSceneStartFrame(document, sceneId), 0, Math.max(0, getDocumentFrameCount(document) - 1)),
    pixelSelection: null,
    selection: {
      kind: 'scene',
      ids: [sceneId],
    },
    selectedMaterialId: '',
  })
  useEditorStore.getState().markDirty({ selectionDirty: true, renderDirty: true })
}

function setEntitySelection(
  sceneId: string,
  kind: Exclude<SelectionKind, 'scene' | 'material'>,
  id: string,
): void {
  useEditorStore.getState().updateSession({
    activeSidebarTab: 'scenes',
    activeSceneId: sceneId,
    pixelSelection: null,
    selection: {
      kind,
      ids: [id],
    },
    selectedMaterialId: '',
  })
  useEditorStore.getState().markDirty({ selectionDirty: true, renderDirty: true })
}

function applySelection(kind: SelectionKind, ids: string[], additive: boolean): void {
  const store = useEditorStore.getState()

  if (kind === 'scene' && ids[0]) {
    setSceneSelection(ids[0])
    return
  }

  if (kind === 'material') {
    store.updateSession({
      activeSidebarTab: 'materials',
      pixelSelection: null,
      selectedMaterialId: ids[0] ?? '',
      selection: {
        kind,
        ids,
      },
    })
    store.markDirty({ selectionDirty: true })
    return
  }

  const targetId = ids[0]
  const scene = targetId ? findSceneForSelection(store.document, kind, targetId) : null

  if (!additive || store.session.selection.kind !== kind) {
    store.updateSession({
      activeSidebarTab: 'scenes',
      activeSceneId: scene?.id ?? store.session.activeSceneId,
      pixelSelection: null,
      selection: {
        kind,
        ids,
      },
      selectedMaterialId: '',
    })
    store.markDirty({ selectionDirty: true })
    return
  }

  const nextIds = new Set(store.session.selection.ids)
  for (const id of ids) {
    if (nextIds.has(id)) {
      nextIds.delete(id)
    } else {
      nextIds.add(id)
    }
  }

  store.updateSession({
    activeSidebarTab: 'scenes',
    activeSceneId: scene?.id ?? store.session.activeSceneId,
    pixelSelection: null,
    selection: {
      kind,
      ids: Array.from(nextIds),
    },
    selectedMaterialId: '',
  })
  store.markDirty({ selectionDirty: true })
}

function getActiveSceneLocalFrame(document: ThemeDocument, activeSceneId: string, currentFrame: number): number {
  const resolved = resolveSceneFrame(document, currentFrame)
  if (resolved.scene.id === activeSceneId) {
    return resolved.localFrame
  }
  return 0
}

function getSelectionTarget<T extends { id: string }>(
  scene: Scene,
  kind: SelectionKind,
  expectedKind: SelectionKind,
  selectionIds: string[],
  getter: (scene: Scene) => T[],
): T | null {
  if (kind !== expectedKind || selectionIds.length === 0) {
    return null
  }
  return getter(scene).find((item) => item.id === selectionIds[0]) ?? null
}

function deleteFromScene(scene: Scene, kind: SelectionKind, ids: string[]): void {
  if (kind === 'background') {
    scene.backgroundLayers = scene.backgroundLayers.filter((item) => !ids.includes(item.id) || item.locked)
  }
  if (kind === 'object') {
    scene.objectGroups = scene.objectGroups.filter((item) => !ids.includes(item.id) || item.locked)
  }
  if (kind === 'pixel') {
    scene.pixelLayers = scene.pixelLayers.filter((item) => !ids.includes(item.id) || item.locked)
  }
  if (kind === 'time') {
    scene.timeWidgets = scene.timeWidgets.filter((item) => !ids.includes(item.id) || item.locked)
  }
}

function toggleVisibility(
  item: BackgroundLayer | ObjectGroup | PixelLayer | TimeWidget,
  currentFrame: number,
): void {
  if ('bitmap' in item) {
    item.visible = !item.visible
    return
  }

  if ('stateKeys' in item) {
    if ('motion' in item) {
      const visible = !sampleStepKeys(item.stateKeys, currentFrame, DEFAULT_GROUP_STATE).visible
      item.stateKeys = upsertKeyframe(item.stateKeys, currentFrame, { visible })
    } else {
      const visible = !sampleStepKeys(item.stateKeys, currentFrame, DEFAULT_BACKGROUND_STATE).visible
      item.stateKeys = upsertKeyframe(item.stateKeys, currentFrame, { visible })
    }
    return
  }

  item.visible = !item.visible
}

function toggleLock(item: BackgroundLayer | ObjectGroup | PixelLayer | TimeWidget): void {
  item.locked = !item.locked
}

function reorderItem<T extends { id: string }>(list: T[], id: string, direction: 'up' | 'down'): T[] {
  const currentIndex = list.findIndex((item) => item.id === id)
  if (currentIndex < 0) {
    return list
  }

  const nextIndex = direction === 'up' ? currentIndex - 1 : currentIndex + 1
  if (nextIndex < 0 || nextIndex >= list.length) {
    return list
  }

  const next = list.slice()
  const [removed] = next.splice(currentIndex, 1)
  next.splice(nextIndex, 0, removed)
  return next
}

function seedCurrentKeyframe(document: ThemeDocument): void {
  const { currentFrame, selection, activeSceneId } = useEditorStore.getState().session
  const scene = getSceneById(document, activeSceneId)
  if (!scene) {
    return
  }
  const localFrame = getActiveSceneLocalFrame(document, activeSceneId, currentFrame)
  const selectedBackground = getSelectionTarget(
    scene,
    selection.kind,
    'background',
    selection.ids,
    (value) => value.backgroundLayers,
  )
  if (selectedBackground) {
    const lastTransform = selectedBackground.transformKeys[selectedBackground.transformKeys.length - 1]
    const lastState = selectedBackground.stateKeys[selectedBackground.stateKeys.length - 1]
    selectedBackground.transformKeys = upsertKeyframe(selectedBackground.transformKeys, localFrame, lastTransform)
    selectedBackground.stateKeys = upsertKeyframe(selectedBackground.stateKeys, localFrame, lastState)
    return
  }

  const selectedObject = getSelectionTarget(
    scene,
    selection.kind,
    'object',
    selection.ids,
    (value) => value.objectGroups,
  )
  if (selectedObject) {
    const lastTransform = selectedObject.transformKeys[selectedObject.transformKeys.length - 1]
    const lastState = selectedObject.stateKeys[selectedObject.stateKeys.length - 1]
    selectedObject.transformKeys = upsertKeyframe(selectedObject.transformKeys, localFrame, lastTransform)
    selectedObject.stateKeys = upsertKeyframe(selectedObject.stateKeys, localFrame, lastState)
  }
}

function clearCurrentKeyframe(document: ThemeDocument): void {
  const { currentFrame, selection, activeSceneId } = useEditorStore.getState().session
  const scene = getSceneById(document, activeSceneId)
  if (!scene) {
    return
  }
  const localFrame = getActiveSceneLocalFrame(document, activeSceneId, currentFrame)

  const selectedBackground = getSelectionTarget(
    scene,
    selection.kind,
    'background',
    selection.ids,
    (value) => value.backgroundLayers,
  )
  if (selectedBackground) {
    selectedBackground.transformKeys = removeKeyframe(selectedBackground.transformKeys, localFrame)
    selectedBackground.stateKeys = removeKeyframe(selectedBackground.stateKeys, localFrame)
    return
  }

  const selectedObject = getSelectionTarget(
    scene,
    selection.kind,
    'object',
    selection.ids,
    (value) => value.objectGroups,
  )
  if (selectedObject) {
    selectedObject.transformKeys = removeKeyframe(selectedObject.transformKeys, localFrame)
    selectedObject.stateKeys = removeKeyframe(selectedObject.stateKeys, localFrame)
  }
}

function copyPreviousKeyframe(document: ThemeDocument): void {
  const { currentFrame, selection, activeSceneId } = useEditorStore.getState().session
  const scene = getSceneById(document, activeSceneId)
  if (!scene) {
    return
  }
  const localFrame = getActiveSceneLocalFrame(document, activeSceneId, currentFrame)
  const previousFrame = Math.max(0, localFrame - 1)

  const selectedBackground = getSelectionTarget(
    scene,
    selection.kind,
    'background',
    selection.ids,
    (value) => value.backgroundLayers,
  )
  if (selectedBackground) {
    const previousTransform = selectedBackground.transformKeys
      .slice()
      .reverse()
      .find((key) => key.frame <= previousFrame)
    const previousState = selectedBackground.stateKeys
      .slice()
      .reverse()
      .find((key) => key.frame <= previousFrame)

    if (previousTransform) {
      selectedBackground.transformKeys = upsertKeyframe(
        selectedBackground.transformKeys,
        localFrame,
        previousTransform,
      )
    }
    if (previousState) {
      selectedBackground.stateKeys = upsertKeyframe(selectedBackground.stateKeys, localFrame, previousState)
    }
    return
  }

  const selectedObject = getSelectionTarget(
    scene,
    selection.kind,
    'object',
    selection.ids,
    (value) => value.objectGroups,
  )
  if (selectedObject) {
    const previousTransform = selectedObject.transformKeys
      .slice()
      .reverse()
      .find((key) => key.frame <= previousFrame)
    const previousState = selectedObject.stateKeys
      .slice()
      .reverse()
      .find((key) => key.frame <= previousFrame)

    if (previousTransform) {
      selectedObject.transformKeys = upsertKeyframe(selectedObject.transformKeys, localFrame, previousTransform)
    }
    if (previousState) {
      selectedObject.stateKeys = upsertKeyframe(selectedObject.stateKeys, localFrame, previousState)
    }
  }
}

function ensureSelectionAfterDelete(sceneId: string): void {
  const store = useEditorStore.getState()
  const scene = getSceneById(store.document, sceneId)
  if (!scene) {
    setSceneSelection(getInitialSceneId(store.document))
    return
  }

  if (scene.pixelLayers.length > 0) {
    store.updateSession({
      activeSceneId: scene.id,
      selection: {
        kind: 'pixel',
        ids: [scene.pixelLayers[0].id],
      },
    })
    return
  }
  if (scene.objectGroups.length > 0) {
    store.updateSession({
      activeSceneId: scene.id,
      selection: {
        kind: 'object',
        ids: [scene.objectGroups[0].id],
      },
    })
    return
  }
  if (scene.backgroundLayers.length > 0) {
    store.updateSession({
      activeSceneId: scene.id,
      selection: {
        kind: 'background',
        ids: [scene.backgroundLayers[0].id],
      },
    })
    return
  }
  if (scene.timeWidgets.length > 0) {
    store.updateSession({
      activeSceneId: scene.id,
      selection: {
        kind: 'time',
        ids: [scene.timeWidgets[0].id],
      },
    })
    return
  }

  store.updateSession({
    activeSceneId: scene.id,
    selection: {
      kind: 'scene',
      ids: [scene.id],
    },
  })
}

function duplicateScene(sourceScene: Scene): Scene {
  return {
    ...cloneDocument({
      version: 'ThemeWorkshopDocumentV3',
      meta: {
        name: '',
        matrix: { width: 64, height: 64 },
        frameDelay: 140,
        previewMode: 'loop',
        previewDate: '2026-04-22',
        previewTime: '12:34:56',
        updatedAt: '',
      },
      scenes: [sourceScene],
      materials: [],
      themeRules: {
        paletteLock: false,
        matrixBound: { width: 64, height: 64 },
        pixelGridSize: 1,
      },
    }).scenes[0],
    id: uid('scene'),
    name: `${sourceScene.name} Copy`,
    backgroundLayers: sourceScene.backgroundLayers.map((layer) => ({
      ...layer,
      id: uid('background'),
    })),
    objectGroups: sourceScene.objectGroups.map((group) => ({
      ...group,
      id: uid('object'),
      members: group.members.map((member) => ({
        ...member,
        id: uid('member'),
      })),
      motion: {
        ...group.motion,
        points: group.motion.points.map((point) => ({
          ...point,
          id: uid('path'),
        })),
      },
    })),
    pixelLayers: sourceScene.pixelLayers.map((layer, index) => ({
      ...layer,
      id: uid('pixel'),
      name: `${layer.name} Copy`,
      zIndex: index,
      bitmap: cloneBitmap(layer.bitmap),
    })),
    timeWidgets: sourceScene.timeWidgets.map((widget) => ({
      ...widget,
      id: uid('time'),
    })),
  }
}

function clampPlayheadForDocument(frame: number, document: ThemeDocument): number {
  return clamp(frame, 0, Math.max(0, getDocumentFrameCount(document) - 1))
}

function updateMaterialReferences(document: ThemeDocument, materialId: string): void {
  for (const scene of document.scenes) {
    for (const background of scene.backgroundLayers) {
      if (background.assetId === materialId) {
        background.assetId = ''
      }
    }

    for (const objectGroup of scene.objectGroups) {
      objectGroup.members = objectGroup.members.filter((member) => member.assetId !== materialId)
    }
    scene.objectGroups = scene.objectGroups.filter((group) => group.members.length > 0)
  }
}

function findPixelLayer(document: ThemeDocument, id: string): { scene: Scene; layer: PixelLayer } | null {
  for (const scene of document.scenes) {
    const layer = scene.pixelLayers.find((item) => item.id === id)
    if (layer) {
      return { scene, layer }
    }
  }
  return null
}

function setLayerTiming<T extends { id: string; timing: LayerTiming }>(
  items: T[],
  id: string,
  timing: LayerTiming,
  durationFrames: number,
): void {
  const item = items.find((entry) => entry.id === id)
  if (item && !('locked' in item && item.locked)) {
    const maxFrame = Math.max(0, durationFrames - 1)
    const startFrame = clamp(Math.round(timing.startFrame), 0, maxFrame)
    const endFrame = clamp(Math.round(timing.endFrame), startFrame, maxFrame)
    item.timing = {
      startFrame,
      endFrame,
    }
  }
}

function buildStrokePoints(start: Point, points: Point[]): Point[] {
  if (points.length === 0) {
    return [start]
  }

  const stroke: Point[] = []
  let previous = start
  for (const point of points) {
    stroke.push(...buildLinePoints(previous, point))
    previous = point
  }
  return stroke
}

export class CommandBus {
  execute(command: EditorCommand): void {
    const store = useEditorStore.getState()

    switch (command.type) {
      case 'selection.set':
        applySelection(command.kind, command.ids, command.additive)
        return
      case 'selection.clear':
        setSceneSelection(store.session.activeSceneId)
        return
      case 'session.setTool':
        store.updateSession({ currentTool: command.tool })
        return
      case 'session.setPixelTool':
        store.updateSession({ currentPixelTool: command.tool })
        return
      case 'session.setSidebarTab':
        store.updateSession({ activeSidebarTab: command.tab })
        return
      case 'session.setDrawColor':
        store.updateSession({ drawColor: command.color })
        return
      case 'session.setPixelSelection':
        store.updateSession({ pixelSelection: command.selection })
        return
      case 'session.togglePlay':
        store.updateSession({ isPlaying: !store.session.isPlaying })
        return
      case 'session.setPreviewMode':
        withDocumentMutation((document) => {
          document.meta.previewMode = command.previewMode
        })
        return
      case 'session.setAssetMode':
        store.updateSession({ assetBrowserMode: command.mode })
        return
      case 'session.setAssetSearch':
        store.updateSession({ assetSearch: command.value })
        return
      case 'session.setCollection':
        store.updateSession({ selectedCollectionId: command.collectionId })
        return
      case 'session.setSourceAsset':
        store.updateSession({ selectedSourceAssetId: command.sourceAssetId })
        return
      case 'session.setZoomPreset':
        store.updateSession({ viewportZoom: command.zoom, viewportMode: 'manual' })
        return
      case 'session.fitViewport':
        store.updateSession({ viewportZoom: 1, viewportOffset: { x: 0, y: 0 }, viewportMode: 'fit' })
        return
      case 'session.panViewport':
        store.updateSession({ viewportOffset: command.offset, viewportMode: 'manual' })
        return
      case 'session.setUploadDialogOpen':
        store.updateSession({ uploadDialogOpen: command.open })
        return
      case 'session.setUploadChromaKey':
        store.updateSession({ uploadChromaKey: command.value })
        return
      case 'session.setHoverEntity':
        store.updateSession({ hoverEntityId: command.entityId })
        return
      case 'session.setNotice':
        store.updateSession({ notice: command.notice })
        return
      case 'session.clearNotice':
        store.updateSession({ notice: null })
        return
      case 'document.rename':
        withDocumentMutation((document) => {
          document.meta.name = command.name
        })
        return
      case 'document.import':
        store.hydrateDocument(command.document)
        store.updateSession({
          saveState: 'saved',
          lastSavedAt: new Date().toLocaleTimeString('zh-CN', { hour12: false }),
        })
        return
      case 'document.setFrameDelay':
        withDocumentMutation((document) => {
          document.meta.frameDelay = Math.max(40, Math.round(command.value))
        })
        return
      case 'document.setPreviewDate':
        withDocumentMutation((document) => {
          document.meta.previewDate = command.value
        })
        return
      case 'document.setPreviewTime':
        withDocumentMutation((document) => {
          document.meta.previewTime = command.value
        })
        return
      case 'themeRule.update':
        withDocumentMutation((document) => {
          document.themeRules = {
            ...document.themeRules,
            ...command.patch,
          }
        })
        return
      case 'scene.add':
        {
          let nextSceneId = ''
        withDocumentMutation((document) => {
          const sceneIndex = getSceneIndexById(document, store.session.activeSceneId)
          const nextScene = createScene(`Scene ${document.scenes.length + 1}`)
          document.scenes.splice(sceneIndex + 1, 0, nextScene)
            nextSceneId = nextScene.id
        })
          if (nextSceneId.length > 0) {
            setSceneSelection(nextSceneId)
          }
        return
        }
      case 'scene.remove':
        if (store.document.scenes.length <= 1) {
          return
        }
        {
          let nextSceneId = ''
        withDocumentMutation((document) => {
            const sceneIndex = getSceneIndexById(document, command.id)
            if (sceneIndex < 0) {
              return
            }
            document.scenes = document.scenes.filter((scene) => scene.id !== command.id)
            if (document.scenes.length > 0) {
              const nextIndex = Math.min(sceneIndex, document.scenes.length - 1)
              nextSceneId = document.scenes[nextIndex].id
            }
        })
          if (nextSceneId.length > 0) {
            setSceneSelection(nextSceneId)
          }
        return
        }
      case 'scene.duplicate':
        {
          let duplicateId = ''
        withDocumentMutation((document) => {
          const sceneIndex = getSceneIndexById(document, command.id)
          const sourceScene = document.scenes[sceneIndex]
          if (!sourceScene) {
            return
          }
            const nextScene = duplicateScene(sourceScene)
            duplicateId = nextScene.id
          document.scenes.splice(sceneIndex + 1, 0, nextScene)
        })
          if (duplicateId.length > 0) {
            setSceneSelection(duplicateId)
          }
        return
        }
      case 'scene.reorder':
        withDocumentMutation((document) => {
          const sceneIndex = getSceneIndexById(document, command.id)
          if (sceneIndex < 0 || command.toIndex < 0 || command.toIndex >= document.scenes.length) {
            return
          }
          const [scene] = document.scenes.splice(sceneIndex, 1)
          document.scenes.splice(command.toIndex, 0, scene)
        })
        setSceneSelection(command.id)
        return
      case 'scene.select':
        setSceneSelection(command.id)
        return
      case 'scene.trim':
        withDocumentMutation((document) => {
          const scene = getSceneById(document, command.id)
          if (scene) {
            scene.durationFrames = Math.max(8, Math.round(command.durationFrames))
          }
        })
        setSceneSelection(command.id)
        return
      case 'scene.update':
        withDocumentMutation((document) => {
          const scene = getSceneById(document, command.id)
          if (scene) {
            Object.assign(scene, command.patch)
          }
        })
        return
      case 'material.addCatalog':
      case 'material.addUploaded':
        withDocumentMutation((document) => {
          document.materials.push(command.asset)
        })
        useEditorStore.getState().updateSession({
          activeSidebarTab: 'materials',
          selectedMaterialId: command.asset.id,
          selection: {
            kind: 'material',
            ids: [command.asset.id],
          },
          uploadDialogOpen: false,
        })
        return
      case 'material.remove':
        withDocumentMutation((document) => {
          document.materials = document.materials.filter((asset) => asset.id !== command.id)
          updateMaterialReferences(document, command.id)
        })
        useEditorStore.getState().updateSession({
          selectedMaterialId: '',
          selection: {
            kind: 'scene',
            ids: [useEditorStore.getState().session.activeSceneId],
          },
        })
        return
      case 'material.select':
        useEditorStore.getState().updateSession({
          activeSidebarTab: 'materials',
          selectedMaterialId: command.id,
          selection: {
            kind: 'material',
            ids: [command.id],
          },
        })
        return
      case 'scene.insertMaterialAsBackground':
        {
          let nextBackgroundId = ''
        withDocumentMutation((document) => {
          const scene = getSceneById(document, command.sceneId)
          const material = document.materials.find((item) => item.id === command.materialId)
          if (!scene || !material) {
            return
          }
            const nextLayer = createBackgroundLayer(material)
            nextBackgroundId = nextLayer.id
          scene.backgroundLayers.push(nextLayer)
        })
          if (nextBackgroundId.length > 0) {
            setEntitySelection(command.sceneId, 'background', nextBackgroundId)
          }
        return
        }
      case 'scene.insertMaterialAsObject':
        {
          let nextObjectId = ''
        withDocumentMutation((document) => {
          const scene = getSceneById(document, command.sceneId)
          const material = document.materials.find((item) => item.id === command.materialId)
          if (!scene || !material) {
            return
          }
            const nextGroup = createObjectGroup(material, scene.durationFrames)
            nextObjectId = nextGroup.id
          scene.objectGroups.push(nextGroup)
        })
          if (nextObjectId.length > 0) {
            setEntitySelection(command.sceneId, 'object', nextObjectId)
          }
        return
        }
      case 'outline.rename':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, command.kind, command.id)
          if (!scene) {
            return
          }
          if (command.kind === 'background') {
            const item = scene.backgroundLayers.find((entry) => entry.id === command.id)
            if (item) item.name = command.name
          }
          if (command.kind === 'object') {
            const item = scene.objectGroups.find((entry) => entry.id === command.id)
            if (item) item.name = command.name
          }
          if (command.kind === 'pixel') {
            const item = scene.pixelLayers.find((entry) => entry.id === command.id)
            if (item) item.name = command.name
          }
          if (command.kind === 'time') {
            const item = scene.timeWidgets.find((entry) => entry.id === command.id)
            if (item) item.label = command.name
          }
        })
        return
      case 'outline.toggleVisibility':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, command.kind, command.id)
          if (!scene) {
            return
          }
          const localFrame = getActiveSceneLocalFrame(document, scene.id, useEditorStore.getState().session.currentFrame)
          if (command.kind === 'background') {
            const item = scene.backgroundLayers.find((entry) => entry.id === command.id)
            if (item) toggleVisibility(item, localFrame)
          }
          if (command.kind === 'object') {
            const item = scene.objectGroups.find((entry) => entry.id === command.id)
            if (item) toggleVisibility(item, localFrame)
          }
          if (command.kind === 'pixel') {
            const item = scene.pixelLayers.find((entry) => entry.id === command.id)
            if (item) toggleVisibility(item, localFrame)
          }
          if (command.kind === 'time') {
            const item = scene.timeWidgets.find((entry) => entry.id === command.id)
            if (item) toggleVisibility(item, localFrame)
          }
        })
        return
      case 'outline.toggleLock':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, command.kind, command.id)
          if (!scene) {
            return
          }
          if (command.kind === 'background') {
            const item = scene.backgroundLayers.find((entry) => entry.id === command.id)
            if (item) toggleLock(item)
          }
          if (command.kind === 'object') {
            const item = scene.objectGroups.find((entry) => entry.id === command.id)
            if (item) toggleLock(item)
          }
          if (command.kind === 'pixel') {
            const item = scene.pixelLayers.find((entry) => entry.id === command.id)
            if (item) toggleLock(item)
          }
          if (command.kind === 'time') {
            const item = scene.timeWidgets.find((entry) => entry.id === command.id)
            if (item) toggleLock(item)
          }
        })
        return
      case 'outline.reorder':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, command.kind, command.id)
          if (!scene) {
            return
          }
          if (command.kind === 'background') {
            scene.backgroundLayers = reorderItem(scene.backgroundLayers, command.id, command.direction)
          }
          if (command.kind === 'object') {
            scene.objectGroups = reorderItem(scene.objectGroups, command.id, command.direction)
          }
          if (command.kind === 'pixel') {
            scene.pixelLayers = reorderItem(scene.pixelLayers, command.id, command.direction).map((layer, index) => ({
              ...layer,
              zIndex: index,
            }))
          }
          if (command.kind === 'time') {
            scene.timeWidgets = reorderItem(scene.timeWidgets, command.id, command.direction)
          }
        })
        return
      case 'background.updateTransform':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'background', command.id)
          if (!scene) {
            return
          }
          const localFrame = getActiveSceneLocalFrame(document, scene.id, useEditorStore.getState().session.currentFrame)
          const item = scene.backgroundLayers.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.transformKeys = upsertKeyframe(item.transformKeys, localFrame, {
              ...command.patch,
              ...(command.patch.width === undefined
                ? {}
                : { width: Math.max(1, Math.round(command.patch.width)) }),
              ...(command.patch.height === undefined
                ? {}
                : { height: Math.max(1, Math.round(command.patch.height)) }),
              ...(command.patch.opacity === undefined
                ? {}
                : { opacity: clamp(command.patch.opacity, 0, 1) }),
            })
          }
        })
        return
      case 'background.updateState':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'background', command.id)
          if (!scene) {
            return
          }
          const localFrame = getActiveSceneLocalFrame(document, scene.id, useEditorStore.getState().session.currentFrame)
          const item = scene.backgroundLayers.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.stateKeys = upsertKeyframe(item.stateKeys, localFrame, {
              ...command.patch,
              ...(command.patch.rhythmFrames === undefined
                ? {}
                : { rhythmFrames: Math.max(1, Math.round(command.patch.rhythmFrames)) }),
              ...(command.patch.sequenceStepFrames === undefined
                ? {}
                : { sequenceStepFrames: Math.max(1, Math.round(command.patch.sequenceStepFrames)) }),
            })
          }
        })
        return
      case 'background.updateBase':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'background', command.id)
          const item = scene?.backgroundLayers.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            Object.assign(item, command.patch)
          }
        })
        return
      case 'object.updateTransform':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          if (!scene) {
            return
          }
          const localFrame = getActiveSceneLocalFrame(document, scene.id, useEditorStore.getState().session.currentFrame)
          const item = scene.objectGroups.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.transformKeys = upsertKeyframe(item.transformKeys, localFrame, {
              ...command.patch,
              ...(command.patch.scale === undefined ? {} : { scale: Math.max(0.2, command.patch.scale) }),
              ...(command.patch.opacity === undefined
                ? {}
                : { opacity: clamp(command.patch.opacity, 0, 1) }),
            })
          }
        })
        return
      case 'object.updateState':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          if (!scene) {
            return
          }
          const localFrame = getActiveSceneLocalFrame(document, scene.id, useEditorStore.getState().session.currentFrame)
          const item = scene.objectGroups.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.stateKeys = upsertKeyframe(item.stateKeys, localFrame, {
              ...command.patch,
              ...(command.patch.rhythmFrames === undefined
                ? {}
                : { rhythmFrames: Math.max(1, Math.round(command.patch.rhythmFrames)) }),
              ...(command.patch.sequenceStepFrames === undefined
                ? {}
                : { sequenceStepFrames: Math.max(1, Math.round(command.patch.sequenceStepFrames)) }),
            })
          }
        })
        return
      case 'object.updateMotion':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          const item = scene?.objectGroups.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.motion = {
              ...item.motion,
              ...command.patch,
              ...(command.patch.durationFrames === undefined
                ? {}
                : { durationFrames: Math.max(1, Math.round(command.patch.durationFrames)) }),
            }
          }
        })
        return
      case 'object.addPathPoint':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          if (!scene) {
            return
          }
          const item = scene?.objectGroups.find((entry) => entry.id === command.id)
          if (!item || item.locked) {
            return
          }
          const currentFrame = getActiveSceneLocalFrame(document, scene.id, store.session.currentFrame)
          item.motion.points.push({
            id: uid('path'),
            x: command.point.x,
            y: command.point.y,
          })
          item.stateKeys = upsertKeyframe(item.stateKeys, currentFrame, { behavior: 'path' })
        })
        return
      case 'object.updatePathPoint':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          const item = scene?.objectGroups.find((entry) => entry.id === command.id)
          const point = item?.motion.points.find((entry) => entry.id === command.pointId)
          if (point && item && !item.locked) {
            point.x = command.patch.x
            point.y = command.patch.y
          }
        })
        return
      case 'object.removePathPoint':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          const item = scene?.objectGroups.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.motion.points = item.motion.points.filter((point) => point.id !== command.pointId)
          }
        })
        return
      case 'object.clearPath':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'object', command.id)
          const item = scene?.objectGroups.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            item.motion.points = []
          }
        })
        return
      case 'time.update':
        withDocumentMutation((document) => {
          const scene = getSceneForEntity(document, 'time', command.id)
          const item = scene?.timeWidgets.find((entry) => entry.id === command.id)
          if (item && !item.locked) {
            Object.assign(item, {
              ...command.patch,
              ...(command.patch.scale === undefined
                ? {}
                : { scale: Math.max(1, Math.round(command.patch.scale)) }),
              ...(command.patch.letterSpacing === undefined
                ? {}
                : { letterSpacing: Math.max(0, Math.round(command.patch.letterSpacing)) }),
            })
          }
        })
        return
      case 'pixelLayer.create':
        {
          let nextPixelLayerId = ''
        withDocumentMutation((document) => {
          const scene = getSceneById(document, command.sceneId)
          if (!scene) {
            return
          }
          const nextLayer = createPixelLayer(scene.durationFrames)
          nextLayer.zIndex = scene.pixelLayers.length
          nextLayer.name = `Pixel Layer ${scene.pixelLayers.length + 1}`
            nextPixelLayerId = nextLayer.id
          scene.pixelLayers.push(nextLayer)
        })
          if (nextPixelLayerId.length > 0) {
            setEntitySelection(command.sceneId, 'pixel', nextPixelLayerId)
          }
        return
        }
      case 'pixelLayer.delete':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found) {
            return
          }
          found.scene.pixelLayers = found.scene.pixelLayers.filter((layer) => layer.id !== command.id)
        })
        ensureSelectionAfterDelete(useEditorStore.getState().session.activeSceneId)
        return
      case 'pixelLayer.duplicate':
        {
          let duplicatePixelLayerId = ''
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found) {
            return
          }
          const index = found.scene.pixelLayers.findIndex((layer) => layer.id === command.id)
          const duplicate: PixelLayer = {
            ...found.layer,
            id: uid('pixel'),
            name: `${found.layer.name} Copy`,
            bitmap: cloneBitmap(found.layer.bitmap),
          }
            duplicatePixelLayerId = duplicate.id
          found.scene.pixelLayers.splice(index + 1, 0, duplicate)
          found.scene.pixelLayers = found.scene.pixelLayers.map((layer, layerIndex) => ({
            ...layer,
            zIndex: layerIndex,
          }))
        })
          const sceneId = useEditorStore.getState().session.activeSceneId
          if (duplicatePixelLayerId.length > 0) {
            setEntitySelection(sceneId, 'pixel', duplicatePixelLayerId)
          }
        return
        }
      case 'pixelLayer.reorder':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found) {
            return
          }
          found.scene.pixelLayers = reorderItem(
            found.scene.pixelLayers,
            command.id,
            command.direction,
          ).map((layer, index) => ({
            ...layer,
            zIndex: index,
          }))
        })
        return
      case 'pixelLayer.select':
        applySelection('pixel', [command.id], false)
        return
      case 'pixelLayer.update':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (found && !found.layer.locked) {
            Object.assign(found.layer, {
              ...command.patch,
              ...(command.patch.x === undefined ? {} : { x: Math.round(command.patch.x) }),
              ...(command.patch.y === undefined ? {} : { y: Math.round(command.patch.y) }),
              ...(command.patch.opacity === undefined
                ? {}
                : { opacity: clamp(command.patch.opacity, 0, 1) }),
              ...(command.patch.zIndex === undefined
                ? {}
                : { zIndex: Math.max(0, Math.round(command.patch.zIndex)) }),
            })
          }
        })
        return
      case 'pixelLayer.paint':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found || found.layer.locked) {
            return
          }
          paintPoints(found.layer.bitmap, command.points, command.color)
        })
        return
      case 'pixelLayer.erase':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found || found.layer.locked) {
            return
          }
          erasePoints(found.layer.bitmap, command.points)
        })
        return
      case 'pixelLayer.fill':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found || found.layer.locked) {
            return
          }
          floodFill(found.layer.bitmap, command.point, command.color)
        })
        return
      case 'pixelLayer.eyedropper':
        {
          const found = findPixelLayer(store.document, command.id)
          if (!found) {
            return
          }
          const color = rgbaToHex(readPixel(found.layer.bitmap, command.point.x, command.point.y))
          useEditorStore.getState().updateSession({ drawColor: color })
        }
        return
      case 'pixelLayer.transformSelection':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found || found.layer.locked) {
            return
          }
          found.layer.bitmap = moveSelection(found.layer.bitmap, command.selection, command.delta)
        })
        return
      case 'pixelLayer.flip':
        withDocumentMutation((document) => {
          const found = findPixelLayer(document, command.id)
          if (!found || found.layer.locked) {
            return
          }
          found.layer.bitmap = flipBitmap(found.layer.bitmap, command.axis)
        })
        return
      case 'layer.setTiming':
        withDocumentMutation((document) => {
          const scene = getActiveScene(document, useEditorStore.getState().session.activeSceneId)
          if (command.kind === 'pixel') {
            setLayerTiming(scene.pixelLayers, command.id, command.timing, scene.durationFrames)
          }
          if (command.kind === 'object') {
            setLayerTiming(scene.objectGroups, command.id, command.timing, scene.durationFrames)
          }
          if (command.kind === 'time') {
            setLayerTiming(scene.timeWidgets, command.id, command.timing, scene.durationFrames)
          }
        })
        return
      case 'timeline.setPlayhead':
        store.updateSession({
          currentFrame: clampPlayheadForDocument(command.frame, store.document),
        })
        store.markDirty({ renderDirty: true })
        return
      case 'document.deleteSelection':
        if (store.session.selection.kind === 'scene' && store.session.selection.ids[0]) {
          this.execute({ type: 'scene.remove', id: store.session.selection.ids[0] })
          return
        }
        if (store.session.selection.kind === 'material' && store.session.selection.ids[0]) {
          this.execute({ type: 'material.remove', id: store.session.selection.ids[0] })
          return
        }
        withDocumentMutation((document) => {
          const scene = getActiveScene(document, useEditorStore.getState().session.activeSceneId)
          deleteFromScene(scene, store.session.selection.kind, store.session.selection.ids)
        })
        ensureSelectionAfterDelete(useEditorStore.getState().session.activeSceneId)
        return
      case 'keyframe.seedCurrent':
        withDocumentMutation((document) => {
          seedCurrentKeyframe(document)
        })
        return
      case 'keyframe.clearCurrent':
        withDocumentMutation((document) => {
          clearCurrentKeyframe(document)
        })
        return
      case 'keyframe.copyPrevious':
        withDocumentMutation((document) => {
          copyPreviousKeyframe(document)
        })
        return
      case 'history.undo': {
        const { history, document } = store
        const previous = history.past[history.past.length - 1]
        if (!previous) {
          return
        }
        store.replaceDocument(cloneDocument(previous))
        store.setHistory({
          past: history.past.slice(0, -1),
          future: [cloneDocument(document), ...history.future].slice(0, HISTORY_LIMIT),
        })
        sanitizeSessionForDocument(previous)
        store.markDirty({ renderDirty: true, persistDirty: true, documentDirty: true })
        store.updateSession({ saveState: 'dirty' })
        return
      }
      case 'history.redo': {
        const { history, document } = store
        const next = history.future[0]
        if (!next) {
          return
        }
        store.replaceDocument(cloneDocument(next))
        store.setHistory({
          past: [...history.past, cloneDocument(document)].slice(-HISTORY_LIMIT),
          future: history.future.slice(1),
        })
        sanitizeSessionForDocument(next)
        store.markDirty({ renderDirty: true, persistDirty: true, documentDirty: true })
        store.updateSession({ saveState: 'dirty' })
        return
      }
      case 'persistence.markSaved':
        store.markDirty({
          documentDirty: false,
          persistDirty: false,
        })
        store.updateSession({
          saveState: 'saved',
          lastSavedAt: command.at,
        })
        return
      case 'persistence.markError':
        store.updateSession({ saveState: 'error' })
        return
      default:
        return
    }
  }
}

export const commandBus = new CommandBus()

export function buildToolStroke(anchor: Point, points: Point[], mode: 'line' | 'rect'): Point[] {
  const lastPoint = points[points.length - 1] ?? anchor
  return mode === 'line' ? buildLinePoints(anchor, lastPoint) : buildRectOutlinePoints(anchor, lastPoint)
}

export function buildFreehandStroke(anchor: Point, points: Point[]): Point[] {
  return buildStrokePoints(anchor, points)
}

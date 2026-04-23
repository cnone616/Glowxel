export type PreviewMode = 'loop' | 'minute' | 'hour'
export type ToolMode = 'select' | 'pan' | 'place' | 'path'
export type PixelTool =
  | 'brush'
  | 'eraser'
  | 'eyedropper'
  | 'fill'
  | 'rect-select'
  | 'move-selection'
  | 'line'
  | 'rect'
export type SelectionKind = 'scene' | 'background' | 'object' | 'pixel' | 'time' | 'material'
export type AssetKind = 'catalog-entry' | 'catalog-sequence' | 'uploaded-image'
export type AssetTarget = 'background' | 'object'
export type BackgroundBehavior = 'fixed' | 'loop' | 'blink'
export type ObjectBehavior = 'fixed' | 'blink' | 'path'
export type PathMode = 'loop' | 'pingpong' | 'once'
export type TimeWidgetType = 'time' | 'date' | 'week' | 'year'
export type SaveState = 'idle' | 'dirty' | 'saving' | 'saved' | 'error'
export type AssetBrowserMode = 'entries' | 'groups'
export type NoticeTone = 'info' | 'success' | 'warning' | 'error'
export type TransitionType = 'cut' | 'fade'
export type SidebarTab = 'materials' | 'scenes'
export type TimelineLayerKind = 'object' | 'pixel' | 'time'

export interface MatrixSize {
  width: number
  height: number
}

export interface CatalogFrame {
  id: string
  width: number
  height: number
  pixels: number[][]
}

export interface AssetSourceDescriptor {
  collectionId: string
  sourceAssetId: string
  mode: 'entry' | 'group'
  entryId?: string
  groupId?: string
}

export interface AssetRecord {
  id: string
  kind: AssetKind
  label: string
  previewUrl: string
  width: number
  height: number
  dataUrl?: string
  frames?: CatalogFrame[]
  source?: AssetSourceDescriptor
  sourceFileName?: string
  chromaKeyEnabled?: boolean
}

export interface LayerTiming {
  startFrame: number
  endFrame: number
}

export interface BackgroundTransformKey {
  frame: number
  x: number
  y: number
  width: number
  height: number
  opacity: number
}

export interface BackgroundStateKey {
  frame: number
  visible: boolean
  behavior: BackgroundBehavior
  rhythmFrames: number
  speedX: number
  speedY: number
  sequenceStepFrames: number
}

export interface BackgroundLayer {
  id: string
  name: string
  assetId: string
  baseColor: string
  locked: boolean
  transformKeys: BackgroundTransformKey[]
  stateKeys: BackgroundStateKey[]
}

export interface GroupTransformKey {
  frame: number
  x: number
  y: number
  scale: number
  opacity: number
}

export interface GroupStateKey {
  frame: number
  visible: boolean
  behavior: ObjectBehavior
  rhythmFrames: number
  sequenceStepFrames: number
}

export interface PathPoint {
  id: string
  x: number
  y: number
}

export interface ObjectMotion {
  mode: PathMode
  durationFrames: number
  points: PathPoint[]
}

export interface ObjectMember {
  id: string
  name: string
  assetId: string
  offsetX: number
  offsetY: number
  width: number
  height: number
  opacity: number
  visible: boolean
}

export interface ObjectGroup {
  id: string
  name: string
  locked: boolean
  timing: LayerTiming
  transformKeys: GroupTransformKey[]
  stateKeys: GroupStateKey[]
  motion: ObjectMotion
  members: ObjectMember[]
}

export interface PixelLayer {
  id: string
  name: string
  visible: boolean
  locked: boolean
  x: number
  y: number
  opacity: number
  zIndex: number
  bitmap: number[]
  timing: LayerTiming
}

export interface TimeWidget {
  id: string
  type: TimeWidgetType
  label: string
  visible: boolean
  locked: boolean
  x: number
  y: number
  scale: number
  letterSpacing: number
  color: string
  hourFormat: 12 | 24
  showSeconds: boolean
  timing: LayerTiming
}

export interface Scene {
  id: string
  name: string
  durationFrames: number
  transitionOut: TransitionType
  backgroundLayers: BackgroundLayer[]
  objectGroups: ObjectGroup[]
  pixelLayers: PixelLayer[]
  timeWidgets: TimeWidget[]
}

export interface ThemeRules {
  paletteLock: boolean
  matrixBound: MatrixSize
  pixelGridSize: number
}

export interface ThemeDocumentMeta {
  name: string
  matrix: MatrixSize
  frameDelay: number
  previewMode: PreviewMode
  previewDate: string
  previewTime: string
  updatedAt: string
}

export interface ThemeDocumentV3 {
  version: 'ThemeWorkshopDocumentV3'
  meta: ThemeDocumentMeta
  scenes: Scene[]
  materials: AssetRecord[]
  themeRules: ThemeRules
}

export type ThemeDocument = ThemeDocumentV3

export interface SelectionState {
  kind: SelectionKind
  ids: string[]
}

export interface EditorNotice {
  text: string
  tone: NoticeTone
  source: 'topbar' | 'timeline' | 'viewport' | 'system'
}

export interface Point {
  x: number
  y: number
}

export interface Rect {
  x: number
  y: number
  width: number
  height: number
}

export interface DragMoveState {
  kind: 'move'
  entityKind: Exclude<SelectionKind, 'scene' | 'material'>
  ids: string[]
  originPoint: Point
  originDocument: ThemeDocument
}

export interface DragPanState {
  kind: 'pan'
  originPoint: Point
  originOffset: Point
}

export interface DragPathPointState {
  kind: 'path-point'
  objectId: string
  pointId: string
  originPoint: Point
  originDocument: ThemeDocument
}

export type DragState = DragMoveState | DragPanState | DragPathPointState

export interface EditorSession {
  currentTool: ToolMode
  currentPixelTool: PixelTool
  selection: SelectionState
  currentFrame: number
  isPlaying: boolean
  activeSceneId: string
  activeSidebarTab: SidebarTab
  selectedMaterialId: string
  viewportZoom: number
  viewportOffset: Point
  viewportMode: 'fit' | 'manual'
  saveState: SaveState
  lastSavedAt: string
  assetBrowserMode: AssetBrowserMode
  assetSearch: string
  selectedCollectionId: string
  selectedSourceAssetId: string
  uploadDialogOpen: boolean
  uploadChromaKey: boolean
  hoverEntityId: string
  performanceMs: number
  performanceFps: number
  dragState: DragState | null
  temporaryPan: boolean
  notice: EditorNotice | null
  drawColor: string
  pixelSelection: Rect | null
}

export interface DirtyFlags {
  documentDirty: boolean
  renderDirty: boolean
  selectionDirty: boolean
  persistDirty: boolean
}

export interface EditorRuntime {
  dirtyFlags: DirtyFlags
  stageTick: number
  loaded: boolean
}

export interface DrawableBackground {
  id: string
  name: string
  baseColor: string
  asset: AssetRecord | null
  transform: BackgroundTransformKey
  state: BackgroundStateKey
}

export interface DrawableMember extends ObjectMember {
  asset: AssetRecord | null
  x: number
  y: number
}

export interface DrawableObjectGroup {
  id: string
  name: string
  transform: GroupTransformKey
  state: GroupStateKey
  motion: ObjectMotion
  members: DrawableMember[]
  bounds: Rect | null
}

export interface DrawablePixelLayer {
  id: string
  name: string
  x: number
  y: number
  opacity: number
  zIndex: number
  bitmap: number[]
  bounds: Rect
}

export interface DrawableTimeWidget extends TimeWidget {
  text: string
  bounds: Rect
}

export interface TransitionOverlay {
  alpha: number
  sceneId: string
  localFrame: number
  backgrounds: DrawableBackground[]
  pixelLayers: DrawablePixelLayer[]
  objects: DrawableObjectGroup[]
  widgets: DrawableTimeWidget[]
}

export interface RenderSnapshot {
  matrix: MatrixSize
  previewDate: Date
  sceneId: string
  sceneName: string
  sceneFrame: number
  sceneDurationFrames: number
  documentFrameCount: number
  baseAlpha: number
  backgrounds: DrawableBackground[]
  pixelLayers: DrawablePixelLayer[]
  objects: DrawableObjectGroup[]
  widgets: DrawableTimeWidget[]
  transitionOverlay: TransitionOverlay | null
}

export interface AssetCatalogCollection {
  id: string
  label: string
  sourceAssets: AssetCatalogSourceAsset[]
}

export interface AssetCatalogSourceAsset {
  id: string
  label: string
  description: string
  jsonPath: string
}

export interface AssetCatalogManifest {
  version: string
  collections: AssetCatalogCollection[]
}

export interface CatalogEntry {
  id: string
  label: string
  width: number
  height: number
  pixels: number[][]
}

export interface CatalogGroup {
  id: string
  label: string
  entryIds: string[]
}

export interface NormalizedCatalogAsset {
  id: string
  label: string
  description: string
  collectionId: string
  entries: CatalogEntry[]
  entryMap: Record<string, CatalogEntry>
  groups: CatalogGroup[]
}

export interface CatalogState {
  loading: boolean
  error: string
  manifest: AssetCatalogManifest | null
  sourceCache: Record<string, NormalizedCatalogAsset>
}

export interface WorkerUploadTask {
  file: File
  chromaKey: boolean
}

export interface WorkerProcessedAsset {
  id: string
  label: string
  previewUrl: string
  dataUrl: string
  width: number
  height: number
  chromaKeyEnabled: boolean
  sourceFileName: string
}

export type EditorCommand =
  | { type: 'selection.set'; kind: SelectionKind; ids: string[]; additive: boolean }
  | { type: 'selection.clear' }
  | { type: 'session.setTool'; tool: ToolMode }
  | { type: 'session.setPixelTool'; tool: PixelTool }
  | { type: 'session.setSidebarTab'; tab: SidebarTab }
  | { type: 'session.setDrawColor'; color: string }
  | { type: 'session.setPixelSelection'; selection: Rect | null }
  | { type: 'session.togglePlay' }
  | { type: 'session.setPreviewMode'; previewMode: PreviewMode }
  | { type: 'session.setAssetMode'; mode: AssetBrowserMode }
  | { type: 'session.setAssetSearch'; value: string }
  | { type: 'session.setCollection'; collectionId: string }
  | { type: 'session.setSourceAsset'; sourceAssetId: string }
  | { type: 'session.setZoomPreset'; zoom: number }
  | { type: 'session.fitViewport' }
  | { type: 'session.panViewport'; offset: Point }
  | { type: 'session.setUploadDialogOpen'; open: boolean }
  | { type: 'session.setUploadChromaKey'; value: boolean }
  | { type: 'session.setHoverEntity'; entityId: string }
  | { type: 'session.setNotice'; notice: EditorNotice }
  | { type: 'session.clearNotice' }
  | { type: 'document.rename'; name: string }
  | { type: 'document.import'; document: ThemeDocument }
  | { type: 'document.setFrameDelay'; value: number }
  | { type: 'document.setPreviewDate'; value: string }
  | { type: 'document.setPreviewTime'; value: string }
  | { type: 'themeRule.update'; patch: Partial<ThemeRules> }
  | { type: 'scene.add' }
  | { type: 'scene.remove'; id: string }
  | { type: 'scene.duplicate'; id: string }
  | { type: 'scene.reorder'; id: string; toIndex: number }
  | { type: 'scene.select'; id: string }
  | { type: 'scene.trim'; id: string; durationFrames: number }
  | { type: 'scene.update'; id: string; patch: Partial<Pick<Scene, 'name' | 'transitionOut'>> }
  | { type: 'material.addCatalog'; asset: AssetRecord }
  | { type: 'material.addUploaded'; asset: AssetRecord }
  | { type: 'material.remove'; id: string }
  | { type: 'material.select'; id: string }
  | { type: 'scene.insertMaterialAsBackground'; sceneId: string; materialId: string }
  | { type: 'scene.insertMaterialAsObject'; sceneId: string; materialId: string }
  | { type: 'outline.rename'; kind: Exclude<SelectionKind, 'scene' | 'material'>; id: string; name: string }
  | { type: 'outline.toggleVisibility'; kind: Exclude<SelectionKind, 'scene' | 'material'>; id: string }
  | { type: 'outline.toggleLock'; kind: Exclude<SelectionKind, 'scene' | 'material'>; id: string }
  | { type: 'outline.reorder'; kind: Exclude<SelectionKind, 'scene' | 'material'>; id: string; direction: 'up' | 'down' }
  | { type: 'background.updateTransform'; id: string; patch: Partial<Omit<BackgroundTransformKey, 'frame'>> }
  | { type: 'background.updateState'; id: string; patch: Partial<Omit<BackgroundStateKey, 'frame'>> }
  | { type: 'background.updateBase'; id: string; patch: Partial<Pick<BackgroundLayer, 'assetId' | 'baseColor'>> }
  | { type: 'object.updateTransform'; id: string; patch: Partial<Omit<GroupTransformKey, 'frame'>> }
  | { type: 'object.updateState'; id: string; patch: Partial<Omit<GroupStateKey, 'frame'>> }
  | { type: 'object.updateMotion'; id: string; patch: Partial<Omit<ObjectMotion, 'points'>> }
  | { type: 'object.addPathPoint'; id: string; point: Point }
  | { type: 'object.updatePathPoint'; id: string; pointId: string; patch: Point }
  | { type: 'object.removePathPoint'; id: string; pointId: string }
  | { type: 'object.clearPath'; id: string }
  | { type: 'time.update'; id: string; patch: Partial<TimeWidget> }
  | { type: 'pixelLayer.create'; sceneId: string }
  | { type: 'pixelLayer.delete'; id: string }
  | { type: 'pixelLayer.duplicate'; id: string }
  | { type: 'pixelLayer.reorder'; id: string; direction: 'up' | 'down' }
  | { type: 'pixelLayer.select'; id: string }
  | { type: 'pixelLayer.update'; id: string; patch: Partial<Omit<PixelLayer, 'id' | 'bitmap' | 'timing'>> }
  | { type: 'pixelLayer.paint'; id: string; points: Point[]; color: string }
  | { type: 'pixelLayer.erase'; id: string; points: Point[] }
  | { type: 'pixelLayer.fill'; id: string; point: Point; color: string }
  | { type: 'pixelLayer.eyedropper'; id: string; point: Point }
  | { type: 'pixelLayer.transformSelection'; id: string; selection: Rect; delta: Point }
  | { type: 'pixelLayer.flip'; id: string; axis: 'horizontal' | 'vertical' }
  | { type: 'layer.setTiming'; kind: TimelineLayerKind; id: string; timing: LayerTiming }
  | { type: 'timeline.setPlayhead'; frame: number }
  | { type: 'document.deleteSelection' }
  | { type: 'keyframe.seedCurrent' }
  | { type: 'keyframe.clearCurrent' }
  | { type: 'keyframe.copyPrevious' }
  | { type: 'history.undo' }
  | { type: 'history.redo' }
  | { type: 'persistence.markSaved'; at: string }
  | { type: 'persistence.markError' }

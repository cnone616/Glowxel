<script setup>
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import {
  buildProjectAssetFromCatalogEntry,
  buildProjectAssetFromCatalogGroup,
  buildThumbnailDataUrl,
  loadAssetCatalog,
  loadCatalogSourceAsset,
} from './lib/catalog-loader'
import { createUploadedProjectAsset } from './lib/image-upload'
import { formatWidgetText, measurePixelText } from './lib/pixel-font'
import { buildSceneSnapshot, renderSceneToCanvas } from './lib/scene-renderer'
import {
  MATRIX_PRESETS,
  clamp,
  createBackgroundLayer,
  createCharacterGroup,
  createDefaultScene,
  createGroupMember,
  deepClone,
  ensureSceneShape,
  fitRect,
  getAssetFrameSize,
  hasKeyframe,
  removeKeyframe,
  sampleContinuousKeys,
  sampleStepKeys,
  uid,
  upsertKeyframe,
} from './lib/scene-model'
import { loadStoredScene, saveStoredScene } from './lib/storage'

const stageCanvasRef = ref(null)
const stageViewportRef = ref(null)
const importInputRef = ref(null)
const uploadInputRef = ref(null)

const scene = ref(createDefaultScene())
const stageSnapshot = ref(buildSceneSnapshot(scene.value))
const saveState = ref('idle')
const statusMessage = ref('正在准备新的主题编辑器工作台…')
const libraryState = ref({
  loading: true,
  error: '',
  manifest: null,
  cache: {},
})

const leftTabs = [
  { id: 'library', label: '素材库' },
  { id: 'tracks', label: '轨道' },
]

const rightTabs = [
  { id: 'scene', label: '场景' },
  { id: 'background', label: '背景' },
  { id: 'character', label: '角色' },
  { id: 'time', label: '时间' },
]

let playbackTimerId = 0
let autosaveTimerId = 0
let renderTicket = 0
let initialized = false
let internalSceneMutation = false
let dragState = null

const matrixWidth = computed(() => scene.value.meta.matrix.width)
const matrixHeight = computed(() => scene.value.meta.matrix.height)
const frameIndices = computed(() =>
  Array.from({ length: scene.value.meta.frameCount }, (_, index) => index),
)

const selectedBackground = computed(() =>
  scene.value.backgroundLayers.find((layer) => layer.id === scene.value.editorState.selectedId) ?? null,
)
const selectedCharacterGroup = computed(() =>
  scene.value.characterGroups.find((group) => group.id === scene.value.editorState.selectedId) ?? null,
)
const selectedTimeWidget = computed(() =>
  scene.value.timeWidgets.find((widget) => widget.id === scene.value.editorState.selectedId) ?? null,
)

const currentCollection = computed(() => {
  if (libraryState.value.manifest === null) {
    return null
  }
  return (
    libraryState.value.manifest.collections.find(
      (collection) => collection.id === scene.value.editorState.libraryCollectionId,
    ) ?? libraryState.value.manifest.collections[0] ?? null
  )
})

const currentCollectionSourceAssets = computed(() =>
  currentCollection.value === null ? [] : currentCollection.value.sourceAssets,
)

const currentSourceAssetDescriptor = computed(() =>
  currentCollectionSourceAssets.value.find(
    (descriptor) => descriptor.id === scene.value.editorState.librarySourceAssetId,
  ) ?? currentCollectionSourceAssets.value[0] ?? null,
)

const currentSourceAsset = computed(() => {
  if (currentCollection.value === null || currentSourceAssetDescriptor.value === null) {
    return null
  }
  const cacheKey = `${currentCollection.value.id}:${currentSourceAssetDescriptor.value.id}`
  return libraryState.value.cache[cacheKey] ?? null
})

const filteredEntries = computed(() => {
  if (currentSourceAsset.value === null) {
    return []
  }
  const keyword = scene.value.editorState.librarySearch.trim().toLowerCase()
  return currentSourceAsset.value.entries.filter((entry) => {
    if (keyword.length === 0) {
      return true
    }
    return entry.id.toLowerCase().includes(keyword)
  })
})

const filteredGroups = computed(() => {
  if (currentSourceAsset.value === null) {
    return []
  }
  const keyword = scene.value.editorState.librarySearch.trim().toLowerCase()
  return currentSourceAsset.value.groups.filter((group) => {
    if (group.id === 'all') {
      return false
    }
    if (keyword.length === 0) {
      return true
    }
    return group.id.toLowerCase().includes(keyword)
  })
})

const uploadedAssets = computed(() =>
  scene.value.assets.filter((asset) => asset.kind === 'uploaded-image'),
)

const backgroundAssetOptions = computed(() => scene.value.assets)
const characterAssetOptions = computed(() => scene.value.assets)

const selectedBackgroundView = computed(() => {
  if (selectedBackground.value === null) {
    return null
  }
  return {
    transform: sampleContinuousKeys(selectedBackground.value.transformKeys, scene.value.meta.currentFrame, {
      x: 0,
      y: 0,
      width: matrixWidth.value,
      height: matrixHeight.value,
      opacity: 1,
    }),
    state: sampleStepKeys(selectedBackground.value.stateKeys, scene.value.meta.currentFrame, {
      visible: true,
      behavior: 'fixed',
      rhythmFrames: 2,
      speedX: 0,
      speedY: 0,
      sequenceStepFrames: 1,
    }),
  }
})

const selectedCharacterView = computed(() => {
  if (selectedCharacterGroup.value === null) {
    return null
  }
  return {
    transform: sampleContinuousKeys(selectedCharacterGroup.value.transformKeys, scene.value.meta.currentFrame, {
      x: 0,
      y: 0,
      scale: 1,
      opacity: 1,
    }),
    state: sampleStepKeys(selectedCharacterGroup.value.stateKeys, scene.value.meta.currentFrame, {
      visible: true,
      behavior: 'fixed',
      rhythmFrames: 2,
      sequenceStepFrames: 1,
    }),
  }
})

const selectedFrameMarkerInfo = computed(() => {
  if (scene.value.editorState.selectedType === 'background' && selectedBackground.value !== null) {
    return {
      transform: hasKeyframe(selectedBackground.value.transformKeys, scene.value.meta.currentFrame),
      state: hasKeyframe(selectedBackground.value.stateKeys, scene.value.meta.currentFrame),
    }
  }

  if (scene.value.editorState.selectedType === 'character' && selectedCharacterGroup.value !== null) {
    return {
      transform: hasKeyframe(selectedCharacterGroup.value.transformKeys, scene.value.meta.currentFrame),
      state: hasKeyframe(selectedCharacterGroup.value.stateKeys, scene.value.meta.currentFrame),
    }
  }

  return {
    transform: false,
    state: false,
  }
})

function setStatus(message) {
  statusMessage.value = message
}

function getAutosaveLabel() {
  if (saveState.value === 'saving') {
    return '正在自动保存'
  }
  if (saveState.value === 'saved') {
    return scene.value.meta.autosavedAt.length > 0
      ? `已保存 ${scene.value.meta.autosavedAt.slice(11, 19)}`
      : '已保存'
  }
  if (saveState.value === 'error') {
    return '自动保存失败'
  }
  return '未保存'
}

function ensureSelectionValid() {
  const selectedType = scene.value.editorState.selectedType
  const selectedId = scene.value.editorState.selectedId

  if (
    selectedType === 'background' &&
    scene.value.backgroundLayers.some((layer) => layer.id === selectedId)
  ) {
    return
  }
  if (
    selectedType === 'character' &&
    scene.value.characterGroups.some((group) => group.id === selectedId)
  ) {
    return
  }
  if (
    selectedType === 'time' &&
    scene.value.timeWidgets.some((widget) => widget.id === selectedId)
  ) {
    return
  }

  if (scene.value.backgroundLayers.length > 0) {
    scene.value.editorState.selectedType = 'background'
    scene.value.editorState.selectedId = scene.value.backgroundLayers[0].id
    scene.value.editorState.rightTab = 'background'
    return
  }

  if (scene.value.characterGroups.length > 0) {
    scene.value.editorState.selectedType = 'character'
    scene.value.editorState.selectedId = scene.value.characterGroups[0].id
    scene.value.editorState.rightTab = 'character'
    return
  }

  if (scene.value.timeWidgets.length > 0) {
    scene.value.editorState.selectedType = 'time'
    scene.value.editorState.selectedId = scene.value.timeWidgets[0].id
    scene.value.editorState.rightTab = 'time'
  }
}

function selectEntity(type, id) {
  scene.value.editorState.selectedType = type
  scene.value.editorState.selectedId = id
  if (type === 'background') {
    scene.value.editorState.rightTab = 'background'
  } else if (type === 'character') {
    scene.value.editorState.rightTab = 'character'
  } else if (type === 'time') {
    scene.value.editorState.rightTab = 'time'
  }
}

async function ensureCatalogSourceLoaded() {
  if (currentCollection.value === null || currentSourceAssetDescriptor.value === null) {
    return
  }

  const cacheKey = `${currentCollection.value.id}:${currentSourceAssetDescriptor.value.id}`
  if (typeof libraryState.value.cache[cacheKey] === 'object') {
    return
  }

  libraryState.value.loading = true
  try {
    const sourceAsset = await loadCatalogSourceAsset(currentCollection.value, currentSourceAssetDescriptor.value)
    libraryState.value.cache[cacheKey] = sourceAsset
    libraryState.value.error = ''
  } catch (error) {
    libraryState.value.error = error instanceof Error ? error.message : '素材读取失败'
  } finally {
    libraryState.value.loading = false
  }
}

async function loadCatalogManifest() {
  libraryState.value.loading = true
  try {
    const manifest = await loadAssetCatalog()
    libraryState.value.manifest = manifest
    if (manifest.collections.length > 0) {
      scene.value.editorState.libraryCollectionId = manifest.collections[0].id
      if (manifest.collections[0].sourceAssets.length > 0) {
        scene.value.editorState.librarySourceAssetId = manifest.collections[0].sourceAssets[0].id
      }
    }
    await ensureCatalogSourceLoaded()
    setStatus('镜像像素素材库已接入，可以直接从左侧添加背景层和角色组。')
  } catch (error) {
    libraryState.value.error = error instanceof Error ? error.message : '素材目录初始化失败'
    setStatus(libraryState.value.error)
  } finally {
    libraryState.value.loading = false
  }
}

function projectAssetLabel(assetId) {
  const asset = scene.value.assets.find((item) => item.id === assetId)
  return asset === undefined ? '未绑定素材' : asset.label
}

function createProjectAssetFromEntry(entryId, trackHint) {
  return buildProjectAssetFromCatalogEntry(currentSourceAsset.value, entryId, trackHint)
}

function createProjectAssetFromGroup(groupId, trackHint) {
  return buildProjectAssetFromCatalogGroup(currentSourceAsset.value, groupId, trackHint)
}

function addBackgroundLayerFromProjectAsset(projectAsset) {
  const layer = createBackgroundLayer(projectAsset, scene.value.meta.matrix)
  scene.value.assets.push(projectAsset)
  scene.value.backgroundLayers.push(layer)
  selectEntity('background', layer.id)
  scene.value.editorState.leftTab = 'tracks'
  setStatus(`已把 ${projectAsset.label} 加入背景轨道。`)
}

function addCharacterGroupFromProjectAsset(projectAsset) {
  const group = createCharacterGroup(projectAsset, scene.value.meta.matrix)
  scene.value.assets.push(projectAsset)
  scene.value.characterGroups.push(group)
  selectEntity('character', group.id)
  scene.value.editorState.leftTab = 'tracks'
  setStatus(`已新建角色组 ${group.name}。`)
}

function appendProjectAssetToSelectedGroup(projectAsset) {
  if (selectedCharacterGroup.value === null) {
    setStatus('先选中一个角色组，再把素材加入组内。')
    return
  }

  scene.value.assets.push(projectAsset)
  selectedCharacterGroup.value.members.push(createGroupMember(projectAsset))
  scene.value.editorState.selectedMemberId =
    selectedCharacterGroup.value.members[selectedCharacterGroup.value.members.length - 1].id
  selectEntity('character', selectedCharacterGroup.value.id)
  setStatus(`已把 ${projectAsset.label} 加入角色组 ${selectedCharacterGroup.value.name}。`)
}

function addEntryAsBackground(entryId) {
  addBackgroundLayerFromProjectAsset(createProjectAssetFromEntry(entryId, 'background'))
}

function addEntryAsCharacter(entryId) {
  addCharacterGroupFromProjectAsset(createProjectAssetFromEntry(entryId, 'character'))
}

function appendEntryToCurrentGroup(entryId) {
  appendProjectAssetToSelectedGroup(createProjectAssetFromEntry(entryId, 'character'))
}

function addGroupAsBackground(groupId) {
  addBackgroundLayerFromProjectAsset(createProjectAssetFromGroup(groupId, 'background'))
}

function addGroupAsCharacter(groupId) {
  addCharacterGroupFromProjectAsset(createProjectAssetFromGroup(groupId, 'character'))
}

function appendGroupToCurrentGroup(groupId) {
  appendProjectAssetToSelectedGroup(createProjectAssetFromGroup(groupId, 'character'))
}

async function handleImageUpload(event) {
  const fileList = event.target.files
  if (fileList === null) {
    return
  }

  const files = Array.from(fileList)
  if (files.length === 0) {
    return
  }

  for (let index = 0; index < files.length; index += 1) {
    const trackHint = scene.value.editorState.uploadTarget === 'background' ? 'background' : 'character'
    const uploadedAsset = await createUploadedProjectAsset(files[index], {
      chromaKey: scene.value.editorState.uploadChromaKey,
      trackHint,
    })
    if (trackHint === 'background') {
      addBackgroundLayerFromProjectAsset(uploadedAsset)
    } else {
      addCharacterGroupFromProjectAsset(uploadedAsset)
    }
  }

  event.target.value = ''
}

function handleUploadedAssetAction(projectAsset, action) {
  if (action === 'background') {
    addBackgroundLayerFromProjectAsset({
      ...deepClone(projectAsset),
      id: uid('asset'),
    })
    return
  }

  if (action === 'character') {
    addCharacterGroupFromProjectAsset({
      ...deepClone(projectAsset),
      id: uid('asset'),
    })
    return
  }

  appendProjectAssetToSelectedGroup({
    ...deepClone(projectAsset),
    id: uid('asset'),
  })
}

function matrixPresetId() {
  const preset = MATRIX_PRESETS.find(
    (item) =>
      item.id !== 'custom' &&
      item.width === scene.value.meta.matrix.width &&
      item.height === scene.value.meta.matrix.height,
  )
  return preset === undefined ? 'custom' : preset.id
}

function applyMatrixPreset(presetId) {
  const preset = MATRIX_PRESETS.find((item) => item.id === presetId)
  if (preset === undefined) {
    return
  }
  applyMatrixSize(preset.width, preset.height)
}

function applyMatrixSize(width, height) {
  const nextWidth = clamp(parseInt(width, 10) || 64, 16, 160)
  const nextHeight = clamp(parseInt(height, 10) || 64, 16, 160)
  scene.value.meta.matrix.width = nextWidth
  scene.value.meta.matrix.height = nextHeight

  for (let index = 0; index < scene.value.backgroundLayers.length; index += 1) {
    const layer = scene.value.backgroundLayers[index]
    if (layer.projectAssetId.length === 0) {
      layer.transformKeys = upsertKeyframe(layer.transformKeys, 0, {
        x: 0,
        y: 0,
        width: nextWidth,
        height: nextHeight,
        opacity: 1,
      })
    }
  }

  for (let index = 0; index < scene.value.timeWidgets.length; index += 1) {
    fitWidgetIntoMatrix(scene.value.timeWidgets[index], {
      width: nextWidth,
      height: nextHeight,
    })
  }

  setStatus(`矩阵尺寸已切到 ${nextWidth}×${nextHeight}。`)
}

function setFrameCount(value) {
  scene.value.meta.frameCount = clamp(parseInt(value, 10) || 12, 1, 120)
  scene.value.meta.currentFrame = clamp(
    scene.value.meta.currentFrame,
    0,
    scene.value.meta.frameCount - 1,
  )
}

function setCurrentFrame(value) {
  scene.value.meta.currentFrame = clamp(parseInt(value, 10) || 0, 0, scene.value.meta.frameCount - 1)
}

function stepFrame(delta) {
  const frameCount = scene.value.meta.frameCount
  const nextFrame = (scene.value.meta.currentFrame + delta + frameCount) % frameCount
  scene.value.meta.currentFrame = nextFrame
}

function togglePlayback() {
  scene.value.meta.isPlaying = !scene.value.meta.isPlaying
}

function syncPlaybackTimer() {
  if (playbackTimerId !== 0) {
    window.clearInterval(playbackTimerId)
    playbackTimerId = 0
  }

  if (scene.value.meta.isPlaying !== true) {
    return
  }

  playbackTimerId = window.setInterval(() => {
    stepFrame(1)
  }, scene.value.meta.frameDelay)
}

function updateBackgroundTransform(patch) {
  if (selectedBackground.value === null || selectedBackgroundView.value === null) {
    return
  }
  const snapshot = {
    ...selectedBackgroundView.value.transform,
    ...patch,
  }
  selectedBackground.value.transformKeys = upsertKeyframe(
    selectedBackground.value.transformKeys,
    scene.value.meta.currentFrame,
    snapshot,
  )
}

function updateBackgroundState(patch) {
  if (selectedBackground.value === null || selectedBackgroundView.value === null) {
    return
  }
  const snapshot = {
    ...selectedBackgroundView.value.state,
    ...patch,
  }
  selectedBackground.value.stateKeys = upsertKeyframe(
    selectedBackground.value.stateKeys,
    scene.value.meta.currentFrame,
    snapshot,
  )
}

function updateGroupTransform(patch) {
  if (selectedCharacterGroup.value === null || selectedCharacterView.value === null) {
    return
  }
  const snapshot = {
    ...selectedCharacterView.value.transform,
    ...patch,
  }
  selectedCharacterGroup.value.transformKeys = upsertKeyframe(
    selectedCharacterGroup.value.transformKeys,
    scene.value.meta.currentFrame,
    snapshot,
  )
}

function updateGroupState(patch) {
  if (selectedCharacterGroup.value === null || selectedCharacterView.value === null) {
    return
  }
  const snapshot = {
    ...selectedCharacterView.value.state,
    ...patch,
  }
  selectedCharacterGroup.value.stateKeys = upsertKeyframe(
    selectedCharacterGroup.value.stateKeys,
    scene.value.meta.currentFrame,
    snapshot,
  )
}

function seedSelectedFrameKeys() {
  if (scene.value.editorState.selectedType === 'background' && selectedBackgroundView.value !== null) {
    updateBackgroundTransform({})
    updateBackgroundState({})
    setStatus('已把当前背景层状态写入选中帧。')
    return
  }

  if (scene.value.editorState.selectedType === 'character' && selectedCharacterView.value !== null) {
    updateGroupTransform({})
    updateGroupState({})
    setStatus('已把当前角色组状态写入选中帧。')
  }
}

function clearSelectedFrameKeys() {
  if (scene.value.meta.currentFrame === 0) {
    setStatus('第 1 帧作为基础帧保留，不建议清掉。')
    return
  }

  if (scene.value.editorState.selectedType === 'background' && selectedBackground.value !== null) {
    selectedBackground.value.transformKeys = removeKeyframe(
      selectedBackground.value.transformKeys,
      scene.value.meta.currentFrame,
    )
    selectedBackground.value.stateKeys = removeKeyframe(
      selectedBackground.value.stateKeys,
      scene.value.meta.currentFrame,
    )
    setStatus('已移除当前背景层在选中帧上的关键状态。')
    return
  }

  if (scene.value.editorState.selectedType === 'character' && selectedCharacterGroup.value !== null) {
    selectedCharacterGroup.value.transformKeys = removeKeyframe(
      selectedCharacterGroup.value.transformKeys,
      scene.value.meta.currentFrame,
    )
    selectedCharacterGroup.value.stateKeys = removeKeyframe(
      selectedCharacterGroup.value.stateKeys,
      scene.value.meta.currentFrame,
    )
    setStatus('已移除当前角色组在选中帧上的关键状态。')
  }
}

function fitSelectedBackground() {
  if (selectedBackground.value === null) {
    return
  }
  const asset = scene.value.assets.find((item) => item.id === selectedBackground.value.projectAssetId) ?? null
  if (asset === null) {
    updateBackgroundTransform({
      x: 0,
      y: 0,
      width: matrixWidth.value,
      height: matrixHeight.value,
    })
    return
  }
  const size = getAssetFrameSize(asset)
  const fitted = fitRect(size.width, size.height, matrixWidth.value, matrixHeight.value)
  updateBackgroundTransform(fitted)
}

function centerSelectedGroup() {
  if (selectedCharacterGroup.value === null || selectedCharacterView.value === null) {
    return
  }

  const bounds = stageSnapshot.value.characters.find(
    (group) => group.id === selectedCharacterGroup.value.id,
  )?.bounds
  if (bounds === undefined || bounds === null) {
    return
  }

  updateGroupTransform({
    x: selectedCharacterView.value.transform.x + Math.round((matrixWidth.value - bounds.width) / 2 - bounds.x),
    y: selectedCharacterView.value.transform.y + Math.round((matrixHeight.value - bounds.height) / 2 - bounds.y),
  })
}

function removeSelectedBackground() {
  if (selectedBackground.value === null) {
    return
  }
  scene.value.backgroundLayers = scene.value.backgroundLayers.filter(
    (layer) => layer.id !== selectedBackground.value.id,
  )
  ensureSelectionValid()
}

function removeSelectedGroup() {
  if (selectedCharacterGroup.value === null) {
    return
  }
  scene.value.characterGroups = scene.value.characterGroups.filter(
    (group) => group.id !== selectedCharacterGroup.value.id,
  )
  ensureSelectionValid()
}

function nudgeMember(memberId, axis, delta) {
  if (selectedCharacterGroup.value === null) {
    return
  }
  const member = selectedCharacterGroup.value.members.find((item) => item.id === memberId)
  if (member === undefined) {
    return
  }
  if (axis === 'x') {
    member.offsetX += delta
  } else if (axis === 'y') {
    member.offsetY += delta
  } else if (axis === 'w') {
    member.width = Math.max(1, member.width + delta)
  } else if (axis === 'h') {
    member.height = Math.max(1, member.height + delta)
  }
}

function updateMemberAsset(memberId, assetId) {
  if (selectedCharacterGroup.value === null) {
    return
  }
  const member = selectedCharacterGroup.value.members.find((item) => item.id === memberId)
  const asset = scene.value.assets.find((item) => item.id === assetId)
  if (member === undefined || asset === undefined) {
    return
  }
  const size = getAssetFrameSize(asset)
  member.projectAssetId = asset.id
  member.name = asset.label
  member.width = size.width
  member.height = size.height
}

function removeMember(memberId) {
  if (selectedCharacterGroup.value === null) {
    return
  }
  selectedCharacterGroup.value.members = selectedCharacterGroup.value.members.filter(
    (member) => member.id !== memberId,
  )
}

function addSelectedLibraryAssetToCurrentGroup() {
  if (currentSourceAsset.value === null || selectedCharacterGroup.value === null) {
    return
  }

  if (scene.value.editorState.libraryMode === 'groups' && filteredGroups.value.length > 0) {
    appendGroupToCurrentGroup(filteredGroups.value[0].id)
    return
  }

  if (filteredEntries.value.length > 0) {
    appendEntryToCurrentGroup(filteredEntries.value[0].id)
  }
}

function updateWidget(widgetId, patch) {
  const widget = scene.value.timeWidgets.find((item) => item.id === widgetId)
  if (widget === undefined) {
    return
  }
  Object.assign(widget, patch)
}

function fitWidgetIntoMatrix(widget, matrix) {
  const previewDate =
    stageSnapshot.value.previewDate instanceof Date ? stageSnapshot.value.previewDate : new Date()
  const letterSpacing = Math.max(0, widget.letterSpacing)
  let scale = Math.max(1, Number(widget.scale) || 1)
  let measure = measurePixelText(formatWidgetText(widget, previewDate), scale, letterSpacing)

  while (scale > 1 && (measure.width > matrix.width || measure.height > matrix.height)) {
    scale -= 1
    measure = measurePixelText(formatWidgetText(widget, previewDate), scale, letterSpacing)
  }

  widget.scale = scale
  widget.x = clamp(widget.x, 0, Math.max(0, matrix.width - measure.width))
  widget.y = clamp(widget.y, 0, Math.max(0, matrix.height - measure.height))
}

function selectRightTab(tabId) {
  scene.value.editorState.rightTab = tabId
}

function selectLeftTab(tabId) {
  scene.value.editorState.leftTab = tabId
}

async function importSceneFile(event) {
  const fileList = event.target.files
  if (fileList === null || fileList.length === 0) {
    return
  }

  try {
    const text = await fileList[0].text()
    const parsed = JSON.parse(text)
    ensureSceneShape(parsed)
    scene.value = deepClone(parsed)
    ensureSelectionValid()
    setStatus(`已导入场景 ${scene.value.meta.name}。`)
  } catch (error) {
    setStatus(error instanceof Error ? error.message : '导入失败')
  } finally {
    event.target.value = ''
  }
}

function exportSceneJson() {
  const blob = new Blob([JSON.stringify(scene.value, null, 2)], {
    type: 'application/json',
  })
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = `${scene.value.meta.name || 'theme-scene'}.json`
  document.body.appendChild(anchor)
  anchor.click()
  anchor.remove()
  URL.revokeObjectURL(url)
  setStatus('当前场景 JSON 已导出。')
}

function resetScene() {
  scene.value = createDefaultScene()
  setStatus('已重置为新的空白主题场景。')
}

async function renderStage() {
  await nextTick()
  if (stageCanvasRef.value === null) {
    return
  }

  const ticket = ++renderTicket
  const snapshot = await renderSceneToCanvas(stageCanvasRef.value, scene.value)
  if (ticket !== renderTicket) {
    return
  }
  stageSnapshot.value = snapshot
}

function pointerToMatrix(event) {
  if (stageViewportRef.value === null) {
    return { x: 0, y: 0 }
  }
  const rect = stageViewportRef.value.getBoundingClientRect()
  const x = Math.floor(((event.clientX - rect.left) / rect.width) * matrixWidth.value)
  const y = Math.floor(((event.clientY - rect.top) / rect.height) * matrixHeight.value)
  return {
    x: clamp(x, 0, matrixWidth.value - 1),
    y: clamp(y, 0, matrixHeight.value - 1),
  }
}

function beginDrag(type, id, event) {
  event.preventDefault()
  const point = pointerToMatrix(event)

  if (type === 'character') {
    const group = stageSnapshot.value.characters.find((item) => item.id === id)
    if (group === undefined || group.bounds === null) {
      return
    }
    selectEntity('character', id)
    dragState = {
      type,
      id,
      pointerX: point.x,
      pointerY: point.y,
      originX: selectedCharacterView.value?.transform.x ?? 0,
      originY: selectedCharacterView.value?.transform.y ?? 0,
    }
    return
  }

  const widget = stageSnapshot.value.widgets.find((item) => item.id === id)
  if (widget === undefined) {
    return
  }
  selectEntity('time', id)
  dragState = {
    type,
    id,
    pointerX: point.x,
    pointerY: point.y,
    originX: widget.x,
    originY: widget.y,
  }
}

function handlePointerMove(event) {
  if (dragState === null) {
    return
  }

  const point = pointerToMatrix(event)
  const deltaX = point.x - dragState.pointerX
  const deltaY = point.y - dragState.pointerY

  if (dragState.type === 'character') {
    updateGroupTransform({
      x: dragState.originX + deltaX,
      y: dragState.originY + deltaY,
    })
    return
  }

  updateWidget(dragState.id, {
    x: clamp(dragState.originX + deltaX, 0, matrixWidth.value - 1),
    y: clamp(dragState.originY + deltaY, 0, matrixHeight.value - 1),
  })
}

function handlePointerUp() {
  dragState = null
}

function scheduleAutosave() {
  if (initialized !== true || internalSceneMutation === true) {
    return
  }

  if (autosaveTimerId !== 0) {
    window.clearTimeout(autosaveTimerId)
  }

  saveState.value = 'saving'
  autosaveTimerId = window.setTimeout(async () => {
    try {
      const payload = deepClone(scene.value)
      payload.meta.autosavedAt = new Date().toISOString()
      await saveStoredScene(payload)
      internalSceneMutation = true
      scene.value.meta.autosavedAt = payload.meta.autosavedAt
      internalSceneMutation = false
      saveState.value = 'saved'
    } catch (error) {
      saveState.value = 'error'
    }
  }, 500)
}

function visibleFrameDots(frameIndex) {
  if (scene.value.editorState.selectedType === 'background' && selectedBackground.value !== null) {
    return {
      transform: hasKeyframe(selectedBackground.value.transformKeys, frameIndex),
      state: hasKeyframe(selectedBackground.value.stateKeys, frameIndex),
    }
  }

  if (scene.value.editorState.selectedType === 'character' && selectedCharacterGroup.value !== null) {
    return {
      transform: hasKeyframe(selectedCharacterGroup.value.transformKeys, frameIndex),
      state: hasKeyframe(selectedCharacterGroup.value.stateKeys, frameIndex),
    }
  }

  return { transform: false, state: false }
}

function timelineLabel() {
  return `第 ${scene.value.meta.currentFrame + 1} / ${scene.value.meta.frameCount} 帧`
}

function previewTimeLabel(widget) {
  return formatWidgetText(widget, stageSnapshot.value.previewDate)
}

function itemThumb(entryOrGroup) {
  if (entryOrGroup.previewUrl) {
    return entryOrGroup.previewUrl
  }
  if (entryOrGroup.entryIds) {
    const firstEntry = currentSourceAsset.value.entryMap[entryOrGroup.entryIds[0]]
    return buildThumbnailDataUrl(firstEntry)
  }
  return buildThumbnailDataUrl(entryOrGroup)
}

async function bootstrap() {
  await loadCatalogManifest()

  try {
    const storedScene = await loadStoredScene()
    if (storedScene !== null) {
      ensureSceneShape(storedScene)
      scene.value = storedScene
      setStatus('已恢复上次自动保存的工作台场景。')
    } else {
      scene.value = createDefaultScene()
    }
  } catch (error) {
    scene.value = createDefaultScene()
    setStatus('未找到有效的本地场景，已按默认工作台启动。')
  }

  ensureSelectionValid()
  initialized = true
  await renderStage()
}

watch(
  () => [scene.value.meta.isPlaying, scene.value.meta.frameDelay, scene.value.meta.frameCount],
  () => {
    syncPlaybackTimer()
  },
)

watch(
  scene,
  () => {
    ensureSelectionValid()
    scheduleAutosave()
    renderStage()
  },
  { deep: true },
)

watch(
  () => [scene.value.editorState.libraryCollectionId, scene.value.editorState.librarySourceAssetId],
  () => {
    ensureCatalogSourceLoaded()
  },
)

onMounted(async () => {
  window.addEventListener('pointermove', handlePointerMove)
  window.addEventListener('pointerup', handlePointerUp)
  await bootstrap()
})

onBeforeUnmount(() => {
  window.removeEventListener('pointermove', handlePointerMove)
  window.removeEventListener('pointerup', handlePointerUp)
  if (playbackTimerId !== 0) {
    window.clearInterval(playbackTimerId)
  }
  if (autosaveTimerId !== 0) {
    window.clearTimeout(autosaveTimerId)
  }
})
</script>

<template>
  <div class="workshop-shell">
    <header class="workshop-topbar">
      <div class="brand-block">
        <div class="eyebrow">Clock Theme Workshop</div>
        <h1>主题工坊主编辑器</h1>
      </div>

      <label class="top-field project-name-field">
        <span>项目名</span>
        <input v-model="scene.meta.name" type="text" placeholder="给这个主题起个名字" />
      </label>

      <div class="top-field matrix-preset-field">
        <span>矩阵</span>
        <div class="segment-row compact">
          <button
            v-for="preset in MATRIX_PRESETS"
            :key="preset.id"
            class="chip-button"
            :class="{ active: matrixPresetId() === preset.id }"
            @click="applyMatrixPreset(preset.id)"
          >
            {{ preset.label }}
          </button>
        </div>
      </div>

      <div class="matrix-custom-grid matrix-size-grid">
        <label class="top-field">
          <span>宽</span>
          <input
            :value="scene.meta.matrix.width"
            type="number"
            min="16"
            max="160"
            @input="applyMatrixSize($event.target.value, scene.meta.matrix.height)"
          />
        </label>

        <label class="top-field">
          <span>高</span>
          <input
            :value="scene.meta.matrix.height"
            type="number"
            min="16"
            max="160"
            @input="applyMatrixSize(scene.meta.matrix.width, $event.target.value)"
          />
        </label>
      </div>

      <div class="frame-grid header-frame-grid">
        <label class="top-field">
          <span>总帧数</span>
          <input
            :value="scene.meta.frameCount"
            type="number"
            min="1"
            max="120"
            @input="setFrameCount($event.target.value)"
          />
        </label>

        <label class="top-field">
          <span>当前帧</span>
          <input
            :value="scene.meta.currentFrame + 1"
            type="number"
            min="1"
            :max="scene.meta.frameCount"
            @input="setCurrentFrame(Number($event.target.value) - 1)"
          />
        </label>

        <label class="top-field">
          <span>帧延时</span>
          <input
            v-model="scene.meta.frameDelay"
            type="number"
            min="40"
            max="2000"
            step="10"
          />
        </label>
      </div>

      <div class="top-field preview-mode-field">
        <span>预演</span>
        <div class="segment-row compact">
          <button
            class="chip-button"
            :class="{ active: scene.meta.previewMode === 'loop' }"
            @click="scene.meta.previewMode = 'loop'"
          >
            常规
          </button>
          <button
            class="chip-button"
            :class="{ active: scene.meta.previewMode === 'minute' }"
            @click="scene.meta.previewMode = 'minute'"
          >
            分钟
          </button>
          <button
            class="chip-button"
            :class="{ active: scene.meta.previewMode === 'hour' }"
            @click="scene.meta.previewMode = 'hour'"
          >
            整点
          </button>
        </div>
      </div>

      <div class="top-actions header-actions">
        <button class="solid-button" @click="togglePlayback">
          {{ scene.meta.isPlaying ? '暂停' : '播放' }}
        </button>
        <button class="ghost-button" @click="importInputRef.click()">导入 JSON</button>
        <button class="ghost-button" @click="exportSceneJson">导出 JSON</button>
        <button class="ghost-button danger" @click="resetScene">重置</button>
        <div class="save-indicator">{{ getAutosaveLabel() }}</div>
      </div>
      <input ref="importInputRef" class="hidden-input" type="file" accept="application/json" @change="importSceneFile" />
    </header>

    <main class="workspace-grid">
      <aside class="sidebar sidebar-left">
        <div class="tabs-row">
          <button
            v-for="tab in leftTabs"
            :key="tab.id"
            class="tab-button"
            :class="{ active: scene.editorState.leftTab === tab.id }"
            @click="selectLeftTab(tab.id)"
          >
            {{ tab.label }}
          </button>
        </div>

        <section v-if="scene.editorState.leftTab === 'library'" class="panel-stack">
          <div class="panel-section">
            <div class="section-head">
              <strong>镜像像素素材库</strong>
              <span>{{ libraryState.loading ? '加载中' : currentCollection?.label || '未接入' }}</span>
            </div>

            <label class="control-group">
              <span>素材集</span>
              <select v-model="scene.editorState.libraryCollectionId">
                <option
                  v-for="collection in libraryState.manifest?.collections || []"
                  :key="collection.id"
                  :value="collection.id"
                >
                  {{ collection.label }}
                </option>
              </select>
            </label>

            <label class="control-group">
              <span>素材源</span>
              <select v-model="scene.editorState.librarySourceAssetId">
                <option
                  v-for="sourceAsset in currentCollectionSourceAssets"
                  :key="sourceAsset.id"
                  :value="sourceAsset.id"
                >
                  {{ sourceAsset.description }}
                </option>
              </select>
            </label>

            <div class="segment-row compact">
              <button
                class="chip-button"
                :class="{ active: scene.editorState.libraryMode === 'entries' }"
                @click="scene.editorState.libraryMode = 'entries'"
              >
                条目
              </button>
              <button
                class="chip-button"
                :class="{ active: scene.editorState.libraryMode === 'groups' }"
                @click="scene.editorState.libraryMode = 'groups'"
              >
                分组
              </button>
            </div>

            <label class="control-group">
              <span>搜索</span>
              <input v-model="scene.editorState.librarySearch" type="text" placeholder="搜 frame、tile、chunk" />
            </label>

            <p v-if="libraryState.error.length > 0" class="note danger-text">{{ libraryState.error }}</p>
          </div>

          <div class="asset-grid">
            <article
              v-for="entry in scene.editorState.libraryMode === 'entries' ? filteredEntries : filteredGroups"
              :key="entry.id"
              class="asset-card"
            >
              <img class="asset-thumb" :src="itemThumb(entry)" :alt="entry.id" />
              <div class="asset-card-meta">
                <strong>{{ entry.label || entry.id }}</strong>
                <span v-if="entry.entryIds">{{ entry.entryIds.length }} 帧</span>
                <span v-else>{{ entry.width }}×{{ entry.height }}</span>
              </div>
              <div class="asset-card-actions">
                <button
                  class="ghost-button"
                  @click="
                    scene.editorState.libraryMode === 'entries'
                      ? addEntryAsBackground(entry.id)
                      : addGroupAsBackground(entry.id)
                  "
                >
                  建背景
                </button>
                <button
                  class="ghost-button"
                  @click="
                    scene.editorState.libraryMode === 'entries'
                      ? addEntryAsCharacter(entry.id)
                      : addGroupAsCharacter(entry.id)
                  "
                >
                  新角色组
                </button>
                <button
                  class="ghost-button"
                  :disabled="selectedCharacterGroup === null"
                  @click="
                    scene.editorState.libraryMode === 'entries'
                      ? appendEntryToCurrentGroup(entry.id)
                      : appendGroupToCurrentGroup(entry.id)
                  "
                >
                  加到当前组
                </button>
              </div>
            </article>
          </div>

          <div class="panel-section">
            <div class="section-head">
              <strong>图片上传</strong>
              <span>{{ scene.editorState.uploadTarget === 'background' ? '背景层' : '角色组' }}</span>
            </div>

            <div class="segment-row compact">
              <button
                class="chip-button"
                :class="{ active: scene.editorState.uploadTarget === 'background' }"
                @click="scene.editorState.uploadTarget = 'background'"
              >
                作为背景
              </button>
              <button
                class="chip-button"
                :class="{ active: scene.editorState.uploadTarget === 'character' }"
                @click="scene.editorState.uploadTarget = 'character'"
              >
                作为角色
              </button>
            </div>

            <label class="toggle-line">
              <input v-model="scene.editorState.uploadChromaKey" type="checkbox" />
              <span>青绿色抠底</span>
            </label>

            <button class="solid-button full-button" @click="uploadInputRef.click()">上传图片并直接入场景</button>
            <input
              ref="uploadInputRef"
              class="hidden-input"
              type="file"
              multiple
              accept="image/png,image/jpeg,image/webp"
              @change="handleImageUpload"
            />
          </div>

          <div class="panel-section">
            <div class="section-head">
              <strong>上传素材</strong>
              <span>{{ uploadedAssets.length }}</span>
            </div>

            <div class="asset-grid upload-grid">
              <article v-for="asset in uploadedAssets" :key="asset.id" class="asset-card compact-card">
                <img class="asset-thumb" :src="asset.previewUrl" :alt="asset.label" />
                <div class="asset-card-meta">
                  <strong>{{ asset.label }}</strong>
                  <span>{{ asset.width }}×{{ asset.height }}</span>
                </div>
                <div class="asset-card-actions">
                  <button class="ghost-button" @click="handleUploadedAssetAction(asset, 'background')">建背景</button>
                  <button class="ghost-button" @click="handleUploadedAssetAction(asset, 'character')">新角色组</button>
                  <button
                    class="ghost-button"
                    :disabled="selectedCharacterGroup === null"
                    @click="handleUploadedAssetAction(asset, 'append')"
                  >
                    加到当前组
                  </button>
                </div>
              </article>
            </div>
          </div>
        </section>

        <section v-else class="panel-stack">
          <div class="panel-section">
            <div class="section-head">
              <strong>背景轨道</strong>
              <span>{{ scene.backgroundLayers.length }}</span>
            </div>
            <button
              v-for="layer in scene.backgroundLayers"
              :key="layer.id"
              class="track-row"
              :class="{ active: scene.editorState.selectedType === 'background' && scene.editorState.selectedId === layer.id }"
              @click="selectEntity('background', layer.id)"
            >
              <div>
                <strong>{{ layer.name }}</strong>
                <small>{{ projectAssetLabel(layer.projectAssetId) }}</small>
              </div>
              <span>{{ layer.baseColor }}</span>
            </button>
          </div>

          <div class="panel-section">
            <div class="section-head">
              <strong>角色组</strong>
              <span>{{ scene.characterGroups.length }}</span>
            </div>
            <button
              v-for="group in scene.characterGroups"
              :key="group.id"
              class="track-row"
              :class="{ active: scene.editorState.selectedType === 'character' && scene.editorState.selectedId === group.id }"
              @click="selectEntity('character', group.id)"
            >
              <div>
                <strong>{{ group.name }}</strong>
                <small>{{ group.members.length }} 成员</small>
              </div>
              <span>{{ sampleStepKeys(group.stateKeys, scene.meta.currentFrame, { visible: true }).visible ? '显示' : '隐藏' }}</span>
            </button>
          </div>

          <div class="panel-section">
            <div class="section-head">
              <strong>时间层</strong>
              <span>{{ scene.timeWidgets.length }}</span>
            </div>
            <button
              v-for="widget in scene.timeWidgets"
              :key="widget.id"
              class="track-row"
              :class="{ active: scene.editorState.selectedType === 'time' && scene.editorState.selectedId === widget.id }"
              @click="selectEntity('time', widget.id)"
            >
              <div>
                <strong>{{ widget.label }}</strong>
                <small>{{ previewTimeLabel(widget) }}</small>
              </div>
              <span>{{ widget.visible ? '显示' : '隐藏' }}</span>
            </button>
          </div>
        </section>
      </aside>

      <section class="stage-column">
        <div class="stage-toolbar">
          <div class="stage-summary">
            <strong>{{ timelineLabel() }}</strong>
            <span>{{ matrixWidth }}×{{ matrixHeight }}</span>
            <span>{{ scene.meta.previewMode === 'loop' ? '常规循环' : scene.meta.previewMode === 'minute' ? '分钟预演' : '整点预演' }}</span>
          </div>
          <div class="stage-controls">
            <button class="ghost-button" @click="stepFrame(-1)">上一帧</button>
            <button class="solid-button" @click="togglePlayback">
              {{ scene.meta.isPlaying ? '暂停' : '播放' }}
            </button>
            <button class="ghost-button" @click="stepFrame(1)">下一帧</button>
          </div>
        </div>

        <div class="stage-workspace">
          <div ref="stageViewportRef" class="stage-viewport">
            <div
              class="stage-frame"
              :style="{ '--stage-aspect': String(matrixWidth / matrixHeight) }"
            >
              <canvas ref="stageCanvasRef" class="stage-canvas"></canvas>
              <svg
                class="stage-overlay"
                :viewBox="`0 0 ${matrixWidth} ${matrixHeight}`"
                preserveAspectRatio="none"
              >
                <template v-for="group in stageSnapshot.characters" :key="group.id">
                  <rect
                    v-if="
                      group.bounds &&
                      scene.editorState.selectedType === 'character' &&
                      scene.editorState.selectedId === group.id
                    "
                    class="overlay-box active"
                    :x="group.bounds.x"
                    :y="group.bounds.y"
                    :width="group.bounds.width"
                    :height="group.bounds.height"
                    @pointerdown.stop="beginDrag('character', group.id, $event)"
                  />
                </template>

                <template v-for="widget in stageSnapshot.widgets" :key="widget.id">
                  <rect
                    v-if="
                      scene.editorState.selectedType === 'time' &&
                      scene.editorState.selectedId === widget.id
                    "
                    class="overlay-box widget-box active"
                    :class="{ muted: widget.visible !== true }"
                    :x="widget.bounds.x"
                    :y="widget.bounds.y"
                    :width="widget.bounds.width"
                    :height="widget.bounds.height"
                    @pointerdown.stop="beginDrag('widget', widget.id, $event)"
                  />
                </template>
              </svg>
            </div>
          </div>

          <div class="timeline-panel">
            <div class="timeline-meta">
              <strong>关键帧时间轴</strong>
              <div class="timeline-actions">
                <button class="ghost-button" @click="seedSelectedFrameKeys">写入当前帧</button>
                <button class="ghost-button" @click="clearSelectedFrameKeys">清掉当前帧</button>
              </div>
            </div>

            <div class="timeline-strip">
              <button
                v-for="frameIndex in frameIndices"
                :key="frameIndex"
                class="frame-chip"
                :class="{ active: frameIndex === scene.meta.currentFrame }"
                @click="setCurrentFrame(frameIndex)"
              >
                <span>{{ frameIndex + 1 }}</span>
                <div class="frame-dots">
                  <i class="frame-dot" :class="{ active: visibleFrameDots(frameIndex).transform }"></i>
                  <i class="frame-dot state" :class="{ active: visibleFrameDots(frameIndex).state }"></i>
                </div>
              </button>
            </div>

            <div class="timeline-note">
              <span>当前帧变换键：{{ selectedFrameMarkerInfo.transform ? '有' : '无' }}</span>
              <span>当前帧状态键：{{ selectedFrameMarkerInfo.state ? '有' : '无' }}</span>
              <span>拖动画布上的角色组或时间框，会把位置写进当前帧。</span>
            </div>
          </div>
        </div>
      </section>

      <aside class="sidebar sidebar-right">
        <div class="tabs-row">
          <button
            v-for="tab in rightTabs"
            :key="tab.id"
            class="tab-button"
            :class="{ active: scene.editorState.rightTab === tab.id }"
            @click="selectRightTab(tab.id)"
          >
            {{ tab.label }}
          </button>
        </div>

        <section v-if="scene.editorState.rightTab === 'scene'" class="panel-stack">
          <div class="panel-section">
            <div class="section-head">
              <strong>场景总览</strong>
              <span>{{ scene.assets.length }} 项素材</span>
            </div>
            <div class="metric-grid">
              <div>
                <span>背景层</span>
                <strong>{{ scene.backgroundLayers.length }}</strong>
              </div>
              <div>
                <span>角色组</span>
                <strong>{{ scene.characterGroups.length }}</strong>
              </div>
              <div>
                <span>时间组件</span>
                <strong>{{ scene.timeWidgets.length }}</strong>
              </div>
              <div>
                <span>自动保存</span>
                <strong>{{ getAutosaveLabel() }}</strong>
              </div>
            </div>

            <label class="control-group">
              <span>预演日期</span>
              <input v-model="scene.meta.previewDate" type="date" />
            </label>

            <label class="control-group">
              <span>预演时间</span>
              <input v-model="scene.meta.previewTime" type="time" step="1" />
            </label>

            <p class="note">
              时间层独立于帧外。你在这里切换日期和时间，只会影响时间组件显示，不会改背景层和角色组关键帧。
            </p>
          </div>
        </section>

        <section v-else-if="scene.editorState.rightTab === 'background'" class="panel-stack">
          <div v-if="selectedBackground && selectedBackgroundView" class="panel-section">
            <div class="section-head">
              <strong>背景层</strong>
              <span>{{ selectedBackground.name }}</span>
            </div>

            <label class="control-group">
              <span>名称</span>
              <input v-model="selectedBackground.name" type="text" />
            </label>

            <label class="control-group">
              <span>绑定素材</span>
              <select
                :value="selectedBackground.projectAssetId"
                @change="selectedBackground.projectAssetId = $event.target.value"
              >
                <option value="">只用纯色底板</option>
                <option v-for="asset in backgroundAssetOptions" :key="asset.id" :value="asset.id">
                  {{ asset.label }}
                </option>
              </select>
            </label>

            <label class="control-group">
              <span>底色</span>
              <input v-model="selectedBackground.baseColor" type="color" />
            </label>

            <div class="segment-row compact">
              <button
                class="chip-button"
                :class="{ active: selectedBackgroundView.state.behavior === 'fixed' }"
                @click="updateBackgroundState({ behavior: 'fixed' })"
              >
                固定
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedBackgroundView.state.behavior === 'loop' }"
                @click="updateBackgroundState({ behavior: 'loop' })"
              >
                循环
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedBackgroundView.state.behavior === 'blink' }"
                @click="updateBackgroundState({ behavior: 'blink' })"
              >
                闪烁
              </button>
            </div>

            <label class="toggle-line">
              <input
                :checked="selectedBackgroundView.state.visible"
                type="checkbox"
                @change="updateBackgroundState({ visible: $event.target.checked })"
              />
              <span>当前帧可见</span>
            </label>

            <div class="number-grid">
              <label class="control-group">
                <span>X</span>
                <input
                  :value="Math.round(selectedBackgroundView.transform.x)"
                  type="number"
                  @input="updateBackgroundTransform({ x: Number($event.target.value) })"
                />
              </label>
              <label class="control-group">
                <span>Y</span>
                <input
                  :value="Math.round(selectedBackgroundView.transform.y)"
                  type="number"
                  @input="updateBackgroundTransform({ y: Number($event.target.value) })"
                />
              </label>
              <label class="control-group">
                <span>宽</span>
                <input
                  :value="Math.round(selectedBackgroundView.transform.width)"
                  type="number"
                  min="1"
                  @input="updateBackgroundTransform({ width: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>高</span>
                <input
                  :value="Math.round(selectedBackgroundView.transform.height)"
                  type="number"
                  min="1"
                  @input="updateBackgroundTransform({ height: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>透明度</span>
                <input
                  :value="selectedBackgroundView.transform.opacity.toFixed(2)"
                  type="number"
                  min="0"
                  max="1"
                  step="0.05"
                  @input="updateBackgroundTransform({ opacity: clamp(Number($event.target.value), 0, 1) })"
                />
              </label>
              <label class="control-group">
                <span>序列步长</span>
                <input
                  :value="selectedBackgroundView.state.sequenceStepFrames"
                  type="number"
                  min="1"
                  max="24"
                  @input="updateBackgroundState({ sequenceStepFrames: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>闪烁节奏</span>
                <input
                  :value="selectedBackgroundView.state.rhythmFrames"
                  type="number"
                  min="1"
                  max="24"
                  @input="updateBackgroundState({ rhythmFrames: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>速度 X</span>
                <input
                  :value="selectedBackgroundView.state.speedX"
                  type="number"
                  min="-16"
                  max="16"
                  @input="updateBackgroundState({ speedX: Number($event.target.value) })"
                />
              </label>
              <label class="control-group">
                <span>速度 Y</span>
                <input
                  :value="selectedBackgroundView.state.speedY"
                  type="number"
                  min="-16"
                  max="16"
                  @input="updateBackgroundState({ speedY: Number($event.target.value) })"
                />
              </label>
            </div>

            <div class="action-row">
              <button class="ghost-button" @click="fitSelectedBackground">适配矩阵</button>
              <button class="ghost-button danger" @click="removeSelectedBackground">删除背景层</button>
            </div>
          </div>

          <div v-else class="empty-panel">先从左侧轨道里选中一个背景层。</div>
        </section>

        <section v-else-if="scene.editorState.rightTab === 'character'" class="panel-stack">
          <div v-if="selectedCharacterGroup && selectedCharacterView" class="panel-section">
            <div class="section-head">
              <strong>角色组</strong>
              <span>{{ selectedCharacterGroup.name }}</span>
            </div>

            <label class="control-group">
              <span>名称</span>
              <input v-model="selectedCharacterGroup.name" type="text" />
            </label>

            <div class="segment-row compact">
              <button
                class="chip-button"
                :class="{ active: selectedCharacterView.state.behavior === 'fixed' }"
                @click="updateGroupState({ behavior: 'fixed' })"
              >
                固定
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedCharacterView.state.behavior === 'loop' }"
                @click="updateGroupState({ behavior: 'loop' })"
              >
                循环动作
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedCharacterView.state.behavior === 'blink' }"
                @click="updateGroupState({ behavior: 'blink' })"
              >
                闪烁
              </button>
            </div>

            <label class="toggle-line">
              <input
                :checked="selectedCharacterView.state.visible"
                type="checkbox"
                @change="updateGroupState({ visible: $event.target.checked })"
              />
              <span>当前帧可见</span>
            </label>

            <div class="number-grid">
              <label class="control-group">
                <span>X</span>
                <input
                  :value="Math.round(selectedCharacterView.transform.x)"
                  type="number"
                  @input="updateGroupTransform({ x: Number($event.target.value) })"
                />
              </label>
              <label class="control-group">
                <span>Y</span>
                <input
                  :value="Math.round(selectedCharacterView.transform.y)"
                  type="number"
                  @input="updateGroupTransform({ y: Number($event.target.value) })"
                />
              </label>
              <label class="control-group">
                <span>整体缩放</span>
                <input
                  :value="selectedCharacterView.transform.scale.toFixed(2)"
                  type="number"
                  min="0.2"
                  max="8"
                  step="0.05"
                  @input="updateGroupTransform({ scale: Math.max(0.2, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>透明度</span>
                <input
                  :value="selectedCharacterView.transform.opacity.toFixed(2)"
                  type="number"
                  min="0"
                  max="1"
                  step="0.05"
                  @input="updateGroupTransform({ opacity: clamp(Number($event.target.value), 0, 1) })"
                />
              </label>
              <label class="control-group">
                <span>序列步长</span>
                <input
                  :value="selectedCharacterView.state.sequenceStepFrames"
                  type="number"
                  min="1"
                  max="24"
                  @input="updateGroupState({ sequenceStepFrames: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>闪烁节奏</span>
                <input
                  :value="selectedCharacterView.state.rhythmFrames"
                  type="number"
                  min="1"
                  max="24"
                  @input="updateGroupState({ rhythmFrames: Math.max(1, Number($event.target.value)) })"
                />
              </label>
            </div>

            <div class="action-row">
              <button class="ghost-button" @click="centerSelectedGroup">组合居中</button>
              <button class="ghost-button" @click="addSelectedLibraryAssetToCurrentGroup">把当前库素材加进来</button>
              <button class="ghost-button danger" @click="removeSelectedGroup">删除角色组</button>
            </div>
          </div>

          <div v-if="selectedCharacterGroup" class="panel-section">
            <div class="section-head">
              <strong>组成员</strong>
              <span>{{ selectedCharacterGroup.members.length }}</span>
            </div>

            <div class="member-list">
              <article v-for="member in selectedCharacterGroup.members" :key="member.id" class="member-card">
                <div class="member-head">
                  <strong>{{ member.name }}</strong>
                  <label class="toggle-line mini">
                    <input v-model="member.visible" type="checkbox" />
                    <span>显示</span>
                  </label>
                </div>
                <label class="control-group">
                  <span>素材</span>
                  <select :value="member.projectAssetId" @change="updateMemberAsset(member.id, $event.target.value)">
                    <option v-for="asset in characterAssetOptions" :key="asset.id" :value="asset.id">
                      {{ asset.label }}
                    </option>
                  </select>
                </label>

                <div class="number-grid tight-grid">
                  <label class="control-group">
                    <span>偏移 X</span>
                    <input
                      :value="member.offsetX"
                      type="number"
                      @input="member.offsetX = Number($event.target.value)"
                    />
                  </label>
                  <label class="control-group">
                    <span>偏移 Y</span>
                    <input
                      :value="member.offsetY"
                      type="number"
                      @input="member.offsetY = Number($event.target.value)"
                    />
                  </label>
                  <label class="control-group">
                    <span>宽</span>
                    <input
                      :value="member.width"
                      type="number"
                      min="1"
                      @input="member.width = Math.max(1, Number($event.target.value))"
                    />
                  </label>
                  <label class="control-group">
                    <span>高</span>
                    <input
                      :value="member.height"
                      type="number"
                      min="1"
                      @input="member.height = Math.max(1, Number($event.target.value))"
                    />
                  </label>
                </div>

                <div class="action-row compact-row">
                  <button class="ghost-button" @click="nudgeMember(member.id, 'x', -1)">X-1</button>
                  <button class="ghost-button" @click="nudgeMember(member.id, 'x', 1)">X+1</button>
                  <button class="ghost-button" @click="nudgeMember(member.id, 'y', -1)">Y-1</button>
                  <button class="ghost-button" @click="nudgeMember(member.id, 'y', 1)">Y+1</button>
                  <button class="ghost-button danger" @click="removeMember(member.id)">删除成员</button>
                </div>
              </article>
            </div>
          </div>

          <div v-if="selectedCharacterGroup === null" class="empty-panel">先在左侧轨道里选中一个角色组。</div>
        </section>

        <section v-else class="panel-stack">
          <div class="panel-section">
            <div class="section-head">
              <strong>时间组件</strong>
              <span>{{ selectedTimeWidget?.label || '未选中' }}</span>
            </div>

            <div class="segment-row compact">
              <button
                v-for="widget in scene.timeWidgets"
                :key="widget.id"
                class="chip-button"
                :class="{ active: selectedTimeWidget && selectedTimeWidget.id === widget.id }"
                @click="selectEntity('time', widget.id)"
              >
                {{ widget.label }}
              </button>
            </div>
          </div>

          <div v-if="selectedTimeWidget" class="panel-section">
            <label class="toggle-line">
              <input v-model="selectedTimeWidget.visible" type="checkbox" />
              <span>组件可见</span>
            </label>

            <div class="number-grid">
              <label class="control-group">
                <span>X</span>
                <input
                  :value="selectedTimeWidget.x"
                  type="number"
                  @input="updateWidget(selectedTimeWidget.id, { x: clamp(Number($event.target.value), 0, matrixWidth - 1) })"
                />
              </label>
              <label class="control-group">
                <span>Y</span>
                <input
                  :value="selectedTimeWidget.y"
                  type="number"
                  @input="updateWidget(selectedTimeWidget.id, { y: clamp(Number($event.target.value), 0, matrixHeight - 1) })"
                />
              </label>
              <label class="control-group">
                <span>缩放</span>
                <input
                  :value="selectedTimeWidget.scale"
                  type="number"
                  min="1"
                  max="6"
                  @input="updateWidget(selectedTimeWidget.id, { scale: Math.max(1, Number($event.target.value)) })"
                />
              </label>
              <label class="control-group">
                <span>字间距</span>
                <input
                  :value="selectedTimeWidget.letterSpacing"
                  type="number"
                  min="0"
                  max="6"
                  @input="updateWidget(selectedTimeWidget.id, { letterSpacing: Math.max(0, Number($event.target.value)) })"
                />
              </label>
            </div>

            <label class="control-group">
              <span>颜色</span>
              <input
                :value="selectedTimeWidget.color"
                type="color"
                @input="updateWidget(selectedTimeWidget.id, { color: $event.target.value })"
              />
            </label>

            <div v-if="selectedTimeWidget.type === 'time'" class="segment-row compact">
              <button
                class="chip-button"
                :class="{ active: selectedTimeWidget.hourFormat === 24 }"
                @click="updateWidget(selectedTimeWidget.id, { hourFormat: 24 })"
              >
                24h
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedTimeWidget.hourFormat === 12 }"
                @click="updateWidget(selectedTimeWidget.id, { hourFormat: 12 })"
              >
                12h
              </button>
              <button
                class="chip-button"
                :class="{ active: selectedTimeWidget.showSeconds === true }"
                @click="updateWidget(selectedTimeWidget.id, { showSeconds: !selectedTimeWidget.showSeconds })"
              >
                {{ selectedTimeWidget.showSeconds ? '隐藏秒' : '显示秒' }}
              </button>
            </div>

            <p class="note">
              当前文本：{{ previewTimeLabel(selectedTimeWidget) }}。时间层不写入帧关键状态，只跟预演日期和时间联动。
            </p>
          </div>

          <div v-else class="empty-panel">先从左侧轨道里选中一个时间组件。</div>
        </section>
      </aside>
    </main>

    <footer class="status-bar">
      <span>{{ statusMessage }}</span>
      <span>选中：{{ scene.editorState.selectedType }} / {{ scene.editorState.selectedId || 'none' }}</span>
      <span>播放：{{ scene.meta.isPlaying ? '中' : '暂停' }}</span>
      <span>自动保存：{{ getAutosaveLabel() }}</span>
    </footer>
  </div>
</template>

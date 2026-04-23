export const SCENE_VERSION = 'ThemeWorkshopSceneV1'
export const MATRIX_PRESETS = [
  { id: '32', label: '32×32', width: 32, height: 32 },
  { id: '64', label: '64×64', width: 64, height: 64 },
  { id: '96x64', label: '96×64', width: 96, height: 64 },
  { id: 'custom', label: '自定义', width: 64, height: 64 },
]

export function uid(prefix) {
  return `${prefix}-${Date.now()}-${Math.random().toString(16).slice(2, 8)}`
}

export function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value))
}

export function deepClone(value) {
  return JSON.parse(JSON.stringify(value))
}

export function fitRect(sourceWidth, sourceHeight, targetWidth, targetHeight) {
  if (sourceWidth <= 0 || sourceHeight <= 0) {
    return {
      x: 0,
      y: 0,
      width: targetWidth,
      height: targetHeight,
    }
  }

  const scale = Math.min(targetWidth / sourceWidth, targetHeight / sourceHeight)
  const width = Math.max(1, Math.round(sourceWidth * scale))
  const height = Math.max(1, Math.round(sourceHeight * scale))

  return {
    x: Math.floor((targetWidth - width) / 2),
    y: Math.floor((targetHeight - height) / 2),
    width,
    height,
  }
}

export function getAssetFrameSize(projectAsset) {
  if (projectAsset === null || typeof projectAsset !== 'object') {
    return { width: 12, height: 12 }
  }

  if (projectAsset.kind === 'uploaded-image') {
    return { width: projectAsset.width, height: projectAsset.height }
  }

  if (Array.isArray(projectAsset.frames) && projectAsset.frames.length > 0) {
    return {
      width: projectAsset.frames[0].width,
      height: projectAsset.frames[0].height,
    }
  }

  return { width: 12, height: 12 }
}

export function createBaseBackgroundLayer(matrix) {
  return {
    id: uid('bg'),
    name: 'Base Wash',
    projectAssetId: '',
    baseColor: '#071316',
    transformKeys: [
      {
        frame: 0,
        x: 0,
        y: 0,
        width: matrix.width,
        height: matrix.height,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: 'fixed',
        rhythmFrames: 2,
        speedX: 0,
        speedY: 0,
        sequenceStepFrames: 1,
      },
    ],
  }
}

export function createBackgroundLayer(projectAsset, matrix) {
  const assetSize = getAssetFrameSize(projectAsset)
  const fitted = fitRect(assetSize.width, assetSize.height, matrix.width, matrix.height)
  const hasSequence = projectAsset !== null && projectAsset.kind === 'catalog-sequence'

  return {
    id: uid('bg'),
    name: projectAsset !== null ? `${projectAsset.label} Background` : 'Background Layer',
    projectAssetId: projectAsset !== null ? projectAsset.id : '',
    baseColor: '#071316',
    transformKeys: [
      {
        frame: 0,
        x: fitted.x,
        y: fitted.y,
        width: fitted.width,
        height: fitted.height,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: hasSequence ? 'loop' : 'fixed',
        rhythmFrames: 2,
        speedX: 0,
        speedY: 0,
        sequenceStepFrames: 1,
      },
    ],
  }
}

export function createCharacterGroup(projectAsset, matrix) {
  const assetSize = getAssetFrameSize(projectAsset)
  const rect = fitRect(assetSize.width, assetSize.height, Math.floor(matrix.width * 0.45), Math.floor(matrix.height * 0.45))
  const centeredX = Math.floor((matrix.width - rect.width) / 2)
  const centeredY = Math.floor((matrix.height - rect.height) / 2)

  return {
    id: uid('group'),
    name: projectAsset !== null ? projectAsset.label : 'Character Group',
    transformKeys: [
      {
        frame: 0,
        x: centeredX,
        y: centeredY,
        scale: 1,
        opacity: 1,
      },
    ],
    stateKeys: [
      {
        frame: 0,
        visible: true,
        behavior: 'fixed',
        rhythmFrames: 2,
        sequenceStepFrames: 1,
      },
    ],
    members: projectAsset !== null ? [createGroupMember(projectAsset)] : [],
  }
}

export function createGroupMember(projectAsset) {
  const size = getAssetFrameSize(projectAsset)

  return {
    id: uid('member'),
    name: projectAsset.label,
    projectAssetId: projectAsset.id,
    offsetX: 0,
    offsetY: 0,
    width: size.width,
    height: size.height,
    opacity: 1,
    visible: true,
  }
}

export function createTimeWidgets(matrix) {
  const compact = matrix.width <= 32 || matrix.height <= 32
  const timeScale = compact ? 1 : 2
  const dateScale = 1
  const lowerY = Math.max(0, matrix.height - (compact ? 8 : 10))

  return [
    {
      id: 'time',
      type: 'time',
      label: 'TIME',
      visible: true,
      x: compact ? 2 : 4,
      y: compact ? 3 : 6,
      scale: timeScale,
      letterSpacing: 0,
      color: '#ffffff',
      hourFormat: 24,
      showSeconds: false,
    },
    {
      id: 'date',
      type: 'date',
      label: 'DATE',
      visible: true,
      x: compact ? 2 : 4,
      y: lowerY,
      scale: dateScale,
      letterSpacing: 0,
      color: '#8df4d8',
      hourFormat: 24,
      showSeconds: false,
    },
    {
      id: 'week',
      type: 'week',
      label: 'WEEK',
      visible: true,
      x: compact ? Math.max(0, matrix.width - 12) : Math.max(0, matrix.width - 18),
      y: lowerY,
      scale: dateScale,
      letterSpacing: 0,
      color: '#f4c86e',
      hourFormat: 24,
      showSeconds: false,
    },
    {
      id: 'year',
      type: 'year',
      label: 'YEAR',
      visible: false,
      x: compact ? 2 : 4,
      y: Math.max(0, lowerY - 8),
      scale: 1,
      letterSpacing: 0,
      color: '#d0d9e4',
      hourFormat: 24,
      showSeconds: false,
    },
  ]
}

export function createDefaultScene() {
  const matrix = {
    width: 64,
    height: 64,
  }

  return {
    version: SCENE_VERSION,
    meta: {
      name: 'Theme Workshop Scene',
      matrix,
      frameCount: 12,
      currentFrame: 0,
      frameDelay: 160,
      previewMode: 'loop',
      isPlaying: false,
      previewDate: '2026-04-21',
      previewTime: '12:34:56',
      autosavedAt: '',
    },
    assets: [],
    backgroundLayers: [createBaseBackgroundLayer(matrix)],
    characterGroups: [],
    timeWidgets: createTimeWidgets(matrix),
    editorState: {
      leftTab: 'library',
      rightTab: 'scene',
      selectedType: 'background',
      selectedId: '',
      libraryCollectionId: 'adventure-island',
      librarySourceAssetId: '',
      libraryMode: 'entries',
      librarySearch: '',
      uploadTarget: 'character',
      uploadChromaKey: true,
      selectedMemberId: '',
    },
  }
}

export function ensureSceneShape(raw) {
  if (typeof raw !== 'object' || raw === null) {
    throw new Error('场景 JSON 必须是对象')
  }
  if (raw.version !== SCENE_VERSION) {
    throw new Error(`只支持 ${SCENE_VERSION}，当前文件不是新编辑器场景格式`)
  }
  if (typeof raw.meta !== 'object' || raw.meta === null) {
    throw new Error('场景缺少 meta')
  }
  if (typeof raw.meta.matrix !== 'object' || raw.meta.matrix === null) {
    throw new Error('场景缺少矩阵尺寸信息')
  }
  if (Array.isArray(raw.assets) === false) {
    throw new Error('场景缺少 assets 数组')
  }
  if (Array.isArray(raw.backgroundLayers) === false) {
    throw new Error('场景缺少 backgroundLayers 数组')
  }
  if (Array.isArray(raw.characterGroups) === false) {
    throw new Error('场景缺少 characterGroups 数组')
  }
  if (Array.isArray(raw.timeWidgets) === false) {
    throw new Error('场景缺少 timeWidgets 数组')
  }
  if (typeof raw.editorState !== 'object' || raw.editorState === null) {
    throw new Error('场景缺少 editorState')
  }

  return raw
}

export function sortKeys(keys) {
  return keys.slice().sort((first, second) => first.frame - second.frame)
}

export function hasKeyframe(keys, frame) {
  return keys.some((key) => key.frame === frame)
}

export function upsertKeyframe(keys, frame, patch) {
  const next = []
  let found = false

  for (let index = 0; index < keys.length; index += 1) {
    if (keys[index].frame === frame) {
      next.push({ ...keys[index], ...patch, frame })
      found = true
    } else {
      next.push(keys[index])
    }
  }

  if (found === false) {
    next.push({ frame, ...patch })
  }

  return sortKeys(next)
}

export function removeKeyframe(keys, frame) {
  return sortKeys(keys.filter((key) => key.frame !== frame))
}

function interpolateValue(previousValue, nextValue, ratio) {
  if (typeof previousValue === 'number' && typeof nextValue === 'number') {
    return previousValue + (nextValue - previousValue) * ratio
  }
  return previousValue
}

export function sampleContinuousKeys(keys, frame, defaults) {
  if (keys.length === 0) {
    return { ...defaults }
  }

  const ordered = sortKeys(keys)
  const exact = ordered.find((key) => key.frame === frame)
  if (typeof exact === 'object' && exact !== null) {
    return { ...defaults, ...exact }
  }

  let previous = null
  let next = null

  for (let index = 0; index < ordered.length; index += 1) {
    const key = ordered[index]
    if (key.frame < frame) {
      previous = key
      continue
    }
    if (key.frame > frame) {
      next = key
      break
    }
  }

  if (previous === null) {
    return { ...defaults, ...ordered[0] }
  }
  if (next === null) {
    return { ...defaults, ...previous }
  }

  const ratio = (frame - previous.frame) / (next.frame - previous.frame)
  const sampled = { ...defaults }
  const keysToSample = Object.keys({ ...defaults, ...previous, ...next })

  for (let index = 0; index < keysToSample.length; index += 1) {
    const key = keysToSample[index]
    if (key === 'frame') {
      continue
    }
    const previousValue =
      typeof previous[key] === 'undefined'
        ? typeof defaults[key] === 'undefined'
          ? next[key]
          : defaults[key]
        : previous[key]
    const nextValue =
      typeof next[key] === 'undefined'
        ? typeof defaults[key] === 'undefined'
          ? previous[key]
          : defaults[key]
        : next[key]
    sampled[key] = interpolateValue(previousValue, nextValue, ratio)
  }

  return sampled
}

export function sampleStepKeys(keys, frame, defaults) {
  if (keys.length === 0) {
    return { ...defaults }
  }

  const ordered = sortKeys(keys)
  let chosen = ordered[0]

  for (let index = 0; index < ordered.length; index += 1) {
    if (ordered[index].frame <= frame) {
      chosen = ordered[index]
    } else {
      break
    }
  }

  return {
    ...defaults,
    ...chosen,
  }
}

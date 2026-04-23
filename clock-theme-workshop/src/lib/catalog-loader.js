import { uid } from './scene-model'

function getCollectionBasePath(manifestPath) {
  const segments = manifestPath.split('/')
  if (segments.length <= 2) {
    return '/'
  }
  return `${segments.slice(0, -2).join('/')}/`
}

function getCollectionInfo(rawAsset) {
  if (Array.isArray(rawAsset.frames)) {
    return { key: 'frames', orderKey: 'frameOrder' }
  }
  if (Array.isArray(rawAsset.components)) {
    return { key: 'components', orderKey: 'componentOrder' }
  }
  if (Array.isArray(rawAsset.tiles)) {
    return { key: 'tiles', orderKey: 'tileOrder' }
  }
  if (Array.isArray(rawAsset.chunks)) {
    return { key: 'chunks', orderKey: 'chunkOrder' }
  }
  throw new Error(`无法识别素材结构: ${rawAsset.assetId}`)
}

export function buildThumbnailDataUrl(pixelFrame, thumbWidth = 108, thumbHeight = 76) {
  const canvas = document.createElement('canvas')
  canvas.width = thumbWidth
  canvas.height = thumbHeight
  const context = canvas.getContext('2d')
  context.imageSmoothingEnabled = false
  context.fillStyle = '#020508'
  context.fillRect(0, 0, thumbWidth, thumbHeight)

  const sourceCanvas = document.createElement('canvas')
  sourceCanvas.width = pixelFrame.width
  sourceCanvas.height = pixelFrame.height
  const sourceContext = sourceCanvas.getContext('2d')
  const imageData = sourceContext.createImageData(pixelFrame.width, pixelFrame.height)

  for (let index = 0; index < pixelFrame.pixels.length; index += 1) {
    const pixel = pixelFrame.pixels[index]
    const pixelIndex = (pixel[1] * pixelFrame.width + pixel[0]) * 4
    imageData.data[pixelIndex] = pixel[2]
    imageData.data[pixelIndex + 1] = pixel[3]
    imageData.data[pixelIndex + 2] = pixel[4]
    imageData.data[pixelIndex + 3] = 255
  }

  sourceContext.putImageData(imageData, 0, 0)

  const scale = Math.min(thumbWidth / pixelFrame.width, thumbHeight / pixelFrame.height)
  const drawWidth = Math.max(1, Math.round(pixelFrame.width * scale))
  const drawHeight = Math.max(1, Math.round(pixelFrame.height * scale))
  const drawX = Math.floor((thumbWidth - drawWidth) / 2)
  const drawY = Math.floor((thumbHeight - drawHeight) / 2)
  context.drawImage(sourceCanvas, drawX, drawY, drawWidth, drawHeight)

  return canvas.toDataURL('image/png')
}

function normalizeSourceAsset(collection, descriptor, rawAsset) {
  const collectionInfo = getCollectionInfo(rawAsset)
  const items = rawAsset[collectionInfo.key]
  const orderedIds = Array.isArray(rawAsset[collectionInfo.orderKey])
    ? rawAsset[collectionInfo.orderKey].slice()
    : items.map((item) => item.id)

  const entries = items.map((item) => ({
    id: item.id,
    label: item.id,
    width: item.size.width,
    height: item.size.height,
    pixels: item.pixels,
  }))

  const entryMap = {}
  for (let index = 0; index < entries.length; index += 1) {
    entryMap[entries[index].id] = entries[index]
  }

  const groups = [{ id: 'all', label: '全部条目', entryIds: orderedIds.slice() }]
  const groupSources = []

  if (typeof rawAsset.recommendedAnimations === 'object' && rawAsset.recommendedAnimations !== null) {
    groupSources.push(rawAsset.recommendedAnimations)
  }
  if (typeof rawAsset.recommendedGroups === 'object' && rawAsset.recommendedGroups !== null) {
    groupSources.push(rawAsset.recommendedGroups)
  }

  for (let sourceIndex = 0; sourceIndex < groupSources.length; sourceIndex += 1) {
    const source = groupSources[sourceIndex]
    for (const [groupId, entryIds] of Object.entries(source)) {
      if (Array.isArray(entryIds) === false || entryIds.length === 0) {
        continue
      }
      groups.push({
        id: groupId,
        label: groupId,
        entryIds: entryIds.slice(),
      })
    }
  }

  return {
    id: descriptor.id,
    label: rawAsset.label || descriptor.description,
    description: descriptor.description,
    collectionId: collection.id,
    entries,
    entryMap,
    groups,
    orderedIds,
  }
}

export async function loadAssetCatalog() {
  const response = await fetch('/pixel-catalog/asset-catalog.json')
  if (response.ok === false) {
    throw new Error('读取像素素材目录失败')
  }

  const catalog = await response.json()
  const collections = []

  for (let index = 0; index < catalog.collections.length; index += 1) {
    const collection = catalog.collections[index]
    const manifestResponse = await fetch(collection.manifestPath)
    if (manifestResponse.ok === false) {
      throw new Error(`读取素材集合失败: ${collection.id}`)
    }

    const manifest = await manifestResponse.json()
    const basePath = getCollectionBasePath(collection.manifestPath)
    const sourceAssets = manifest.outputs.map((item) => ({
      id: item.id,
      label: item.id,
      description: item.description,
      jsonPath: `${basePath}${item.path}`,
    }))

    collections.push({
      id: collection.id,
      label: collection.label,
      sourceAssets,
    })
  }

  return {
    version: catalog.version,
    collections,
  }
}

export async function loadCatalogSourceAsset(collection, sourceAsset) {
  const response = await fetch(sourceAsset.jsonPath)
  if (response.ok === false) {
    throw new Error(`读取素材源失败: ${sourceAsset.id}`)
  }
  const payload = await response.json()
  return normalizeSourceAsset(collection, sourceAsset, payload)
}

export function buildProjectAssetFromCatalogEntry(normalizedAsset, entryId, trackHint) {
  const entry = normalizedAsset.entryMap[entryId]
  if (typeof entry === 'undefined') {
    throw new Error(`未找到条目 ${entryId}`)
  }

  return {
    id: uid('asset'),
    kind: 'catalog-entry',
    label: `${normalizedAsset.label} · ${entryId}`,
    trackHint,
    previewUrl: buildThumbnailDataUrl(entry),
    source: {
      collectionId: normalizedAsset.collectionId,
      sourceAssetId: normalizedAsset.id,
      mode: 'entry',
      entryId,
    },
    frames: [
      {
        id: entry.id,
        width: entry.width,
        height: entry.height,
        pixels: entry.pixels,
      },
    ],
  }
}

export function buildProjectAssetFromCatalogGroup(normalizedAsset, groupId, trackHint) {
  const group = normalizedAsset.groups.find((item) => item.id === groupId)
  if (typeof group === 'undefined') {
    throw new Error(`未找到分组 ${groupId}`)
  }

  const frames = group.entryIds
    .map((entryId) => normalizedAsset.entryMap[entryId])
    .filter((entry) => typeof entry === 'object')
    .map((entry) => ({
      id: entry.id,
      width: entry.width,
      height: entry.height,
      pixels: entry.pixels,
    }))

  return {
    id: uid('asset'),
    kind: 'catalog-sequence',
    label: `${normalizedAsset.label} · ${group.label}`,
    trackHint,
    previewUrl: frames.length > 0 ? buildThumbnailDataUrl(frames[0]) : '',
    source: {
      collectionId: normalizedAsset.collectionId,
      sourceAssetId: normalizedAsset.id,
      mode: 'group',
      groupId,
    },
    frames,
  }
}

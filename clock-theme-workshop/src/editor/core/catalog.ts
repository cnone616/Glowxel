import type {
  AssetCatalogManifest,
  AssetCatalogSourceAsset,
  AssetRecord,
  CatalogEntry,
  CatalogFrame,
  CatalogGroup,
  NormalizedCatalogAsset,
} from '../types'
import { uid } from './id'

function getCollectionBasePath(manifestPath: string): string {
  const segments = manifestPath.split('/')
  if (segments.length <= 2) {
    return '/'
  }

  return `${segments.slice(0, -2).join('/')}/`
}

function getCollectionInfo(rawAsset: Record<string, unknown>): { key: string; orderKey: string } {
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

  throw new Error('无法识别素材结构')
}

export function buildThumbnailDataUrl(
  pixelFrame: CatalogFrame,
  thumbWidth = 108,
  thumbHeight = 76,
): string {
  const canvas = document.createElement('canvas')
  canvas.width = thumbWidth
  canvas.height = thumbHeight
  const context = canvas.getContext('2d')

  if (!context) {
    throw new Error('缩略图上下文创建失败')
  }

  context.imageSmoothingEnabled = false
  context.fillStyle = '#020508'
  context.fillRect(0, 0, thumbWidth, thumbHeight)

  const sourceCanvas = document.createElement('canvas')
  sourceCanvas.width = pixelFrame.width
  sourceCanvas.height = pixelFrame.height
  const sourceContext = sourceCanvas.getContext('2d')

  if (!sourceContext) {
    throw new Error('像素缩略图上下文创建失败')
  }

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

function normalizeSourceAsset(
  collectionId: string,
  descriptor: AssetCatalogSourceAsset,
  rawAsset: Record<string, unknown>,
): NormalizedCatalogAsset {
  const collectionInfo = getCollectionInfo(rawAsset)
  const items = rawAsset[collectionInfo.key] as Array<Record<string, unknown>>
  const orderedIds = Array.isArray(rawAsset[collectionInfo.orderKey])
    ? (rawAsset[collectionInfo.orderKey] as string[]).slice()
    : items.map((item) => String(item.id))

  const entries: CatalogEntry[] = items.map((item) => ({
    id: String(item.id),
    label: String(item.id),
    width: Number((item.size as { width: number }).width),
    height: Number((item.size as { height: number }).height),
    pixels: item.pixels as number[][],
  }))

  const entryMap: Record<string, CatalogEntry> = {}
  for (const entry of entries) {
    entryMap[entry.id] = entry
  }

  const groups: CatalogGroup[] = [{ id: 'all', label: '全部条目', entryIds: orderedIds }]
  const groupSources: Array<Record<string, unknown>> = []

  if (typeof rawAsset.recommendedAnimations === 'object' && rawAsset.recommendedAnimations !== null) {
    groupSources.push(rawAsset.recommendedAnimations as Record<string, unknown>)
  }

  if (typeof rawAsset.recommendedGroups === 'object' && rawAsset.recommendedGroups !== null) {
    groupSources.push(rawAsset.recommendedGroups as Record<string, unknown>)
  }

  for (const source of groupSources) {
    for (const [groupId, entryIds] of Object.entries(source)) {
      if (!Array.isArray(entryIds) || entryIds.length === 0) {
        continue
      }
      groups.push({
        id: groupId,
        label: groupId,
        entryIds: entryIds.map((entryId) => String(entryId)),
      })
    }
  }

  return {
    id: descriptor.id,
    label: String(rawAsset.label ?? descriptor.description),
    description: descriptor.description,
    collectionId,
    entries,
    entryMap,
    groups,
  }
}

export async function loadAssetCatalog(): Promise<AssetCatalogManifest> {
  const response = await fetch('/pixel-catalog/asset-catalog.json')
  if (!response.ok) {
    throw new Error('读取素材目录失败')
  }

  const catalog = (await response.json()) as {
    version: string
    collections: Array<{ id: string; label: string; manifestPath: string }>
  }

  const collections = []
  for (const collection of catalog.collections) {
    const manifestResponse = await fetch(collection.manifestPath)
    if (!manifestResponse.ok) {
      throw new Error(`读取素材集合失败: ${collection.id}`)
    }

    const manifest = (await manifestResponse.json()) as {
      outputs: Array<{ id: string; description: string; path: string }>
    }
    const basePath = getCollectionBasePath(collection.manifestPath)
    collections.push({
      id: collection.id,
      label: collection.label,
      sourceAssets: manifest.outputs.map((item) => ({
        id: item.id,
        label: item.id,
        description: item.description,
        jsonPath: `${basePath}${item.path}`,
      })),
    })
  }

  return {
    version: catalog.version,
    collections,
  }
}

export async function loadCatalogSourceAsset(
  collectionId: string,
  sourceAsset: AssetCatalogSourceAsset,
): Promise<NormalizedCatalogAsset> {
  const response = await fetch(sourceAsset.jsonPath)
  if (!response.ok) {
    throw new Error(`读取素材源失败: ${sourceAsset.id}`)
  }

  const payload = (await response.json()) as Record<string, unknown>
  return normalizeSourceAsset(collectionId, sourceAsset, payload)
}

export function buildProjectAssetFromCatalogEntry(
  normalizedAsset: NormalizedCatalogAsset,
  entryId: string,
): AssetRecord {
  const entry = normalizedAsset.entryMap[entryId]
  if (!entry) {
    throw new Error(`未找到条目 ${entryId}`)
  }

  return {
    id: uid('asset'),
    kind: 'catalog-entry',
    label: `${normalizedAsset.label} · ${entryId}`,
    previewUrl: buildThumbnailDataUrl({
      id: entry.id,
      width: entry.width,
      height: entry.height,
      pixels: entry.pixels,
    }),
    width: entry.width,
    height: entry.height,
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

export function buildProjectAssetFromCatalogGroup(
  normalizedAsset: NormalizedCatalogAsset,
  groupId: string,
): AssetRecord {
  const group = normalizedAsset.groups.find((item) => item.id === groupId)
  if (!group) {
    throw new Error(`未找到分组 ${groupId}`)
  }

  const frames: CatalogFrame[] = group.entryIds
    .map((entryId) => normalizedAsset.entryMap[entryId])
    .filter((entry): entry is CatalogEntry => Boolean(entry))
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
    previewUrl: frames.length > 0 ? buildThumbnailDataUrl(frames[0]) : '',
    width: frames[0]?.width ?? 12,
    height: frames[0]?.height ?? 12,
    source: {
      collectionId: normalizedAsset.collectionId,
      sourceAssetId: normalizedAsset.id,
      mode: 'group',
      groupId,
    },
    frames,
  }
}

import { drawPixelText, formatWidgetText, measurePixelText } from './pixel-font'
import { sampleContinuousKeys, sampleStepKeys } from './scene-model'

const pixelCanvasCache = new Map()
const imageCache = new Map()

function parsePreviewDate(previewDate, previewTime) {
  const date = typeof previewDate === 'string' && previewDate.length > 0 ? previewDate : '2026-04-21'
  const time = typeof previewTime === 'string' && previewTime.length > 0 ? previewTime : '12:34:56'
  const parsed = new Date(`${date}T${time}`)
  if (Number.isNaN(parsed.getTime())) {
    return new Date('2026-04-21T12:34:56')
  }
  return parsed
}

function buildPreviewDate(scene) {
  const baseDate = parsePreviewDate(scene.meta.previewDate, scene.meta.previewTime)
  if (scene.meta.previewMode === 'minute') {
    baseDate.setMinutes(baseDate.getMinutes() + 1)
  } else if (scene.meta.previewMode === 'hour') {
    baseDate.setHours(baseDate.getHours() + 1)
    baseDate.setMinutes(0)
    baseDate.setSeconds(0)
  }
  baseDate.setMilliseconds(0)
  const elapsedMs = scene.meta.currentFrame * scene.meta.frameDelay
  baseDate.setTime(baseDate.getTime() + elapsedMs)
  return baseDate
}

function getProjectAsset(scene, assetId) {
  return scene.assets.find((asset) => asset.id === assetId) ?? null
}

function buildPixelFrameCanvas(cacheKey, frame) {
  if (pixelCanvasCache.has(cacheKey)) {
    return pixelCanvasCache.get(cacheKey)
  }

  const canvas = document.createElement('canvas')
  canvas.width = frame.width
  canvas.height = frame.height
  const context = canvas.getContext('2d')
  const imageData = context.createImageData(frame.width, frame.height)

  for (let index = 0; index < frame.pixels.length; index += 1) {
    const pixel = frame.pixels[index]
    const pixelIndex = (pixel[1] * frame.width + pixel[0]) * 4
    imageData.data[pixelIndex] = pixel[2]
    imageData.data[pixelIndex + 1] = pixel[3]
    imageData.data[pixelIndex + 2] = pixel[4]
    imageData.data[pixelIndex + 3] = 255
  }

  context.putImageData(imageData, 0, 0)
  pixelCanvasCache.set(cacheKey, canvas)
  return canvas
}

function loadImage(url) {
  if (imageCache.has(url)) {
    return imageCache.get(url)
  }

  const promise = new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('图片素材解码失败'))
    image.src = url
  })

  imageCache.set(url, promise)
  return promise
}

function selectProjectAssetFrame(projectAsset, frameIndex, sequenceStepFrames) {
  if (projectAsset === null) {
    return null
  }

  if (projectAsset.kind === 'uploaded-image') {
    return {
      kind: 'uploaded-image',
      width: projectAsset.width,
      height: projectAsset.height,
      dataUrl: projectAsset.dataUrl,
    }
  }

  if (Array.isArray(projectAsset.frames) === false || projectAsset.frames.length === 0) {
    return null
  }

  const step = Math.max(1, sequenceStepFrames)
  const index = projectAsset.frames.length === 1 ? 0 : Math.floor(frameIndex / step) % projectAsset.frames.length
  return {
    kind: 'pixel-frame',
    ...projectAsset.frames[index],
  }
}

function drawFrameToRect(context, assetFrame, x, y, width, height, opacity) {
  if (assetFrame === null) {
    return Promise.resolve()
  }

  context.save()
  context.globalAlpha = opacity
  context.imageSmoothingEnabled = false

  if (assetFrame.kind === 'uploaded-image') {
    return loadImage(assetFrame.dataUrl).then((image) => {
      context.drawImage(image, x, y, width, height)
      context.restore()
    })
  }

  const sourceCanvas = buildPixelFrameCanvas(`${assetFrame.id}-${assetFrame.width}-${assetFrame.height}`, assetFrame)
  context.drawImage(sourceCanvas, x, y, width, height)
  context.restore()
  return Promise.resolve()
}

function getBackgroundTransform(layer, frame) {
  return sampleContinuousKeys(layer.transformKeys, frame, {
    x: 0,
    y: 0,
    width: 32,
    height: 32,
    opacity: 1,
  })
}

function getBackgroundState(layer, frame) {
  return sampleStepKeys(layer.stateKeys, frame, {
    visible: true,
    behavior: 'fixed',
    rhythmFrames: 2,
    speedX: 0,
    speedY: 0,
    sequenceStepFrames: 1,
  })
}

function getGroupTransform(group, frame) {
  return sampleContinuousKeys(group.transformKeys, frame, {
    x: 0,
    y: 0,
    scale: 1,
    opacity: 1,
  })
}

function getGroupState(group, frame) {
  return sampleStepKeys(group.stateKeys, frame, {
    visible: true,
    behavior: 'fixed',
    rhythmFrames: 2,
    sequenceStepFrames: 1,
  })
}

function buildGroupBounds(items) {
  if (items.length === 0) {
    return null
  }

  let minX = Number.POSITIVE_INFINITY
  let minY = Number.POSITIVE_INFINITY
  let maxX = Number.NEGATIVE_INFINITY
  let maxY = Number.NEGATIVE_INFINITY

  for (let index = 0; index < items.length; index += 1) {
    minX = Math.min(minX, items[index].x)
    minY = Math.min(minY, items[index].y)
    maxX = Math.max(maxX, items[index].x + items[index].width)
    maxY = Math.max(maxY, items[index].y + items[index].height)
  }

  return {
    x: minX,
    y: minY,
    width: maxX - minX,
    height: maxY - minY,
  }
}

function buildBackgroundDrawables(scene) {
  const drawables = []

  for (let index = 0; index < scene.backgroundLayers.length; index += 1) {
    const layer = scene.backgroundLayers[index]
    const transform = getBackgroundTransform(layer, scene.meta.currentFrame)
    const state = getBackgroundState(layer, scene.meta.currentFrame)
    const projectAsset = getProjectAsset(scene, layer.projectAssetId)
    const assetFrame = selectProjectAssetFrame(projectAsset, scene.meta.currentFrame, state.sequenceStepFrames)
    drawables.push({
      id: layer.id,
      name: layer.name,
      baseColor: layer.baseColor,
      transform,
      state,
      assetFrame,
    })
  }

  return drawables
}

function buildCharacterDrawables(scene) {
  const drawables = []

  for (let index = 0; index < scene.characterGroups.length; index += 1) {
    const group = scene.characterGroups[index]
    const transform = getGroupTransform(group, scene.meta.currentFrame)
    const state = getGroupState(group, scene.meta.currentFrame)
    const members = []

    for (let memberIndex = 0; memberIndex < group.members.length; memberIndex += 1) {
      const member = group.members[memberIndex]
      const projectAsset = getProjectAsset(scene, member.projectAssetId)
      const assetFrame = selectProjectAssetFrame(projectAsset, scene.meta.currentFrame, state.sequenceStepFrames)
      const width = Math.max(1, Math.round(member.width * transform.scale))
      const height = Math.max(1, Math.round(member.height * transform.scale))
      members.push({
        ...member,
        assetFrame,
        x: Math.round(transform.x + member.offsetX * transform.scale),
        y: Math.round(transform.y + member.offsetY * transform.scale),
        width,
        height,
      })
    }

    drawables.push({
      id: group.id,
      name: group.name,
      transform,
      state,
      members,
      bounds: buildGroupBounds(members),
    })
  }

  return drawables
}

function buildWidgetDrawables(scene, previewDate) {
  return scene.timeWidgets.map((widget) => {
    const text = formatWidgetText(widget, previewDate)
    const measure = measurePixelText(text, widget.scale, widget.letterSpacing)

    return {
      ...widget,
      text,
      bounds: {
        x: widget.x,
        y: widget.y,
        width: measure.width,
        height: measure.height,
      },
    }
  })
}

export function buildSceneSnapshot(scene) {
  const previewDate = buildPreviewDate(scene)
  return {
    matrix: scene.meta.matrix,
    previewDate,
    backgrounds: buildBackgroundDrawables(scene),
    characters: buildCharacterDrawables(scene),
    widgets: buildWidgetDrawables(scene, previewDate),
  }
}

async function drawCharacterGroup(context, group, currentFrame) {
  if (group.state.visible !== true) {
    return
  }

  let groupOpacity = group.transform.opacity
  if (group.state.behavior === 'blink') {
    const phase = Math.floor(currentFrame / Math.max(1, group.state.rhythmFrames)) % 2
    groupOpacity = phase === 0 ? group.transform.opacity : group.transform.opacity * 0.18
  }

  for (let index = 0; index < group.members.length; index += 1) {
    const member = group.members[index]
    if (member.visible !== true || member.assetFrame === null) {
      continue
    }
    await drawFrameToRect(
      context,
      member.assetFrame,
      member.x,
      member.y,
      member.width,
      member.height,
      groupOpacity * member.opacity,
    )
  }
}

export async function renderSceneToCanvas(canvas, scene) {
  const snapshot = buildSceneSnapshot(scene)
  canvas.width = snapshot.matrix.width
  canvas.height = snapshot.matrix.height
  const context = canvas.getContext('2d')
  context.clearRect(0, 0, snapshot.matrix.width, snapshot.matrix.height)
  context.fillStyle = '#000000'
  context.fillRect(0, 0, snapshot.matrix.width, snapshot.matrix.height)
  context.imageSmoothingEnabled = false

  for (let index = 0; index < snapshot.backgrounds.length; index += 1) {
    const layer = snapshot.backgrounds[index]
    if (layer.state.visible !== true) {
      continue
    }

    context.save()
    context.fillStyle = layer.baseColor
    context.globalAlpha = layer.transform.opacity
    context.fillRect(layer.transform.x, layer.transform.y, layer.transform.width, layer.transform.height)
    context.restore()

    if (layer.assetFrame === null) {
      continue
    }

    let opacity = layer.transform.opacity
    if (layer.state.behavior === 'blink') {
      const phase = Math.floor(scene.meta.currentFrame / Math.max(1, layer.state.rhythmFrames)) % 2
      opacity = phase === 0 ? layer.transform.opacity : layer.transform.opacity * 0.18
    }

    if (layer.state.behavior === 'loop') {
      const offsetX =
        ((scene.meta.currentFrame * layer.state.speedX) % layer.transform.width + layer.transform.width) %
        layer.transform.width
      const offsetY =
        ((scene.meta.currentFrame * layer.state.speedY) % layer.transform.height + layer.transform.height) %
        layer.transform.height

      for (let tileX = -1; tileX <= 2; tileX += 1) {
        for (let tileY = -1; tileY <= 2; tileY += 1) {
          await drawFrameToRect(
            context,
            layer.assetFrame,
            Math.round(layer.transform.x - offsetX + tileX * layer.transform.width),
            Math.round(layer.transform.y - offsetY + tileY * layer.transform.height),
            layer.transform.width,
            layer.transform.height,
            opacity,
          )
        }
      }
    } else {
      await drawFrameToRect(
        context,
        layer.assetFrame,
        layer.transform.x,
        layer.transform.y,
        layer.transform.width,
        layer.transform.height,
        opacity,
      )
    }
  }

  for (let index = 0; index < snapshot.characters.length; index += 1) {
    await drawCharacterGroup(context, snapshot.characters[index], scene.meta.currentFrame)
  }

  for (let index = 0; index < snapshot.widgets.length; index += 1) {
    const widget = snapshot.widgets[index]
    if (widget.visible === true) {
      drawPixelText(context, widget, snapshot.previewDate)
    }
  }

  return snapshot
}

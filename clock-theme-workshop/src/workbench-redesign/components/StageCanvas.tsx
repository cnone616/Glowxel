import { useEffect, useRef, useState } from 'react'
import { buildPixelTextCells } from '../../editor/core/pixelFont'
import { selectAssetFrame } from '../../editor/core/renderEngine'
import type {
  CatalogFrame,
  DrawableBackground,
  DrawableObjectGroup,
  DrawablePixelLayer,
  DrawableTimeWidget,
  RenderSnapshot,
  TransitionOverlay,
} from '../../editor/types'

const pixelFrameCache = new Map<string, HTMLCanvasElement>()
const uploadedImageCache = new Map<string, HTMLImageElement>()

function toColor(hex: string): string {
  return hex
}

function getPixelFrameCanvas(frame: CatalogFrame): HTMLCanvasElement {
  const cacheKey = `${frame.id}-${frame.width}-${frame.height}`
  const cached = pixelFrameCache.get(cacheKey)
  if (cached) {
    return cached
  }

  const canvas = window.document.createElement('canvas')
  canvas.width = frame.width
  canvas.height = frame.height
  const context = canvas.getContext('2d')
  if (!context) {
    throw new Error('像素帧上下文创建失败')
  }

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

  pixelFrameCache.set(cacheKey, canvas)
  return canvas
}

function getUploadedImage(dataUrl: string, onLoad: () => void): HTMLImageElement {
  const cached = uploadedImageCache.get(dataUrl)
  if (cached) {
    if (!cached.complete) {
      cached.onload = onLoad
    }
    return cached
  }

  const image = new Image()
  image.onload = onLoad
  image.src = dataUrl
  uploadedImageCache.set(dataUrl, image)
  return image
}

function drawRaster(
  context: CanvasRenderingContext2D,
  asset: CatalogFrame | { dataUrl: string } | null,
  x: number,
  y: number,
  width: number,
  height: number,
): void {
  if (!asset) {
    return
  }

  if ('pixels' in asset) {
    context.drawImage(getPixelFrameCanvas(asset), x, y, width, height)
    return
  }

  const image = getUploadedImage(asset.dataUrl, () => {})
  if (image.complete) {
    context.drawImage(image, x, y, width, height)
  }
}

function drawBackgrounds(
  context: CanvasRenderingContext2D,
  backgrounds: DrawableBackground[],
  frameIndex: number,
  alpha: number,
) {
  for (const background of backgrounds) {
    const assetFrame = selectAssetFrame(background.asset, frameIndex, background.state.sequenceStepFrames)
    const previousAlpha = context.globalAlpha
    let opacity = background.transform.opacity * alpha
    if (background.state.behavior === 'blink') {
      const phase = Math.floor(frameIndex / Math.max(1, background.state.rhythmFrames)) % 2
      opacity = phase === 0 ? opacity : opacity * 0.18
    }

    const tiles =
      background.state.behavior === 'loop'
        ? Array.from({ length: 16 }, (_, index) => ({
            tileX: (index % 4) - 1,
            tileY: Math.floor(index / 4) - 1,
          }))
        : [{ tileX: 0, tileY: 0 }]

    const offsetX =
      background.state.behavior === 'loop'
        ? ((frameIndex * background.state.speedX) % background.transform.width + background.transform.width) %
          background.transform.width
        : 0
    const offsetY =
      background.state.behavior === 'loop'
        ? ((frameIndex * background.state.speedY) % background.transform.height + background.transform.height) %
          background.transform.height
        : 0

    context.globalAlpha = opacity
    context.fillStyle = toColor(background.baseColor)
    context.fillRect(
      background.transform.x,
      background.transform.y,
      background.transform.width,
      background.transform.height,
    )

    if (assetFrame) {
      for (const tile of tiles) {
        drawRaster(
          context,
          'pixels' in assetFrame ? assetFrame : assetFrame.dataUrl ? { dataUrl: assetFrame.dataUrl } : null,
          Math.round(background.transform.x - offsetX + tile.tileX * background.transform.width),
          Math.round(background.transform.y - offsetY + tile.tileY * background.transform.height),
          background.transform.width,
          background.transform.height,
        )
      }
    }

    context.globalAlpha = previousAlpha
  }
}

function drawPixelLayers(
  context: CanvasRenderingContext2D,
  pixelLayers: DrawablePixelLayer[],
  alpha: number,
) {
  for (const layer of pixelLayers) {
    const imageData = context.createImageData(64, 64)
    for (let index = 0; index < layer.bitmap.length; index += 4) {
      imageData.data[index] = layer.bitmap[index] ?? 0
      imageData.data[index + 1] = layer.bitmap[index + 1] ?? 0
      imageData.data[index + 2] = layer.bitmap[index + 2] ?? 0
      imageData.data[index + 3] = Math.round((layer.bitmap[index + 3] ?? 0) * layer.opacity * alpha)
    }

    const bufferCanvas = window.document.createElement('canvas')
    bufferCanvas.width = 64
    bufferCanvas.height = 64
    const bufferContext = bufferCanvas.getContext('2d')
    if (!bufferContext) {
      continue
    }
    bufferContext.putImageData(imageData, 0, 0)
    context.drawImage(bufferCanvas, layer.x, layer.y, 64, 64)
  }
}

function drawObjects(
  context: CanvasRenderingContext2D,
  groups: DrawableObjectGroup[],
  frameIndex: number,
  alpha: number,
) {
  for (const group of groups) {
    if (!group.state.visible) {
      continue
    }

    let opacity = group.transform.opacity * alpha
    if (group.state.behavior === 'blink') {
      const phase = Math.floor(frameIndex / Math.max(1, group.state.rhythmFrames)) % 2
      opacity = phase === 0 ? opacity : opacity * 0.18
    }

    for (const member of group.members) {
      if (!member.visible || !member.asset) {
        continue
      }

      const assetFrame = selectAssetFrame(member.asset, frameIndex, group.state.sequenceStepFrames)
      if (!assetFrame) {
        continue
      }

      const previousAlpha = context.globalAlpha
      context.globalAlpha = opacity * member.opacity
      drawRaster(
        context,
        'pixels' in assetFrame ? assetFrame : assetFrame.dataUrl ? { dataUrl: assetFrame.dataUrl } : null,
        member.x,
        member.y,
        Math.max(1, Math.round(member.width * group.transform.scale)),
        Math.max(1, Math.round(member.height * group.transform.scale)),
      )
      context.globalAlpha = previousAlpha
    }
  }
}

function drawWidgets(
  context: CanvasRenderingContext2D,
  widgets: DrawableTimeWidget[],
  alpha: number,
) {
  const previousAlpha = context.globalAlpha
  context.globalAlpha = alpha
  for (const widget of widgets) {
    context.fillStyle = widget.color
    for (const cell of buildPixelTextCells(widget)) {
      context.fillRect(cell.x, cell.y, cell.size, cell.size)
    }
  }
  context.globalAlpha = previousAlpha
}

function drawPack(
  context: CanvasRenderingContext2D,
  pack: Pick<RenderSnapshot, 'backgrounds' | 'pixelLayers' | 'objects' | 'widgets'> | TransitionOverlay,
  frameIndex: number,
  alpha: number,
) {
  drawBackgrounds(context, pack.backgrounds, frameIndex, alpha)
  drawPixelLayers(context, pack.pixelLayers, alpha)
  drawObjects(context, pack.objects, frameIndex, alpha)
  drawWidgets(context, pack.widgets, alpha)
}

export function StageCanvas({
  className,
  snapshot,
}: {
  className?: string
  snapshot: RenderSnapshot
}) {
  const ref = useRef<HTMLCanvasElement | null>(null)
  const [version, setVersion] = useState(0)

  useEffect(() => {
    const canvas = ref.current
    if (!canvas) {
      return
    }
    const context = canvas.getContext('2d')
    if (!context) {
      return
    }

    const notifyLoaded = () => {
      setVersion((value) => value + 1)
    }

    context.clearRect(0, 0, snapshot.matrix.width, snapshot.matrix.height)
    context.imageSmoothingEnabled = false
    context.fillStyle = '#090b10'
    context.fillRect(0, 0, snapshot.matrix.width, snapshot.matrix.height)

    for (const background of snapshot.backgrounds) {
      if (background.asset?.dataUrl) {
        getUploadedImage(background.asset.dataUrl, notifyLoaded)
      }
    }

    for (const group of snapshot.objects) {
      for (const member of group.members) {
        if (member.asset?.dataUrl) {
          getUploadedImage(member.asset.dataUrl, notifyLoaded)
        }
      }
    }

    if (snapshot.transitionOverlay) {
      for (const background of snapshot.transitionOverlay.backgrounds) {
        if (background.asset?.dataUrl) {
          getUploadedImage(background.asset.dataUrl, notifyLoaded)
        }
      }
      for (const group of snapshot.transitionOverlay.objects) {
        for (const member of group.members) {
          if (member.asset?.dataUrl) {
            getUploadedImage(member.asset.dataUrl, notifyLoaded)
          }
        }
      }
    }

    drawPack(context, snapshot, snapshot.sceneFrame, snapshot.baseAlpha)
    if (snapshot.transitionOverlay) {
      drawPack(
        context,
        snapshot.transitionOverlay,
        snapshot.transitionOverlay.localFrame,
        snapshot.transitionOverlay.alpha,
      )
    }
  }, [snapshot, version])

  return <canvas ref={ref} className={className} width={snapshot.matrix.width} height={snapshot.matrix.height} />
}

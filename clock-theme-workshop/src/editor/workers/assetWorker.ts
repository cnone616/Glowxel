/// <reference lib="webworker" />

import type { WorkerProcessedAsset } from '../types'

declare class FileReaderSync {
  readAsDataURL(blob: Blob): string
}

interface WorkerRequestMessage {
  type: 'process'
  requestId: string
  files: File[]
  chromaKey: boolean
}

interface WorkerSuccessMessage {
  type: 'success'
  requestId: string
  assets: WorkerProcessedAsset[]
}

interface WorkerErrorMessage {
  type: 'error'
  requestId: string
  message: string
}

function uid(prefix: string): string {
  return `${prefix}-${Date.now()}-${Math.random().toString(16).slice(2, 10)}`
}

function shouldKeyOutPixel(red: number, green: number, blue: number): boolean {
  const brightEnough = green > 96 && blue > 96
  const lowRed = red < 146
  const closeToCyan = Math.abs(green - blue) < 72
  const cyanBias = green - red > 34 && blue - red > 20
  return brightEnough && lowRed && closeToCyan && cyanBias
}

function blobToDataUrl(blob: Blob): string {
  const reader = new FileReaderSync()
  return reader.readAsDataURL(blob)
}

async function buildThumbnailDataUrl(canvas: OffscreenCanvas): Promise<string> {
  const thumbWidth = 108
  const thumbHeight = 76
  const thumbnailCanvas = new OffscreenCanvas(thumbWidth, thumbHeight)
  const context = thumbnailCanvas.getContext('2d')
  if (!context) {
    throw new Error('缩略图上下文创建失败')
  }

  context.imageSmoothingEnabled = false
  context.fillStyle = '#020508'
  context.fillRect(0, 0, thumbWidth, thumbHeight)

  const scale = Math.min(thumbWidth / canvas.width, thumbHeight / canvas.height)
  const drawWidth = Math.max(1, Math.round(canvas.width * scale))
  const drawHeight = Math.max(1, Math.round(canvas.height * scale))
  const drawX = Math.floor((thumbWidth - drawWidth) / 2)
  const drawY = Math.floor((thumbHeight - drawHeight) / 2)
  context.drawImage(canvas, drawX, drawY, drawWidth, drawHeight)

  const blob = await thumbnailCanvas.convertToBlob({ type: 'image/png' })
  return blobToDataUrl(blob)
}

async function processFile(
  file: File,
  chromaKey: boolean,
): Promise<WorkerProcessedAsset> {
  if (typeof createImageBitmap !== 'function') {
    throw new Error('当前环境不支持 createImageBitmap')
  }

  if (typeof OffscreenCanvas !== 'function') {
    throw new Error('当前环境不支持 OffscreenCanvas')
  }

  const image = await createImageBitmap(file)
  const canvas = new OffscreenCanvas(image.width, image.height)
  const context = canvas.getContext('2d', { willReadFrequently: true })

  if (!context) {
    throw new Error('图片处理上下文创建失败')
  }

  context.drawImage(image, 0, 0)

  if (chromaKey) {
    const imageData = context.getImageData(0, 0, canvas.width, canvas.height)
    for (let index = 0; index < imageData.data.length; index += 4) {
      const red = imageData.data[index]
      const green = imageData.data[index + 1]
      const blue = imageData.data[index + 2]
      if (shouldKeyOutPixel(red, green, blue)) {
        imageData.data[index + 3] = 0
      }
    }
    context.putImageData(imageData, 0, 0)
  }

  const processedBlob = await canvas.convertToBlob({ type: 'image/png' })
  const label = file.name.replace(/\.[^.]+$/, '')

  return {
    id: uid('asset'),
    label,
    previewUrl: await buildThumbnailDataUrl(canvas),
    dataUrl: blobToDataUrl(processedBlob),
    width: canvas.width,
    height: canvas.height,
    chromaKeyEnabled: chromaKey,
    sourceFileName: file.name,
  }
}

self.onmessage = async (event: MessageEvent<WorkerRequestMessage>) => {
  if (event.data.type !== 'process') {
    return
  }

  try {
    const assets: WorkerProcessedAsset[] = []
    for (const file of event.data.files) {
      assets.push(await processFile(file, event.data.chromaKey))
    }

    const message: WorkerSuccessMessage = {
      type: 'success',
      requestId: event.data.requestId,
      assets,
    }

    self.postMessage(message)
  } catch (error) {
    const message: WorkerErrorMessage = {
      type: 'error',
      requestId: event.data.requestId,
      message: error instanceof Error ? error.message : '图片处理失败',
    }

    self.postMessage(message)
  }
}

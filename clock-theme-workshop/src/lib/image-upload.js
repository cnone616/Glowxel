import { uid } from './scene-model'

function loadImage(url) {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = () => reject(new Error('图片解码失败'))
    image.src = url
  })
}

function readAsDataUrl(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => {
      if (typeof reader.result !== 'string') {
        reject(new Error('文件读取失败'))
        return
      }
      resolve(reader.result)
    }
    reader.onerror = () => reject(new Error('文件读取失败'))
    reader.readAsDataURL(file)
  })
}

function shouldKeyOutPixel(red, green, blue) {
  const brightEnough = green > 96 && blue > 96
  const lowRed = red < 146
  const closeToCyan = Math.abs(green - blue) < 72
  const cyanBias = green - red > 34 && blue - red > 20
  return brightEnough && lowRed && closeToCyan && cyanBias
}

function buildThumbnailFromCanvas(canvas) {
  const thumbWidth = 108
  const thumbHeight = 76
  const thumbCanvas = document.createElement('canvas')
  thumbCanvas.width = thumbWidth
  thumbCanvas.height = thumbHeight
  const context = thumbCanvas.getContext('2d')
  context.imageSmoothingEnabled = false
  context.fillStyle = '#020508'
  context.fillRect(0, 0, thumbWidth, thumbHeight)

  const scale = Math.min(thumbWidth / canvas.width, thumbHeight / canvas.height)
  const drawWidth = Math.max(1, Math.round(canvas.width * scale))
  const drawHeight = Math.max(1, Math.round(canvas.height * scale))
  const drawX = Math.floor((thumbWidth - drawWidth) / 2)
  const drawY = Math.floor((thumbHeight - drawHeight) / 2)
  context.drawImage(canvas, drawX, drawY, drawWidth, drawHeight)

  return thumbCanvas.toDataURL('image/png')
}

export async function createUploadedProjectAsset(file, options) {
  const sourceUrl = await readAsDataUrl(file)
  const image = await loadImage(sourceUrl)
  const canvas = document.createElement('canvas')
  canvas.width = image.naturalWidth
  canvas.height = image.naturalHeight
  const context = canvas.getContext('2d', { willReadFrequently: true })
  context.drawImage(image, 0, 0)

  if (options.chromaKey === true) {
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

  const processedUrl = canvas.toDataURL('image/png')
  return {
    id: uid('asset'),
    kind: 'uploaded-image',
    label: file.name.replace(/\.[^.]+$/, ''),
    trackHint: options.trackHint,
    previewUrl: buildThumbnailFromCanvas(canvas),
    dataUrl: processedUrl,
    width: canvas.width,
    height: canvas.height,
    chromaKeyEnabled: options.chromaKey === true,
    sourceFileName: file.name,
  }
}

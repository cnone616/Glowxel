const ACCEPTED_MIME_TYPES = ['image/png', 'image/jpeg', 'image/webp', 'image/gif']

function fileToDataUrl(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(String(reader.result))
    reader.onerror = () => reject(new Error(`读取素材失败: ${file.name}`))
    reader.readAsDataURL(file)
  })
}

function loadImageMeta(dataUrl, fileName) {
  return new Promise((resolve, reject) => {
    const image = new Image()
    image.onload = () => {
      resolve({
        width: image.naturalWidth,
        height: image.naturalHeight,
      })
    }
    image.onerror = () => reject(new Error(`无法解析图片: ${fileName}`))
    image.src = dataUrl
  })
}

function sanitizeName(fileName) {
  return fileName.replace(/\.[^.]+$/, '') || '未命名素材'
}

export function getAcceptedImageMimeTypes() {
  return [...ACCEPTED_MIME_TYPES]
}

export async function buildMaterialAssetFromFile(file, category) {
  if (!ACCEPTED_MIME_TYPES.includes(file.type)) {
    throw new Error(`不支持的文件类型: ${file.name}`)
  }

  const sourceDataUrl = await fileToDataUrl(file)
  const meta = await loadImageMeta(sourceDataUrl, file.name)

  return {
    id: crypto.randomUUID(),
    name: sanitizeName(file.name),
    category,
    fileName: file.name,
    mimeType: file.type,
    sourceDataUrl,
    width: meta.width,
    height: meta.height,
    fitMode: category === 'background' ? 'cover' : 'contain',
    createdAt: new Date().toISOString(),
  }
}

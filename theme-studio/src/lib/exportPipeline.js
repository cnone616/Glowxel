import JSZip from 'jszip'
import { EXPORT_FILE_NAMES } from './studioSchema'
import { buildCanvasTextLayout, parsePreviewTime } from './studioProject'
import { buildReadabilityReport } from './readability'
import { buildThemeExportBundle } from '../utils/exporters'
import { buildStudioSummary, compileStudioThemePackage } from './studio-compiler'

async function blobToText(blob) {
  return blob.text()
}

function createObjectUrl(blob) {
  return URL.createObjectURL(blob)
}

export function revokeBundleUrls(bundle) {
  if (!bundle) {
    return
  }

  if (bundle.urls.coverUrl) {
    URL.revokeObjectURL(bundle.urls.coverUrl)
  }

  if (bundle.urls.previewUrl) {
    URL.revokeObjectURL(bundle.urls.previewUrl)
  }

  if (bundle.urls.validationUrl) {
    URL.revokeObjectURL(bundle.urls.validationUrl)
  }
}

export async function generateProjectBundle(project) {
  const workingProject = project
  const studioSummary = buildStudioSummary(workingProject)
  const compiledThemePackage = await compileStudioThemePackage(workingProject)
  const textLayout = buildCanvasTextLayout(workingProject)
  const previewTime = parsePreviewTime(workingProject)
  const readability = buildReadabilityReport(textLayout, previewTime)
  const exportBundle = await buildThemeExportBundle(compiledThemePackage, textLayout)

  return {
    projectId: workingProject.id,
    projectName: workingProject.name,
    themePackage: exportBundle.themePackage,
    validation: exportBundle.report,
    readability,
    studioSummary,
    files: exportBundle.files,
    urls: {
      coverUrl: exportBundle.files.coverPngBlob ? createObjectUrl(exportBundle.files.coverPngBlob) : '',
      previewUrl: exportBundle.files.previewGifBlob ? createObjectUrl(exportBundle.files.previewGifBlob) : '',
      validationUrl: createObjectUrl(exportBundle.files.validationBlob),
    },
  }
}

export async function buildPublishZip(bundle) {
  const zip = new JSZip()
  zip.file(EXPORT_FILE_NAMES.themeJson, await blobToText(bundle.files.themeJsonBlob))
  zip.file(EXPORT_FILE_NAMES.validationJson, await blobToText(bundle.files.validationBlob))

  if (bundle.files.coverPngBlob) {
    zip.file(EXPORT_FILE_NAMES.coverPng, bundle.files.coverPngBlob)
  }

  if (bundle.files.previewGifBlob) {
    zip.file(EXPORT_FILE_NAMES.previewGif, bundle.files.previewGifBlob)
  }

  return zip.generateAsync({ type: 'blob' })
}

export function triggerDownload(blob, filename) {
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = filename
  document.body.appendChild(anchor)
  anchor.click()
  anchor.remove()
  URL.revokeObjectURL(url)
}

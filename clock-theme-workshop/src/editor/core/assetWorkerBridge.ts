import type { AssetRecord, WorkerProcessedAsset } from '../types'

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

type WorkerResponseMessage = WorkerSuccessMessage | WorkerErrorMessage

export class AssetWorkerBridge {
  private readonly worker: Worker

  constructor() {
    this.worker = new Worker(new URL('../workers/assetWorker.ts', import.meta.url), {
      type: 'module',
    })
  }

  processFiles(files: File[], chromaKey: boolean): Promise<AssetRecord[]> {
    return new Promise((resolve, reject) => {
      const requestId = `${Date.now()}-${Math.random().toString(16).slice(2, 10)}`

      const handleMessage = (event: MessageEvent<WorkerResponseMessage>) => {
        if (event.data.requestId !== requestId) {
          return
        }

        this.worker.removeEventListener('message', handleMessage)

        if (event.data.type === 'error') {
          reject(new Error(event.data.message))
          return
        }

        resolve(
          event.data.assets.map((asset) => ({
            id: asset.id,
            kind: 'uploaded-image',
            label: asset.label,
            previewUrl: asset.previewUrl,
            dataUrl: asset.dataUrl,
            width: asset.width,
            height: asset.height,
            chromaKeyEnabled: asset.chromaKeyEnabled,
            sourceFileName: asset.sourceFileName,
          })),
        )
      }

      this.worker.addEventListener('message', handleMessage)
      this.worker.postMessage({
        type: 'process',
        requestId,
        files,
        chromaKey,
      } satisfies WorkerRequestMessage)
    })
  }
}

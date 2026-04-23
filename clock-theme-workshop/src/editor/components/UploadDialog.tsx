import { useMemo, useState } from 'react'
import type { ChangeEvent } from 'react'
import * as Dialog from '@radix-ui/react-dialog'
import { commandBus } from '../core/commandBus'
import { AssetWorkerBridge } from '../core/assetWorkerBridge'
import { useEditorStore } from '../state/editorStore'

export function UploadDialog() {
  const open = useEditorStore((state) => state.session.uploadDialogOpen)
  const uploadChromaKey = useEditorStore((state) => state.session.uploadChromaKey)
  const workerBridge = useMemo(() => new AssetWorkerBridge(), [])
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')

  const handleUpload = async (event: ChangeEvent<HTMLInputElement>) => {
    const fileList = event.target.files
    if (!fileList || fileList.length === 0) {
      return
    }

    setBusy(true)
    setError('')

    try {
      const assets = await workerBridge.processFiles(Array.from(fileList), uploadChromaKey)
      for (const asset of assets) {
        commandBus.execute({ type: 'material.addUploaded', asset })
      }
      commandBus.execute({ type: 'session.setUploadDialogOpen', open: false })
    } catch (uploadError) {
      setError(uploadError instanceof Error ? uploadError.message : '图片处理失败')
    } finally {
      setBusy(false)
      event.target.value = ''
    }
  }

  return (
    <Dialog.Root open={open} onOpenChange={(nextOpen) => commandBus.execute({ type: 'session.setUploadDialogOpen', open: nextOpen })}>
      <Dialog.Portal>
        <Dialog.Overlay className="dialog-overlay" />
        <Dialog.Content className="dialog-content">
          <div className="dialog-head">
            <div>
              <span className="eyebrow-label">Upload</span>
              <Dialog.Title>上传素材到素材库</Dialog.Title>
            </div>
            <Dialog.Close className="ghost-inline">关闭</Dialog.Close>
          </div>

          <div className="dialog-body">
            <div className="field-stack">
              <label className="toggle-line">
                <input
                  checked={uploadChromaKey}
                  onChange={(event) =>
                    commandBus.execute({
                      type: 'session.setUploadChromaKey',
                      value: event.target.checked,
                    })
                  }
                  type="checkbox"
                />
                <span>启用青绿色抠底</span>
              </label>

              <label className={busy ? 'upload-dropzone busy' : 'upload-dropzone'}>
                <input
                  accept="image/png,image/jpeg,image/webp"
                  className="hidden-input"
                  disabled={busy}
                  multiple
                  onChange={handleUpload}
                  type="file"
                />
                <strong>{busy ? '正在处理图片…' : '选择图片或拖到这里'}</strong>
                <span>上传后先进入素材库，再由你显式插入当前场景。</span>
              </label>

              {error.length > 0 ? <p className="inline-error">{error}</p> : null}
            </div>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  )
}

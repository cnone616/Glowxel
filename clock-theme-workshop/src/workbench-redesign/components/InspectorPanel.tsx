import { commandBus } from '../../editor/core/commandBus'
import { getActiveScene } from '../../editor/core/themeDocument'
import { useEditorStore } from '../../editor/state/editorStore'

function getTransitionLabel(value: 'cut' | 'fade'): string {
  return value === 'cut' ? '直接切换' : '淡入淡出'
}

function getBackgroundBehaviorLabel(value: 'fixed' | 'loop' | 'blink'): string {
  if (value === 'fixed') {
    return '固定'
  }
  if (value === 'loop') {
    return '循环'
  }
  return '闪烁'
}

function getObjectBehaviorLabel(value: 'fixed' | 'blink' | 'path'): string {
  if (value === 'fixed') {
    return '固定'
  }
  if (value === 'blink') {
    return '闪烁'
  }
  return '路径'
}

function getPathModeLabel(value: 'loop' | 'pingpong' | 'once'): string {
  if (value === 'loop') {
    return '循环'
  }
  if (value === 'pingpong') {
    return '往返'
  }
  return '单次'
}

function getTimeTypeLabel(value: 'time' | 'date' | 'week' | 'year'): string {
  if (value === 'time') {
    return '时间'
  }
  if (value === 'date') {
    return '日期'
  }
  if (value === 'week') {
    return '星期'
  }
  return '年份'
}

function getMaterialKindLabel(value: 'catalog-entry' | 'catalog-sequence' | 'uploaded-image'): string {
  if (value === 'catalog-entry') {
    return '目录单帧'
  }
  if (value === 'catalog-sequence') {
    return '目录序列'
  }
  return '上传图片'
}

function Field({
  label,
  children,
}: {
  label: string
  children: React.ReactNode
}) {
  return (
    <label className="mx-inspector-field">
      <span>{label}</span>
      {children}
    </label>
  )
}

function NumberField({
  label,
  value,
  onChange,
  min,
  max,
  step,
}: {
  label: string
  value: number
  onChange: (value: number) => void
  min?: number
  max?: number
  step?: number
}) {
  return (
    <Field label={label}>
      <input
        min={min}
        max={max}
        onChange={(event) => onChange(Number(event.target.value))}
        step={step ?? 1}
        type="number"
        value={value}
      />
    </Field>
  )
}

function TimingFields({
  kind,
  id,
  startFrame,
  endFrame,
}: {
  kind: 'pixel' | 'object' | 'time'
  id: string
  startFrame: number
  endFrame: number
}) {
  return (
    <div className="mx-inspector-grid">
      <NumberField
        label="起始帧"
        value={startFrame}
        onChange={(value) =>
          commandBus.execute({
            type: 'layer.setTiming',
            kind,
            id,
            timing: { startFrame: value, endFrame },
          })
        }
      />
      <NumberField
        label="结束帧"
        value={endFrame}
        onChange={(value) =>
          commandBus.execute({
            type: 'layer.setTiming',
            kind,
            id,
            timing: { startFrame, endFrame: value },
          })
        }
      />
    </div>
  )
}

function SceneInspector() {
  const document = useEditorStore((state) => state.document)
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{activeScene.name}</strong>
        <span>场景</span>
      </div>

      <Field label="场景名">
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'scene.update',
              id: activeScene.id,
              patch: { name: event.target.value },
            })
          }
          type="text"
          value={activeScene.name}
        />
      </Field>

      <NumberField
        label="时长"
        min={8}
        value={activeScene.durationFrames}
        onChange={(value) => commandBus.execute({ type: 'scene.trim', id: activeScene.id, durationFrames: value })}
      />

      <Field label="转场">
        <div className="mx-segmented">
          {(['cut', 'fade'] as const).map((transitionOut) => (
            <button
              key={transitionOut}
              className={activeScene.transitionOut === transitionOut ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() =>
                commandBus.execute({
                  type: 'scene.update',
                  id: activeScene.id,
                  patch: { transitionOut },
                })
              }
              type="button"
            >
              {getTransitionLabel(transitionOut)}
            </button>
          ))}
        </div>
      </Field>

      <div className="mx-inspector-grid">
        <Field label="调色板锁定">
          <button
            className={document.themeRules.paletteLock ? 'mx-segment is-active' : 'mx-segment'}
            onClick={() =>
              commandBus.execute({
                type: 'themeRule.update',
                patch: { paletteLock: !document.themeRules.paletteLock },
              })
            }
            type="button"
          >
            {document.themeRules.paletteLock ? '已锁定' : '未锁定'}
          </button>
        </Field>
        <NumberField
          label="像素网格"
          min={1}
          value={document.themeRules.pixelGridSize}
          onChange={(value) =>
            commandBus.execute({
              type: 'themeRule.update',
              patch: { pixelGridSize: value },
            })
          }
        />
      </div>

      <div className="mx-inspector-grid">
        <NumberField
          label="矩阵宽度"
          min={1}
          value={document.themeRules.matrixBound.width}
          onChange={(value) =>
            commandBus.execute({
              type: 'themeRule.update',
              patch: {
                matrixBound: {
                  width: value,
                  height: document.themeRules.matrixBound.height,
                },
              },
            })
          }
        />
        <NumberField
          label="矩阵高度"
          min={1}
          value={document.themeRules.matrixBound.height}
          onChange={(value) =>
            commandBus.execute({
              type: 'themeRule.update',
              patch: {
                matrixBound: {
                  width: document.themeRules.matrixBound.width,
                  height: value,
                },
              },
            })
          }
        />
      </div>
    </>
  )
}

function PixelLayerInspector() {
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const drawColor = useEditorStore((state) => state.session.drawColor)
  const layer = activeScene.pixelLayers.find((item) => item.id === selectionId) ?? null

  if (!layer) {
    return null
  }

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{layer.name}</strong>
        <span>像素层</span>
      </div>

      <Field label="图层名">
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'pixelLayer.update',
              id: layer.id,
              patch: { name: event.target.value },
            })
          }
          type="text"
          value={layer.name}
        />
      </Field>

      <div className="mx-inspector-grid">
        <Field label="显隐">
          <button
            className={layer.visible ? 'mx-segment is-active' : 'mx-segment'}
            onClick={() => commandBus.execute({ type: 'outline.toggleVisibility', kind: 'pixel', id: layer.id })}
            type="button"
          >
            {layer.visible ? '显示' : '隐藏'}
          </button>
        </Field>
        <Field label="锁定">
          <button
            className={layer.locked ? 'mx-segment is-active' : 'mx-segment'}
            onClick={() => commandBus.execute({ type: 'outline.toggleLock', kind: 'pixel', id: layer.id })}
            type="button"
          >
            {layer.locked ? '已锁' : '未锁'}
          </button>
        </Field>
      </div>

      <TimingFields kind="pixel" id={layer.id} startFrame={layer.timing.startFrame} endFrame={layer.timing.endFrame} />

      <div className="mx-inspector-grid">
        <NumberField
          label="X"
          value={layer.x}
          onChange={(value) => commandBus.execute({ type: 'pixelLayer.update', id: layer.id, patch: { x: value } })}
        />
        <NumberField
          label="Y"
          value={layer.y}
          onChange={(value) => commandBus.execute({ type: 'pixelLayer.update', id: layer.id, patch: { y: value } })}
        />
        <NumberField
          label="不透明度"
          max={1}
          min={0}
          step={0.05}
          value={layer.opacity}
          onChange={(value) =>
            commandBus.execute({ type: 'pixelLayer.update', id: layer.id, patch: { opacity: value } })
          }
        />
        <NumberField
          label="层级"
          value={layer.zIndex}
          onChange={(value) =>
            commandBus.execute({ type: 'pixelLayer.update', id: layer.id, patch: { zIndex: value } })
          }
        />
      </div>

      <Field label="当前绘制颜色">
        <input
          onChange={(event) => commandBus.execute({ type: 'session.setDrawColor', color: event.target.value })}
          type="color"
          value={drawColor}
        />
      </Field>

      <div className="mx-inspector-actions">
        <button
          className="mx-toolchip"
          onClick={() => commandBus.execute({ type: 'pixelLayer.flip', id: layer.id, axis: 'horizontal' })}
          type="button"
        >
          水平镜像
        </button>
        <button
          className="mx-toolchip"
          onClick={() => commandBus.execute({ type: 'pixelLayer.flip', id: layer.id, axis: 'vertical' })}
          type="button"
        >
          垂直镜像
        </button>
      </div>
    </>
  )
}

function BackgroundInspector() {
  const document = useEditorStore((state) => state.document)
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const snapshot = useEditorStore((state) => state.snapshot)
  const layer = activeScene.backgroundLayers.find((item) => item.id === selectionId) ?? null
  const view = snapshot.backgrounds.find((item) => item.id === selectionId) ?? null

  if (!layer || !view) {
    return null
  }

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{layer.name}</strong>
        <span>背景层</span>
      </div>

      <Field label="名称">
        <input
          onChange={(event) => commandBus.execute({ type: 'outline.rename', kind: 'background', id: layer.id, name: event.target.value })}
          type="text"
          value={layer.name}
        />
      </Field>

      <Field label="绑定素材">
        <select
          onChange={(event) =>
            commandBus.execute({
              type: 'background.updateBase',
              id: layer.id,
              patch: { assetId: event.target.value },
            })
          }
          value={layer.assetId}
        >
          <option value="">纯色底板</option>
          {document.materials.map((asset) => (
            <option key={asset.id} value={asset.id}>
              {asset.label}
            </option>
          ))}
        </select>
      </Field>

      <Field label="底色">
        <input
          onChange={(event) =>
            commandBus.execute({
              type: 'background.updateBase',
              id: layer.id,
              patch: { baseColor: event.target.value },
            })
          }
          type="color"
          value={layer.baseColor}
        />
      </Field>

      <Field label="行为">
        <div className="mx-segmented">
          {(['fixed', 'loop', 'blink'] as const).map((behavior) => (
            <button
              key={behavior}
              className={view.state.behavior === behavior ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() =>
                commandBus.execute({
                  type: 'background.updateState',
                  id: layer.id,
                  patch: { behavior },
                })
              }
              type="button"
            >
              {getBackgroundBehaviorLabel(behavior)}
            </button>
          ))}
        </div>
      </Field>

      <div className="mx-inspector-grid">
        <NumberField label="X" value={Math.round(view.transform.x)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { x: value } })} />
        <NumberField label="Y" value={Math.round(view.transform.y)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { y: value } })} />
        <NumberField label="宽度" min={1} value={Math.round(view.transform.width)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { width: value } })} />
        <NumberField label="高度" min={1} value={Math.round(view.transform.height)} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { height: value } })} />
        <NumberField label="不透明度" max={1} min={0} step={0.05} value={view.transform.opacity} onChange={(value) => commandBus.execute({ type: 'background.updateTransform', id: layer.id, patch: { opacity: value } })} />
      </div>

      <div className="mx-inspector-grid">
        <NumberField label="序列步长" min={1} value={view.state.sequenceStepFrames} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { sequenceStepFrames: value } })} />
        <NumberField label="闪烁节奏" min={1} value={view.state.rhythmFrames} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { rhythmFrames: value } })} />
        <NumberField label="速度 X" value={view.state.speedX} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { speedX: value } })} />
        <NumberField label="速度 Y" value={view.state.speedY} onChange={(value) => commandBus.execute({ type: 'background.updateState', id: layer.id, patch: { speedY: value } })} />
      </div>
    </>
  )
}

function ObjectInspector() {
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const snapshot = useEditorStore((state) => state.snapshot)
  const objectGroup = activeScene.objectGroups.find((item) => item.id === selectionId) ?? null
  const view = snapshot.objects.find((item) => item.id === selectionId) ?? null

  if (!objectGroup || !view) {
    return null
  }

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{objectGroup.name}</strong>
        <span>对象组</span>
      </div>

      <Field label="名称">
        <input
          onChange={(event) => commandBus.execute({ type: 'outline.rename', kind: 'object', id: objectGroup.id, name: event.target.value })}
          type="text"
          value={objectGroup.name}
        />
      </Field>

      <TimingFields kind="object" id={objectGroup.id} startFrame={objectGroup.timing.startFrame} endFrame={objectGroup.timing.endFrame} />

      <Field label="行为">
        <div className="mx-segmented">
          {(['fixed', 'blink', 'path'] as const).map((behavior) => (
            <button
              key={behavior}
              className={view.state.behavior === behavior ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() =>
                commandBus.execute({
                  type: 'object.updateState',
                  id: objectGroup.id,
                  patch: { behavior },
                })
              }
              type="button"
            >
              {getObjectBehaviorLabel(behavior)}
            </button>
          ))}
        </div>
      </Field>

      <div className="mx-inspector-grid">
        <NumberField label="X" value={Math.round(view.transform.x)} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { x: value } })} />
        <NumberField label="Y" value={Math.round(view.transform.y)} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { y: value } })} />
        <NumberField label="缩放" min={0.2} step={0.05} value={view.transform.scale} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { scale: value } })} />
        <NumberField label="不透明度" max={1} min={0} step={0.05} value={view.transform.opacity} onChange={(value) => commandBus.execute({ type: 'object.updateTransform', id: objectGroup.id, patch: { opacity: value } })} />
      </div>

      <div className="mx-inspector-grid">
        <NumberField label="序列步长" min={1} value={view.state.sequenceStepFrames} onChange={(value) => commandBus.execute({ type: 'object.updateState', id: objectGroup.id, patch: { sequenceStepFrames: value } })} />
        <NumberField label="闪烁节奏" min={1} value={view.state.rhythmFrames} onChange={(value) => commandBus.execute({ type: 'object.updateState', id: objectGroup.id, patch: { rhythmFrames: value } })} />
      </div>

      <Field label="路径模式">
        <div className="mx-segmented">
          {(['loop', 'pingpong', 'once'] as const).map((mode) => (
            <button
              key={mode}
              className={objectGroup.motion.mode === mode ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() =>
                commandBus.execute({
                  type: 'object.updateMotion',
                  id: objectGroup.id,
                  patch: { mode },
                })
              }
              type="button"
            >
              {getPathModeLabel(mode)}
            </button>
          ))}
        </div>
      </Field>

      <div className="mx-inspector-grid">
        <NumberField
          label="路径持续帧"
          min={1}
          value={objectGroup.motion.durationFrames}
          onChange={(value) =>
            commandBus.execute({
              type: 'object.updateMotion',
              id: objectGroup.id,
              patch: { durationFrames: value },
            })
          }
        />
        <Field label="路径点">
          <div className="mx-readonly-field">{objectGroup.motion.points.length}</div>
        </Field>
      </div>

      <div className="mx-inspector-actions">
        <button
          className="mx-toolchip"
          onClick={() => commandBus.execute({ type: 'object.clearPath', id: objectGroup.id })}
          type="button"
        >
          清空路径
        </button>
      </div>
    </>
  )
}

function TimeInspector() {
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const widget = activeScene.timeWidgets.find((item) => item.id === selectionId) ?? null

  if (!widget) {
    return null
  }

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{widget.label}</strong>
        <span>时间组件</span>
      </div>

      <Field label="名称">
        <input
          onChange={(event) => commandBus.execute({ type: 'outline.rename', kind: 'time', id: widget.id, name: event.target.value })}
          type="text"
          value={widget.label}
        />
      </Field>

      <TimingFields kind="time" id={widget.id} startFrame={widget.timing.startFrame} endFrame={widget.timing.endFrame} />

      <Field label="类型">
        <select
          onChange={(event) =>
            commandBus.execute({
              type: 'time.update',
              id: widget.id,
              patch: { type: event.target.value as typeof widget.type },
            })
          }
          value={widget.type}
        >
          <option value="time">{getTimeTypeLabel('time')}</option>
          <option value="date">{getTimeTypeLabel('date')}</option>
          <option value="week">{getTimeTypeLabel('week')}</option>
          <option value="year">{getTimeTypeLabel('year')}</option>
        </select>
      </Field>

      <div className="mx-inspector-grid">
        <NumberField label="X" value={widget.x} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { x: value } })} />
        <NumberField label="Y" value={widget.y} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { y: value } })} />
        <NumberField label="缩放" min={1} value={widget.scale} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { scale: value } })} />
        <NumberField label="字间距" min={0} value={widget.letterSpacing} onChange={(value) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { letterSpacing: value } })} />
      </div>

      <Field label="颜色">
        <input
          onChange={(event) => commandBus.execute({ type: 'time.update', id: widget.id, patch: { color: event.target.value } })}
          type="color"
          value={widget.color}
        />
      </Field>

      <Field label="时间格式">
        <div className="mx-segmented">
          {[24, 12].map((hourFormat) => (
            <button
              key={hourFormat}
              className={widget.hourFormat === hourFormat ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() =>
                commandBus.execute({
                  type: 'time.update',
                  id: widget.id,
                  patch: { hourFormat: hourFormat as 12 | 24 },
                })
              }
              type="button"
            >
              {hourFormat}h
            </button>
          ))}
          <button
            className={widget.showSeconds ? 'mx-segment is-active' : 'mx-segment'}
            onClick={() =>
              commandBus.execute({
                type: 'time.update',
                id: widget.id,
                patch: { showSeconds: !widget.showSeconds },
              })
            }
            type="button"
          >
            {widget.showSeconds ? '隐藏秒' : '显示秒'}
          </button>
        </div>
      </Field>
    </>
  )
}

function MaterialInspector() {
  const document = useEditorStore((state) => state.document)
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const selectionId = useEditorStore((state) => state.session.selection.ids[0])
  const material = document.materials.find((item) => item.id === selectionId) ?? null

  if (!material) {
    return null
  }

  return (
    <>
      <div className="mx-inspector-head">
        <strong>{material.label}</strong>
        <span>素材</span>
      </div>

      <div className="mx-material-preview">
        <img alt={material.label} src={material.previewUrl} />
      </div>

      <div className="mx-inspector-grid">
        <Field label="类型">
          <div className="mx-readonly-field">{getMaterialKindLabel(material.kind)}</div>
        </Field>
        <Field label="尺寸">
          <div className="mx-readonly-field">
            {material.width} x {material.height}
          </div>
        </Field>
      </div>

      <div className="mx-inspector-actions">
        <button
          className="mx-toolchip"
          onClick={() =>
            commandBus.execute({
              type: 'scene.insertMaterialAsBackground',
              sceneId: activeScene.id,
              materialId: material.id,
            })
          }
          type="button"
        >
          插入背景层
        </button>
        <button
          className="mx-toolchip"
          onClick={() =>
            commandBus.execute({
              type: 'scene.insertMaterialAsObject',
              sceneId: activeScene.id,
              materialId: material.id,
            })
          }
          type="button"
        >
          插入对象组
        </button>
        <button
          className="mx-toolchip danger"
          onClick={() => commandBus.execute({ type: 'material.remove', id: material.id })}
          type="button"
        >
          删除素材
        </button>
      </div>
    </>
  )
}

export function InspectorPanel() {
  const selectionKind = useEditorStore((state) => state.session.selection.kind)

  return (
    <section className="mx-pane mx-inspector-pane">
      {selectionKind === 'scene' ? <SceneInspector /> : null}
      {selectionKind === 'pixel' ? <PixelLayerInspector /> : null}
      {selectionKind === 'background' ? <BackgroundInspector /> : null}
      {selectionKind === 'object' ? <ObjectInspector /> : null}
      {selectionKind === 'time' ? <TimeInspector /> : null}
      {selectionKind === 'material' ? <MaterialInspector /> : null}
    </section>
  )
}

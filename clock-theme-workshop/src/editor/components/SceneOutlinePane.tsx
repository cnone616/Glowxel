import { useMemo, useState } from 'react'
import type { MouseEvent } from 'react'
import { commandBus } from '../core/commandBus'
import { useEditorStore } from '../state/editorStore'
import { VirtualList } from './VirtualList'

type OutlineRow =
  | { type: 'header'; id: string; label: string; count: number; hint: string }
  | {
      type: 'item'
      id: string
      kind: 'background' | 'object' | 'time'
      label: string
      detail: string
      positionLabel: string
      visible: boolean
      locked: boolean
      kindLabel: string
      canMoveUp: boolean
      canMoveDown: boolean
    }

function OutlineActionButton({
  children,
  disabled,
  onClick,
  title,
}: {
  children: string
  disabled?: boolean
  onClick: (event: MouseEvent<HTMLButtonElement>) => void
  title: string
}) {
  return (
    <button
      className="outline-action-button"
      disabled={disabled}
      onClick={onClick}
      title={title}
      type="button"
    >
      {children}
    </button>
  )
}

export function SceneOutlinePane() {
  const document = useEditorStore((state) => state.document)
  const snapshot = useEditorStore((state) => state.snapshot)
  const selection = useEditorStore((state) => state.session.selection)
  const currentFrame = useEditorStore((state) => state.session.currentFrame)
  const [editingId, setEditingId] = useState('')
  const [editingValue, setEditingValue] = useState('')

  const rows = useMemo<OutlineRow[]>(() => {
    const backgroundRows: OutlineRow[] = document.backgroundLayers.map((item, index) => {
      const view = snapshot.backgrounds.find((entry) => entry.id === item.id)
      return {
        type: 'item',
        id: item.id,
        kind: 'background',
        label: item.name,
        detail: view ? `背景 · ${view.state.behavior}` : '背景',
        positionLabel: view ? `${Math.round(view.transform.x)}, ${Math.round(view.transform.y)}` : '--',
        visible: view ? view.state.visible : false,
        locked: item.locked,
        kindLabel: 'BG',
        canMoveUp: index > 0,
        canMoveDown: index < document.backgroundLayers.length - 1,
      }
    })

    const objectRows: OutlineRow[] = document.objectGroups.map((item, index) => {
      const view = snapshot.objects.find((entry) => entry.id === item.id)
      return {
        type: 'item',
        id: item.id,
        kind: 'object',
        label: item.name,
        detail: view ? `对象 · ${view.state.behavior}` : '对象',
        positionLabel: view ? `${Math.round(view.transform.x)}, ${Math.round(view.transform.y)}` : '--',
        visible: view ? view.state.visible : false,
        locked: item.locked,
        kindLabel: 'OBJ',
        canMoveUp: index > 0,
        canMoveDown: index < document.objectGroups.length - 1,
      }
    })

    const timeRows: OutlineRow[] = document.timeWidgets.map((item, index) => ({
      type: 'item',
      id: item.id,
      kind: 'time',
      label: item.label,
      detail: `时间 · ${item.type}`,
      positionLabel: `${Math.round(item.x)}, ${Math.round(item.y)}`,
      visible: item.visible,
      locked: item.locked,
      kindLabel: 'TIME',
      canMoveUp: index > 0,
      canMoveDown: index < document.timeWidgets.length - 1,
    }))

    return [
      { type: 'header', id: 'header-backgrounds', label: '背景层', count: document.backgroundLayers.length, hint: '舞台底板与铺底内容' },
      ...backgroundRows,
      { type: 'header', id: 'header-objects', label: '对象组', count: document.objectGroups.length, hint: '主动画与摆放元素' },
      ...objectRows,
      { type: 'header', id: 'header-time', label: '时间组件', count: document.timeWidgets.length, hint: '时钟、日期与文本组件' },
      ...timeRows,
    ]
  }, [currentFrame, document.backgroundLayers, document.objectGroups, document.timeWidgets, snapshot.backgrounds, snapshot.objects])

  const commitRename = () => {
    const row = rows.find(
      (entry): entry is Extract<OutlineRow, { type: 'item' }> =>
        entry.type === 'item' && entry.id === editingId,
    )
    if (!row || editingValue.trim().length === 0) {
      setEditingId('')
      setEditingValue('')
      return
    }

    commandBus.execute({
      type: 'outline.rename',
      kind: row.kind,
      id: row.id,
      name: editingValue.trim(),
    })
    setEditingId('')
    setEditingValue('')
  }

  return (
    <section className="pane-section scene-outline-pane">
      <div className="pane-head">
        <div>
          <span className="eyebrow-label">Scene</span>
          <h2>场景结构</h2>
        </div>
      </div>

      <div className="pane-controls outline-summary-strip">
        <div className="summary-pill">
          <strong>背景</strong>
          <span>{document.backgroundLayers.length}</span>
        </div>
        <div className="summary-pill">
          <strong>对象</strong>
          <span>{document.objectGroups.length}</span>
        </div>
        <div className="summary-pill">
          <strong>时间</strong>
          <span>{document.timeWidgets.length}</span>
        </div>
        <div className="summary-pill">
          <strong>当前帧</strong>
          <span>{currentFrame + 1}</span>
        </div>
      </div>

      <VirtualList
        className="virtual-pane-list outline-list"
        estimateSize={68}
        itemKey={(row) => row.id}
        items={rows}
        renderItem={(row) => {
          if (row.type === 'header') {
            return (
              <div className="outline-group-header">
                <div>
                  <strong>{row.label}</strong>
                  <span>{row.hint}</span>
                </div>
                <span className="outline-group-count">{row.count}</span>
              </div>
            )
          }

          const itemRow = row
          const selected = selection.kind === itemRow.kind && selection.ids.includes(itemRow.id)

          return (
            <div
              className={selected ? 'outline-row active' : 'outline-row'}
              data-kind={itemRow.kind}
              onClick={(event) =>
                commandBus.execute({
                  type: 'selection.set',
                  kind: itemRow.kind,
                  ids: [itemRow.id],
                  additive: event.shiftKey,
                })
              }
              onDoubleClick={() => {
                setEditingId(itemRow.id)
                setEditingValue(itemRow.label)
              }}
            >
              <div className="outline-kind-chip">{itemRow.kindLabel}</div>

              <div className="outline-main">
                {editingId === itemRow.id ? (
                  <input
                    autoFocus
                    onBlur={commitRename}
                    onChange={(event) => setEditingValue(event.target.value)}
                    onKeyDown={(event) => {
                      if (event.key === 'Enter') {
                        commitRename()
                      }
                      if (event.key === 'Escape') {
                        setEditingId('')
                        setEditingValue('')
                      }
                    }}
                    type="text"
                    value={editingValue}
                  />
                ) : (
                  <>
                    <div className="outline-title-line">
                      <strong>{itemRow.label}</strong>
                      {!itemRow.visible ? <span className="outline-state-chip muted">隐藏</span> : null}
                      {itemRow.locked ? <span className="outline-state-chip warning">锁定</span> : null}
                    </div>
                    <div className="outline-meta-line">
                      <span>{itemRow.detail}</span>
                      <span>{itemRow.positionLabel}</span>
                    </div>
                  </>
                )}
              </div>

              <div className="outline-actions">
                <OutlineActionButton
                  onClick={(event) => {
                    event.stopPropagation()
                    commandBus.execute({
                      type: 'outline.toggleVisibility',
                      kind: itemRow.kind,
                      id: itemRow.id,
                    })
                  }}
                  title={itemRow.visible ? '隐藏图层' : '显示图层'}
                >
                  {itemRow.visible ? '显' : '隐'}
                </OutlineActionButton>
                <OutlineActionButton
                  onClick={(event) => {
                    event.stopPropagation()
                    commandBus.execute({
                      type: 'outline.toggleLock',
                      kind: itemRow.kind,
                      id: itemRow.id,
                    })
                  }}
                  title={itemRow.locked ? '解锁图层' : '锁定图层'}
                >
                  {itemRow.locked ? '锁' : '开'}
                </OutlineActionButton>
                <OutlineActionButton
                  onClick={(event) => {
                    event.stopPropagation()
                    setEditingId(itemRow.id)
                    setEditingValue(itemRow.label)
                  }}
                  title="重命名"
                >
                  名
                </OutlineActionButton>
                <OutlineActionButton
                  disabled={!itemRow.canMoveUp}
                  onClick={(event) => {
                    event.stopPropagation()
                    commandBus.execute({
                      type: 'outline.reorder',
                      kind: itemRow.kind,
                      id: itemRow.id,
                      direction: 'up',
                    })
                  }}
                  title="上移"
                >
                  ↑
                </OutlineActionButton>
                <OutlineActionButton
                  disabled={!itemRow.canMoveDown}
                  onClick={(event) => {
                    event.stopPropagation()
                    commandBus.execute({
                      type: 'outline.reorder',
                      kind: itemRow.kind,
                      id: itemRow.id,
                      direction: 'down',
                    })
                  }}
                  title="下移"
                >
                  ↓
                </OutlineActionButton>
              </div>
            </div>
          )
        }}
      />
    </section>
  )
}

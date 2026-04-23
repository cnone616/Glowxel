import * as Tooltip from '@radix-ui/react-tooltip'
import { commandBus } from '../core/commandBus'
import { useEditorStore } from '../state/editorStore'

const TOOL_ITEMS = [
  { id: 'select', label: '选择', hint: '点选、框选、拖动选中对象' },
  { id: 'pan', label: '平移', hint: '拖动画布视口' },
  { id: 'place', label: '摆放', hint: '点击舞台放置当前对象' },
  { id: 'path', label: '路径', hint: '记录并编辑对象路径点' },
] as const

export function ToolModeBar() {
  const currentTool = useEditorStore((state) => state.session.currentTool)
  const isPlaying = useEditorStore((state) => state.session.isPlaying)
  const viewportZoom = useEditorStore((state) => state.session.viewportZoom)
  const viewportMode = useEditorStore((state) => state.session.viewportMode)

  return (
    <Tooltip.Provider delayDuration={120}>
      <div className="tool-mode-bar">
        <div className="tool-cluster">
          {TOOL_ITEMS.map((tool) => (
            <Tooltip.Root key={tool.id}>
              <Tooltip.Trigger asChild>
                <button
                  className={currentTool === tool.id ? 'tool-button active' : 'tool-button'}
                  onClick={() =>
                    commandBus.execute({
                      type: 'session.setTool',
                      tool: tool.id,
                    })
                  }
                  type="button"
                >
                  {tool.label}
                </button>
              </Tooltip.Trigger>
              <Tooltip.Portal>
                <Tooltip.Content className="tooltip-content" side="bottom">
                  {tool.hint}
                </Tooltip.Content>
              </Tooltip.Portal>
            </Tooltip.Root>
          ))}
        </div>

        <div className="tool-cluster">
          <button className="tool-button emphasized" onClick={() => commandBus.execute({ type: 'session.togglePlay' })} type="button">
            {isPlaying ? '暂停' : '播放'}
          </button>
          <button className="tool-button" onClick={() => commandBus.execute({ type: 'session.fitViewport' })} type="button">
            适配
          </button>
          {[1, 2, 4].map((zoom) => (
            <button
              key={zoom}
              className={viewportMode === 'manual' && viewportZoom === zoom ? 'tool-button active' : 'tool-button'}
              onClick={() => commandBus.execute({ type: 'session.setZoomPreset', zoom })}
              type="button"
            >
              {zoom}x
            </button>
          ))}
        </div>
      </div>
    </Tooltip.Provider>
  )
}

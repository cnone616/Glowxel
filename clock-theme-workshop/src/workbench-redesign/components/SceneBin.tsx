import { buildProjectAssetFromCatalogEntry, buildProjectAssetFromCatalogGroup } from '../../editor/core/catalog'
import { commandBus } from '../../editor/core/commandBus'
import {
  DEFAULT_BACKGROUND_STATE,
  DEFAULT_GROUP_STATE,
  getActiveScene,
  isLayerVisibleAtFrame,
  resolveSceneFrame,
  sampleStepKeys,
} from '../../editor/core/themeDocument'
import { useEditorStore } from '../../editor/state/editorStore'
import { UploadDialog } from './UploadDialog'

function getMaterialKindLabel(value: 'catalog-entry' | 'catalog-sequence' | 'uploaded-image'): string {
  if (value === 'catalog-entry') {
    return '目录单帧'
  }
  if (value === 'catalog-sequence') {
    return '目录序列'
  }
  return '上传图片'
}

function getTransitionLabel(value: 'cut' | 'fade'): string {
  return value === 'cut' ? '直接切换' : '淡入淡出'
}

function LayerRow({
  kind,
  id,
  label,
  selected,
  visible,
  locked,
}: {
  kind: 'background' | 'pixel' | 'object' | 'time'
  id: string
  label: string
  selected: boolean
  visible: boolean
  locked: boolean
}) {
  return (
    <div className={selected ? 'mx-layer-row is-active' : 'mx-layer-row'}>
      <button
        className="mx-layer-main"
        onClick={() => commandBus.execute({ type: 'selection.set', kind, ids: [id], additive: false })}
        type="button"
      >
        {label}
      </button>
      <button
        className={visible ? 'mx-layer-action is-active' : 'mx-layer-action'}
        onClick={() => commandBus.execute({ type: 'outline.toggleVisibility', kind, id })}
        type="button"
      >
        {visible ? '可见' : '隐藏'}
      </button>
      <button
        className={locked ? 'mx-layer-action is-active' : 'mx-layer-action'}
        onClick={() => commandBus.execute({ type: 'outline.toggleLock', kind, id })}
        type="button"
      >
        {locked ? '已锁' : '未锁'}
      </button>
    </div>
  )
}

export function SceneBin() {
  const document = useEditorStore((state) => state.document)
  const catalog = useEditorStore((state) => state.catalog)
  const session = useEditorStore((state) => state.session)
  const activeScene = useEditorStore((state) => getActiveScene(state.document, state.session.activeSceneId))
  const localFrame = useEditorStore((state) => {
    const resolved = resolveSceneFrame(state.document, state.session.currentFrame)
    return resolved.scene.id === state.session.activeSceneId ? resolved.localFrame : 0
  })

  const currentCollection = catalog.manifest?.collections.find(
    (collection) => collection.id === session.selectedCollectionId,
  )
  const currentSource = currentCollection?.sourceAssets.find(
    (source) => source.id === session.selectedSourceAssetId,
  )
  const currentNormalized =
    currentCollection && currentSource
      ? catalog.sourceCache[`${currentCollection.id}:${currentSource.id}`]
      : null

  const search = session.assetSearch.trim().toLowerCase()
  const filteredMaterials = document.materials.filter((item) => {
    if (search.length === 0) {
      return true
    }
    return item.label.toLowerCase().includes(search)
  })
  const filteredEntries =
    currentNormalized?.entries.filter((entry) => {
      if (search.length === 0) {
        return true
      }
      return entry.label.toLowerCase().includes(search)
    }) ?? []
  const filteredGroups =
    currentNormalized?.groups.filter((group) => {
      if (group.id === 'all') {
        return false
      }
      if (search.length === 0) {
        return true
      }
      return group.label.toLowerCase().includes(search)
    }) ?? []

  return (
    <section className="mx-pane mx-scene-bin">
      <div className="mx-sidebar-tabs">
        <button
          className={session.activeSidebarTab === 'materials' ? 'mx-segment is-active' : 'mx-segment'}
          onClick={() => commandBus.execute({ type: 'session.setSidebarTab', tab: 'materials' })}
          type="button"
        >
          素材库
        </button>
        <button
          className={session.activeSidebarTab === 'scenes' ? 'mx-segment is-active' : 'mx-segment'}
          onClick={() => commandBus.execute({ type: 'session.setSidebarTab', tab: 'scenes' })}
          type="button"
        >
          场景
        </button>
      </div>

      {session.activeSidebarTab === 'materials' ? (
        <div className="mx-sidebar-scroll">
          <div className="mx-panel-strip">
            <button
              className="mx-toolchip"
              onClick={() => commandBus.execute({ type: 'session.setUploadDialogOpen', open: true })}
              type="button"
            >
              上传图片
            </button>
            <input
              className="mx-search-input"
              onChange={(event) => commandBus.execute({ type: 'session.setAssetSearch', value: event.target.value })}
              placeholder="搜索素材"
              type="text"
              value={session.assetSearch}
            />
          </div>

          <div className="mx-panel-strip">
            <select
              onChange={(event) => commandBus.execute({ type: 'session.setCollection', collectionId: event.target.value })}
              value={session.selectedCollectionId}
            >
              {catalog.manifest?.collections.map((collection) => (
                <option key={collection.id} value={collection.id}>
                  {collection.label}
                </option>
              ))}
            </select>
            <select
              onChange={(event) => commandBus.execute({ type: 'session.setSourceAsset', sourceAssetId: event.target.value })}
              value={session.selectedSourceAssetId}
            >
              {currentCollection?.sourceAssets.map((source) => (
                <option key={source.id} value={source.id}>
                  {source.description}
                </option>
              ))}
            </select>
          </div>

          <div className="mx-panel-strip">
            <button
              className={session.assetBrowserMode === 'entries' ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() => commandBus.execute({ type: 'session.setAssetMode', mode: 'entries' })}
              type="button"
            >
              条目
            </button>
            <button
              className={session.assetBrowserMode === 'groups' ? 'mx-segment is-active' : 'mx-segment'}
              onClick={() => commandBus.execute({ type: 'session.setAssetMode', mode: 'groups' })}
              type="button"
            >
              分组
            </button>
          </div>

          <div className="mx-sidebar-section">
            <div className="mx-section-title">我的素材</div>
            <div className="mx-material-list">
              {filteredMaterials.map((asset) => (
                <article
                  key={asset.id}
                  className={session.selectedMaterialId === asset.id ? 'mx-material-row is-active' : 'mx-material-row'}
                >
                  <button
                    className="mx-material-main"
                    onClick={() => commandBus.execute({ type: 'material.select', id: asset.id })}
                    type="button"
                  >
                    <img alt={asset.label} className="mx-material-thumb" src={asset.previewUrl} />
                    <div className="mx-material-copy">
                      <strong>{asset.label}</strong>
                      <span>
                        {getMaterialKindLabel(asset.kind)} / {asset.width}x{asset.height}
                      </span>
                    </div>
                  </button>
                  <div className="mx-material-actions">
                    <button
                      className="mx-layer-action"
                      onClick={() =>
                        commandBus.execute({
                          type: 'scene.insertMaterialAsBackground',
                          sceneId: activeScene.id,
                          materialId: asset.id,
                        })
                      }
                      type="button"
                    >
                      插入背景
                    </button>
                    <button
                      className="mx-layer-action"
                      onClick={() =>
                        commandBus.execute({
                          type: 'scene.insertMaterialAsObject',
                          sceneId: activeScene.id,
                          materialId: asset.id,
                        })
                      }
                      type="button"
                    >
                      插入对象
                    </button>
                    <button
                      className="mx-layer-action danger"
                      onClick={() => commandBus.execute({ type: 'material.remove', id: asset.id })}
                      type="button"
                    >
                      删除
                    </button>
                  </div>
                </article>
              ))}
            </div>
          </div>

          <div className="mx-sidebar-section">
            <div className="mx-section-title">目录素材</div>
            <div className="mx-material-list">
              {catalog.loading && !currentNormalized ? <div className="mx-empty">目录加载中…</div> : null}
              {catalog.error ? <div className="mx-empty">{catalog.error}</div> : null}
              {currentNormalized
                ? (session.assetBrowserMode === 'entries' ? filteredEntries : filteredGroups).map((item) => {
                    const asset =
                      session.assetBrowserMode === 'entries'
                        ? buildProjectAssetFromCatalogEntry(currentNormalized, item.id)
                        : buildProjectAssetFromCatalogGroup(currentNormalized, item.id)
                    return (
                      <article key={item.id} className="mx-material-row">
                        <div className="mx-material-main">
                          <img alt={asset.label} className="mx-material-thumb" src={asset.previewUrl} />
                          <div className="mx-material-copy">
                            <strong>{asset.label}</strong>
                            <span>
                              {session.assetBrowserMode === 'entries' ? '目录单帧' : '目录序列'}
                            </span>
                          </div>
                        </div>
                        <div className="mx-material-actions">
                          <button
                            className="mx-layer-action"
                            onClick={() => commandBus.execute({ type: 'material.addCatalog', asset })}
                            type="button"
                          >
                            入库
                          </button>
                        </div>
                      </article>
                    )
                  })
                : null}
            </div>
          </div>
        </div>
      ) : (
        <div className="mx-sidebar-scroll">
          <div className="mx-panel-strip">
            <button className="mx-toolchip" onClick={() => commandBus.execute({ type: 'scene.add' })} type="button">
              新建场景
            </button>
            <button
              className="mx-toolchip"
              onClick={() => commandBus.execute({ type: 'scene.duplicate', id: activeScene.id })}
              type="button"
            >
              复制场景
            </button>
          </div>

          <div className="mx-sidebar-section">
            <div className="mx-section-title">场景列表</div>
            <div className="mx-scene-list">
              {document.scenes.map((scene, index) => (
                <article
                  key={scene.id}
                  className={scene.id === activeScene.id ? 'mx-scene-card is-active' : 'mx-scene-card'}
                  draggable
                  onDragStart={(event) => event.dataTransfer.setData('text/plain', scene.id)}
                  onDragOver={(event) => event.preventDefault()}
                  onDrop={(event) => {
                    event.preventDefault()
                    const draggedId = event.dataTransfer.getData('text/plain')
                    if (draggedId) {
                      commandBus.execute({ type: 'scene.reorder', id: draggedId, toIndex: index })
                    }
                  }}
                >
                  <button className="mx-scene-main" onClick={() => commandBus.execute({ type: 'scene.select', id: scene.id })} type="button">
                    <div className="mx-scene-title">{scene.name}</div>
                    <div className="mx-scene-subtitle">
                      {scene.durationFrames}f / {getTransitionLabel(scene.transitionOut)}
                    </div>
                  </button>
                  <div className="mx-material-actions">
                    <button className="mx-layer-action" onClick={() => commandBus.execute({ type: 'scene.duplicate', id: scene.id })} type="button">
                      复制
                    </button>
                    <button className="mx-layer-action danger" onClick={() => commandBus.execute({ type: 'scene.remove', id: scene.id })} type="button">
                      删除
                    </button>
                  </div>
                </article>
              ))}
            </div>
          </div>

          <div className="mx-sidebar-section">
            <div className="mx-section-title">当前场景图层</div>
            <div className="mx-layer-group">
              <div className="mx-layer-group-title">背景层</div>
              {activeScene.backgroundLayers.map((layer) => (
                <LayerRow
                  key={layer.id}
                  id={layer.id}
                  kind="background"
                  label={layer.name}
                  locked={layer.locked}
                  selected={session.selection.kind === 'background' && session.selection.ids[0] === layer.id}
                  visible={sampleStepKeys(layer.stateKeys, localFrame, DEFAULT_BACKGROUND_STATE).visible}
                />
              ))}
            </div>

            <div className="mx-layer-group">
              <div className="mx-layer-group-title">像素层</div>
              {activeScene.pixelLayers.map((layer) => (
                <LayerRow
                  key={layer.id}
                  id={layer.id}
                  kind="pixel"
                  label={layer.name}
                  locked={layer.locked}
                  selected={session.selection.kind === 'pixel' && session.selection.ids[0] === layer.id}
                  visible={layer.visible && isLayerVisibleAtFrame(layer.timing, localFrame)}
                />
              ))}
            </div>

            <div className="mx-layer-group">
              <div className="mx-layer-group-title">对象组</div>
              {activeScene.objectGroups.map((group) => (
                <LayerRow
                  key={group.id}
                  id={group.id}
                  kind="object"
                  label={group.name}
                  locked={group.locked}
                  selected={session.selection.kind === 'object' && session.selection.ids[0] === group.id}
                  visible={
                    sampleStepKeys(group.stateKeys, localFrame, DEFAULT_GROUP_STATE).visible &&
                    isLayerVisibleAtFrame(group.timing, localFrame)
                  }
                />
              ))}
            </div>

            <div className="mx-layer-group">
              <div className="mx-layer-group-title">时间组件</div>
              {activeScene.timeWidgets.map((widget) => (
                <LayerRow
                  key={widget.id}
                  id={widget.id}
                  kind="time"
                  label={widget.label}
                  locked={widget.locked}
                  selected={session.selection.kind === 'time' && session.selection.ids[0] === widget.id}
                  visible={widget.visible && isLayerVisibleAtFrame(widget.timing, localFrame)}
                />
              ))}
            </div>
          </div>
        </div>
      )}

      <UploadDialog />
    </section>
  )
}

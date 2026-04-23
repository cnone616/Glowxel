import * as Tabs from '@radix-ui/react-tabs'
import { buildProjectAssetFromCatalogEntry, buildProjectAssetFromCatalogGroup, buildThumbnailDataUrl } from '../core/catalog'
import { commandBus } from '../core/commandBus'
import { useEditorStore } from '../state/editorStore'
import { VirtualList } from './VirtualList'

export function AssetBrowserPane() {
  const catalog = useEditorStore((state) => state.catalog)
  const session = useEditorStore((state) => state.session)
  const currentCollection = catalog.manifest?.collections.find(
    (collection) => collection.id === session.selectedCollectionId,
  )
  const currentSource = currentCollection?.sourceAssets.find(
    (source) => source.id === session.selectedSourceAssetId,
  )
  const currentNormalized = currentCollection && currentSource
    ? catalog.sourceCache[`${currentCollection.id}:${currentSource.id}`]
    : null

  const search = session.assetSearch.trim().toLowerCase()
  const entries = currentNormalized
    ? currentNormalized.entries.filter((entry) => {
        if (search.length === 0) {
          return true
        }
        return entry.id.toLowerCase().includes(search)
      })
    : []
  const groups = currentNormalized
    ? currentNormalized.groups.filter((group) => {
        if (group.id === 'all') {
          return false
        }
        if (search.length === 0) {
          return true
        }
        return group.id.toLowerCase().includes(search)
      })
    : []

  return (
    <section className="pane-section asset-browser-pane">
      <div className="pane-head">
        <div>
          <span className="eyebrow-label">Assets</span>
          <h2>素材浏览</h2>
        </div>
        <button className="solid-inline" onClick={() => commandBus.execute({ type: 'session.setUploadDialogOpen', open: true })} type="button">
          上传
        </button>
      </div>

      <div className="pane-controls">
        <label className="field-label">
          <span>入场景目标</span>
          <div className="segmented-row">
            <button
              className={session.assetTarget === 'background' ? 'segment-button active' : 'segment-button'}
              onClick={() => commandBus.execute({ type: 'session.setAssetTarget', target: 'background' })}
              type="button"
            >
              背景
            </button>
            <button
              className={session.assetTarget === 'object' ? 'segment-button active' : 'segment-button'}
              onClick={() => commandBus.execute({ type: 'session.setAssetTarget', target: 'object' })}
              type="button"
            >
              对象
            </button>
          </div>
        </label>

        <label className="field-label">
          <span>素材集</span>
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
        </label>

        <label className="field-label">
          <span>素材源</span>
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
        </label>

        <label className="field-label">
          <span>搜索</span>
          <input
            onChange={(event) => commandBus.execute({ type: 'session.setAssetSearch', value: event.target.value })}
            placeholder="搜索 frame、tile、chunk"
            type="text"
            value={session.assetSearch}
          />
        </label>
      </div>

      <Tabs.Root
        className="asset-tabs"
        onValueChange={(value) =>
          commandBus.execute({
            type: 'session.setAssetMode',
            mode: value === 'groups' ? 'groups' : 'entries',
          })
        }
        value={session.assetBrowserMode}
      >
        <Tabs.List className="asset-tab-list">
          <Tabs.Trigger className="asset-tab-trigger" value="entries">
            条目
          </Tabs.Trigger>
          <Tabs.Trigger className="asset-tab-trigger" value="groups">
            分组
          </Tabs.Trigger>
        </Tabs.List>

        <Tabs.Content className="asset-tab-content" value="entries">
          {catalog.loading ? <div className="empty-state">正在加载素材目录…</div> : null}
          {catalog.error.length > 0 ? <div className="empty-state danger">{catalog.error}</div> : null}
          {!catalog.loading && catalog.error.length === 0 ? (
            <VirtualList
              className="virtual-pane-list"
              estimateSize={92}
              itemKey={(entry) => entry.id}
              items={entries}
              renderItem={(entry) => (
                <article className="asset-row">
                  <img alt={entry.id} className="asset-thumb" src={buildThumbnailDataUrl({
                    id: entry.id,
                    width: entry.width,
                    height: entry.height,
                    pixels: entry.pixels,
                  })} />
                  <div className="asset-row-body">
                    <strong>{entry.label}</strong>
                    <span>
                      {entry.width}×{entry.height}
                    </span>
                  </div>
                  <button
                    className="ghost-inline"
                    onClick={() => {
                      if (!currentNormalized) {
                        return
                      }
                      const asset = buildProjectAssetFromCatalogEntry(
                        currentNormalized,
                        entry.id,
                        session.assetTarget,
                      )
                      commandBus.execute({
                        type: 'asset.addCatalog',
                        asset,
                        target: session.assetTarget,
                      })
                    }}
                    type="button"
                  >
                    {session.assetTarget === 'background' ? '建背景' : '建对象'}
                  </button>
                </article>
              )}
            />
          ) : null}
        </Tabs.Content>

        <Tabs.Content className="asset-tab-content" value="groups">
          {catalog.loading ? <div className="empty-state">正在加载素材目录…</div> : null}
          {catalog.error.length > 0 ? <div className="empty-state danger">{catalog.error}</div> : null}
          {!catalog.loading && catalog.error.length === 0 ? (
            <VirtualList
              className="virtual-pane-list"
              estimateSize={92}
              itemKey={(group) => group.id}
              items={groups}
              renderItem={(group) => (
                <article className="asset-row">
                  <img
                    alt={group.label}
                    className="asset-thumb"
                    src={
                      currentNormalized
                        ? buildThumbnailDataUrl({
                            id: group.entryIds[0],
                            width: currentNormalized.entryMap[group.entryIds[0]].width,
                            height: currentNormalized.entryMap[group.entryIds[0]].height,
                            pixels: currentNormalized.entryMap[group.entryIds[0]].pixels,
                          })
                        : ''
                    }
                  />
                  <div className="asset-row-body">
                    <strong>{group.label}</strong>
                    <span>{group.entryIds.length} 帧</span>
                  </div>
                  <button
                    className="ghost-inline"
                    onClick={() => {
                      if (!currentNormalized) {
                        return
                      }
                      const asset = buildProjectAssetFromCatalogGroup(
                        currentNormalized,
                        group.id,
                        session.assetTarget,
                      )
                      commandBus.execute({
                        type: 'asset.addCatalog',
                        asset,
                        target: session.assetTarget,
                      })
                    }}
                    type="button"
                  >
                    {session.assetTarget === 'background' ? '建背景' : '建对象'}
                  </button>
                </article>
              )}
            />
          ) : null}
        </Tabs.Content>
      </Tabs.Root>
    </section>
  )
}

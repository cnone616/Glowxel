import { useEffect } from 'react'
import { loadAssetCatalog, loadCatalogSourceAsset } from '../core/catalog'
import { loadStoredDocument } from '../core/storage'
import { createDefaultDocument } from '../core/themeDocument'
import { useEditorStore } from '../state/editorStore'

export function useEditorBootstrap(): void {
  useEffect(() => {
    let cancelled = false

    const bootstrap = async () => {
      const store = useEditorStore.getState()

      try {
        const [catalogManifest, storedDocument] = await Promise.all([
          loadAssetCatalog(),
          loadStoredDocument(),
        ])

        if (cancelled) {
          return
        }

        store.setCatalogState({
          loading: false,
          error: '',
          manifest: catalogManifest,
        })

        const initialCollection = catalogManifest.collections[0]
        const initialSource = initialCollection?.sourceAssets[0]
        if (initialCollection && initialSource) {
          store.updateSession({
            selectedCollectionId: initialCollection.id,
            selectedSourceAssetId: initialSource.id,
          })

          const normalized = await loadCatalogSourceAsset(initialCollection.id, initialSource)
          if (cancelled) {
            return
          }
          store.cacheCatalogSource(`${initialCollection.id}:${initialSource.id}`, normalized)
        }

        store.hydrateDocument(storedDocument ?? createDefaultDocument())
      } catch (error) {
        if (cancelled) {
          return
        }

        store.setCatalogState({
          loading: false,
          error: error instanceof Error ? error.message : '素材目录加载失败',
        })
        store.hydrateDocument(createDefaultDocument())
      }
    }

    void bootstrap()

    return () => {
      cancelled = true
    }
  }, [])
}

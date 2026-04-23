import type { ThemeDocument } from '../types'
import { ensureThemeDocument } from './themeDocument'

const DATABASE_NAME = 'clock-theme-workshop-v2'
const STORE_NAME = 'documents'
const CURRENT_DOCUMENT_KEY = 'workbench-current-document'

function openDatabase(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const request = window.indexedDB.open(DATABASE_NAME, 1)

    request.onupgradeneeded = () => {
      const database = request.result
      if (!database.objectStoreNames.contains(STORE_NAME)) {
        database.createObjectStore(STORE_NAME)
      }
    }

    request.onsuccess = () => resolve(request.result)
    request.onerror = () => reject(request.error ?? new Error('IndexedDB 打开失败'))
  })
}

export async function loadStoredDocument(): Promise<ThemeDocument | null> {
  const database = await openDatabase()

  return new Promise((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readonly')
    const store = transaction.objectStore(STORE_NAME)
    const request = store.get(CURRENT_DOCUMENT_KEY)

    request.onsuccess = () => {
      const result = request.result as unknown
      if (!result) {
        resolve(null)
        return
      }

      try {
        resolve(ensureThemeDocument(result))
      } catch (error) {
        reject(error)
      }
    }
    request.onerror = () => reject(request.error ?? new Error('读取本地文档失败'))
  })
}

export async function saveStoredDocument(document: ThemeDocument): Promise<void> {
  const database = await openDatabase()

  return new Promise((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readwrite')
    const store = transaction.objectStore(STORE_NAME)
    const request = store.put(document, CURRENT_DOCUMENT_KEY)

    request.onsuccess = () => resolve()
    request.onerror = () => reject(request.error ?? new Error('保存本地文档失败'))
  })
}

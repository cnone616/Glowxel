const DATABASE_NAME = 'clock-theme-workshop-v1'
const STORE_NAME = 'scenes'
const CURRENT_SCENE_KEY = 'current-scene'

function openDatabase() {
  return new Promise((resolve, reject) => {
    const request = window.indexedDB.open(DATABASE_NAME, 1)

    request.onupgradeneeded = () => {
      const database = request.result
      if (database.objectStoreNames.contains(STORE_NAME) === false) {
        database.createObjectStore(STORE_NAME)
      }
    }

    request.onsuccess = () => resolve(request.result)
    request.onerror = () => reject(request.error ?? new Error('IndexedDB 打开失败'))
  })
}

export async function loadStoredScene() {
  const database = await openDatabase()

  return new Promise((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readonly')
    const store = transaction.objectStore(STORE_NAME)
    const request = store.get(CURRENT_SCENE_KEY)

    request.onsuccess = () => resolve(request.result ?? null)
    request.onerror = () => reject(request.error ?? new Error('读取本地场景失败'))
  })
}

export async function saveStoredScene(scene) {
  const database = await openDatabase()

  return new Promise((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readwrite')
    const store = transaction.objectStore(STORE_NAME)
    const request = store.put(scene, CURRENT_SCENE_KEY)

    request.onsuccess = () => resolve(true)
    request.onerror = () => reject(request.error ?? new Error('保存本地场景失败'))
  })
}

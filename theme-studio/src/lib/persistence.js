import { openDB } from 'idb'

const DATABASE_NAME = 'theme-studio'
const DATABASE_VERSION = 1
const PROJECT_STORE = 'projects'
const PUBLISH_STORE = 'publishes'

const dbPromise = openDB(DATABASE_NAME, DATABASE_VERSION, {
  upgrade(db) {
    if (!db.objectStoreNames.contains(PROJECT_STORE)) {
      db.createObjectStore(PROJECT_STORE, { keyPath: 'id' })
    }

    if (!db.objectStoreNames.contains(PUBLISH_STORE)) {
      db.createObjectStore(PUBLISH_STORE, { keyPath: 'id' })
    }
  },
})

export async function listProjectsFromDb() {
  const db = await dbPromise
  return db.getAll(PROJECT_STORE)
}

export async function getProjectFromDb(projectId) {
  const db = await dbPromise
  return db.get(PROJECT_STORE, projectId)
}

export async function saveProjectToDb(project) {
  const db = await dbPromise
  return db.put(PROJECT_STORE, project)
}

export async function deleteProjectFromDb(projectId) {
  const db = await dbPromise
  return db.delete(PROJECT_STORE, projectId)
}

export async function listPublishRecordsFromDb() {
  const db = await dbPromise
  return db.getAll(PUBLISH_STORE)
}

export async function savePublishRecordToDb(record) {
  const db = await dbPromise
  return db.put(PUBLISH_STORE, record)
}

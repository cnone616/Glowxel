import { defineStore } from 'pinia'
import { buildPublishZip, generateProjectBundle, revokeBundleUrls, triggerDownload } from '../lib/exportPipeline'
import { listPublishRecordsFromDb, savePublishRecordToDb } from '../lib/persistence'

function buildRecord(project, draft) {
  return {
    id: crypto.randomUUID(),
    projectId: project.id,
    projectName: project.name,
    version: draft.version,
    description: draft.description,
    tags: draft.tags,
    publishedAt: new Date().toISOString(),
  }
}

export const usePublishStore = defineStore('publishStore', {
  state: () => ({
    records: [],
    activeBundle: null,
    activeProjectId: '',
    activeProjectUpdatedAt: '',
    loading: false,
  }),
  actions: {
    async boot() {
      this.records = await listPublishRecordsFromDb()
    },

    async ensureBundle(project) {
      if (
        this.activeBundle &&
        this.activeProjectId === project.id &&
        this.activeProjectUpdatedAt === project.updatedAt
      ) {
        return this.activeBundle
      }

      this.loading = true
      const bundle = await generateProjectBundle(project)
      revokeBundleUrls(this.activeBundle)
      this.activeBundle = bundle
      this.activeProjectId = project.id
      this.activeProjectUpdatedAt = project.updatedAt
      this.loading = false
      return bundle
    },

    async publishProject(project) {
      const bundle = await this.ensureBundle(project)
      const zipBlob = await buildPublishZip(bundle)
      const record = buildRecord(project, project.publishDraft)
      await savePublishRecordToDb(record)
      this.records.unshift(record)
      return {
        bundle,
        zipBlob,
        record,
      }
    },

    downloadBlob(blob, filename) {
      triggerDownload(blob, filename)
    },
  },
})

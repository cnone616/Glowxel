import { defineStore } from 'pinia'
import { getBackgroundPresetByKey, getMotionPresetByKey } from '../data/studioCatalog'
import {
  buildTweenFramesFromTuples,
  cloneProject,
  createBlankStudioProject,
  createProjectSummary,
  createStudioObjectFromAsset,
  createStudioProjectFromTemplate,
  createTriggerAction,
  ensureFontId,
  normalizeStudioProject,
  syncProjectThemePackage,
} from '../lib/studioProject'
import { deleteProjectFromDb, getProjectFromDb, listProjectsFromDb, saveProjectToDb } from '../lib/persistence'
import { FRAME_TYPE_DIFF, FRAME_TYPE_FULL } from '../utils/themeContract'
import { removePixel, tuplesToPixelMap, upsertPixel } from '../utils/frameRuntime'

function fillFromPoint(frameTuples, x, y, color) {
  const targetMap = tuplesToPixelMap(frameTuples)
  const visited = new Set()
  const queue = [[x, y]]

  const targetKey = `${x}:${y}`
  const targetColor = targetMap.get(targetKey) || [0, 0, 0]
  const replace = [color.r, color.g, color.b]

  while (queue.length > 0) {
    const current = queue.shift()
    const cx = current[0]
    const cy = current[1]
    const key = `${cx}:${cy}`

    if (visited.has(key)) {
      continue
    }

    visited.add(key)

    const currentColor = targetMap.get(key) || [0, 0, 0]
    const sameColor =
      currentColor[0] === targetColor[0] &&
      currentColor[1] === targetColor[1] &&
      currentColor[2] === targetColor[2]

    if (!sameColor) {
      continue
    }

    upsertPixel(frameTuples, cx, cy, replace[0], replace[1], replace[2])

    if (cx > 0) {
      queue.push([cx - 1, cy])
    }
    if (cx < 63) {
      queue.push([cx + 1, cy])
    }
    if (cy > 0) {
      queue.push([cx, cy - 1])
    }
    if (cy < 63) {
      queue.push([cx, cy + 1])
    }
  }
}

function cloneFrameTuples(frameTuples) {
  return frameTuples.map((tuple) => [...tuple])
}

function removeStudioAssetReferences(draft, assetId) {
  draft.studio.scene.background.assetIds = draft.studio.scene.background.assetIds.filter((id) => id !== assetId)
  draft.studio.scene.objects = draft.studio.scene.objects.filter((object) => object.assetId !== assetId)

  for (const triggerKey of ['minute', 'hour']) {
    draft.studio.triggers[triggerKey] = draft.studio.triggers[triggerKey].map((action) => {
      if (action.targetObjectId && !draft.studio.scene.objects.some((object) => object.id === action.targetObjectId)) {
        return {
          ...action,
          targetObjectId: '',
        }
      }
      return action
    })
  }
}

export const useProjectStore = defineStore('projectStore', {
  state: () => ({
    initialized: false,
    projects: [],
    activeProject: null,
    loading: false,
    hasPendingPixelEdits: false,
  }),
  getters: {
    activeSummary(state) {
      if (!state.activeProject) {
        return null
      }
      return createProjectSummary(state.activeProject)
    },
  },
  actions: {
    async boot() {
      if (this.initialized) {
        return
      }

      this.loading = true
      const projects = await listProjectsFromDb()
      this.projects = projects
        .map((project) => {
          normalizeStudioProject(project)
          return createProjectSummary(project)
        })
        .sort((a, b) => new Date(b.updatedAt).getTime() - new Date(a.updatedAt).getTime())
      this.loading = false
      this.initialized = true
    },

    updateProjectSummary(project) {
      normalizeStudioProject(project)
      const summary = createProjectSummary(project)
      const existingIndex = this.projects.findIndex((item) => item.id === project.id)
      if (existingIndex === -1) {
        this.projects.unshift(summary)
        return
      }

      this.projects.splice(existingIndex, 1, summary)
      this.projects.sort((a, b) => new Date(b.updatedAt).getTime() - new Date(a.updatedAt).getTime())
    },

    async saveProject(project) {
      await saveProjectToDb(project)
      this.updateProjectSummary(project)
    },

    async createProjectFromTemplate(templateKey) {
      const project = createStudioProjectFromTemplate(templateKey)
      normalizeStudioProject(project)
      this.activeProject = project
      this.hasPendingPixelEdits = false
      await this.saveProject(project)
      return project
    },

    async createBlankProject() {
      const project = createBlankStudioProject()
      normalizeStudioProject(project)
      this.activeProject = project
      this.hasPendingPixelEdits = false
      await this.saveProject(project)
      return project
    },

    async loadProject(projectId) {
      const project = await getProjectFromDb(projectId)
      this.activeProject = project ? cloneProject(project) : null
      if (this.activeProject) {
        normalizeStudioProject(this.activeProject)
      }
      this.hasPendingPixelEdits = false
      return this.activeProject
    },

    async deleteProject(projectId) {
      await deleteProjectFromDb(projectId)
      this.projects = this.projects.filter((project) => project.id !== projectId)
      if (this.activeProject && this.activeProject.id === projectId) {
        this.activeProject = null
      }
    },

    mutateActiveProject(mutator) {
      if (!this.activeProject) {
        return
      }

      const draft = cloneProject(this.activeProject)
      normalizeStudioProject(draft)
      mutator(draft)
      syncProjectThemePackage(draft)
      this.activeProject = draft
    },

    async persistActiveProject() {
      if (!this.activeProject) {
        return
      }

      await this.saveProject(this.activeProject)
      this.hasPendingPixelEdits = false
    },

    async commitPixelFrameChanges() {
      if (!this.activeProject) {
        return
      }

      if (!this.hasPendingPixelEdits) {
        await this.persistActiveProject()
        return
      }

      const draft = cloneProject(this.activeProject)
      normalizeStudioProject(draft)
      syncProjectThemePackage(draft)
      this.activeProject = draft
      await this.persistActiveProject()
    },

    async setProjectName(name) {
      this.mutateActiveProject((draft) => {
        draft.name = name
      })
      await this.persistActiveProject()
    },

    paintPixel(layerKey, frameIndex, x, y, color) {
      if (!this.activeProject) {
        return
      }

      const frame = this.activeProject.layers[layerKey].frames[frameIndex]
      upsertPixel(frame, x, y, color.r, color.g, color.b)
      this.hasPendingPixelEdits = true
    },

    erasePixel(layerKey, frameIndex, x, y) {
      if (!this.activeProject) {
        return
      }

      const frame = this.activeProject.layers[layerKey].frames[frameIndex]
      removePixel(frame, x, y)
      this.hasPendingPixelEdits = true
    },

    fillPixel(layerKey, frameIndex, x, y, color) {
      if (!this.activeProject) {
        return
      }

      const frame = this.activeProject.layers[layerKey].frames[frameIndex]
      fillFromPoint(frame, x, y, color)
      this.hasPendingPixelEdits = true
    },

    getCompositeColor(frameIndex, x, y) {
      if (!this.activeProject) {
        return { r: 0, g: 0, b: 0 }
      }

      for (let layerIndex = 2; layerIndex >= 0; layerIndex -= 1) {
        const layerKey = ['accent', 'motion', 'background'][layerIndex]
        const frame = this.activeProject.layers[layerKey].frames[frameIndex]
        for (let i = 0; i < frame.length; i += 1) {
          if (frame[i][0] === x && frame[i][1] === y) {
            return { r: frame[i][2], g: frame[i][3], b: frame[i][4] }
          }
        }
      }

      return { r: 0, g: 0, b: 0 }
    },

    async setFrameDelay(frameIndex, delay) {
      this.mutateActiveProject((draft) => {
        draft.timeline.frames[frameIndex].delay = delay
      })
      await this.persistActiveProject()
    },

    async setFrameType(frameIndex, type) {
      this.mutateActiveProject((draft) => {
        if (frameIndex === 0) {
          draft.timeline.frames[frameIndex].type = FRAME_TYPE_FULL
          return
        }

        draft.timeline.frames[frameIndex].type = type
      })
      await this.persistActiveProject()
    },

    async insertFrameAfter(frameIndex) {
      this.mutateActiveProject((draft) => {
        const sourceMeta = draft.timeline.frames[frameIndex]
        const newMeta = {
          id: crypto.randomUUID(),
          delay: sourceMeta.delay,
          type: sourceMeta.type,
        }

        draft.timeline.frames.splice(frameIndex + 1, 0, newMeta)
        draft.layers.background.frames.splice(frameIndex + 1, 0, cloneProject(draft.layers.background.frames[frameIndex]))
        draft.layers.motion.frames.splice(frameIndex + 1, 0, cloneProject(draft.layers.motion.frames[frameIndex]))
        draft.layers.accent.frames.splice(frameIndex + 1, 0, cloneProject(draft.layers.accent.frames[frameIndex]))
      })
      await this.persistActiveProject()
    },

    async duplicateFrame(frameIndex) {
      await this.insertFrameAfter(frameIndex)
    },

    async removeFrame(frameIndex) {
      this.mutateActiveProject((draft) => {
        if (draft.timeline.frames.length === 1) {
          return
        }

        draft.timeline.frames.splice(frameIndex, 1)
        draft.layers.background.frames.splice(frameIndex, 1)
        draft.layers.motion.frames.splice(frameIndex, 1)
        draft.layers.accent.frames.splice(frameIndex, 1)

        if (draft.timeline.loopStartIndex >= draft.timeline.frames.length) {
          draft.timeline.loopStartIndex = draft.timeline.frames.length - 1
        }

        if (draft.timeline.introEndIndex >= draft.timeline.frames.length) {
          draft.timeline.introEndIndex = draft.timeline.frames.length - 1
        }

        if (draft.timeline.frames[0].type !== FRAME_TYPE_FULL) {
          draft.timeline.frames[0].type = FRAME_TYPE_FULL
        }
      })
      await this.persistActiveProject()
    },

    async applyMotionPreset(presetKey) {
      const preset = getMotionPresetByKey(presetKey)
      if (!preset || !this.activeProject) {
        return
      }

      this.mutateActiveProject((draft) => {
        draft.layers.motion.frames = preset.apply(draft.timeline.frames.length)
      })
      await this.persistActiveProject()
    },

    async applyBackgroundPreset(presetKey) {
      const preset = getBackgroundPresetByKey(presetKey)
      if (!preset || !this.activeProject) {
        return
      }

      this.mutateActiveProject((draft) => {
        draft.layers.background.frames = preset.apply(draft.timeline.frames.length)
      })
      await this.persistActiveProject()
    },

    async copyPreviousFrameToCurrent(layerKey, frameIndex) {
      if (frameIndex < 1) {
        return
      }

      this.mutateActiveProject((draft) => {
        draft.layers[layerKey].frames[frameIndex] = cloneFrameTuples(draft.layers[layerKey].frames[frameIndex - 1])
      })
      await this.persistActiveProject()
    },

    async clearLayerFrame(layerKey, frameIndex) {
      this.mutateActiveProject((draft) => {
        draft.layers[layerKey].frames[frameIndex] = []
      })
      await this.persistActiveProject()
    },

    async generateTweenFrames(layerKey, frameIndex, stepCount) {
      if (!this.activeProject) {
        return
      }

      if (frameIndex >= this.activeProject.timeline.frames.length - 1) {
        return
      }

      this.mutateActiveProject((draft) => {
        const sourceFrame = draft.layers[layerKey].frames[frameIndex]
        const nextFrame = draft.layers[layerKey].frames[frameIndex + 1]
        const tweenFrames = buildTweenFramesFromTuples(sourceFrame, nextFrame, stepCount)
        if (tweenFrames.length === 0) {
          return
        }

        const insertionIndex = frameIndex + 1
        const sourceDelay = draft.timeline.frames[frameIndex].delay
        const otherLayerKeys = ['background', 'motion', 'accent'].filter((candidate) => candidate !== layerKey)

        for (let tweenIndex = 0; tweenIndex < tweenFrames.length; tweenIndex += 1) {
          draft.timeline.frames.splice(insertionIndex + tweenIndex, 0, {
            id: crypto.randomUUID(),
            delay: sourceDelay,
            type: FRAME_TYPE_DIFF,
          })

          draft.layers[layerKey].frames.splice(insertionIndex + tweenIndex, 0, cloneFrameTuples(tweenFrames[tweenIndex]))

          for (let keyIndex = 0; keyIndex < otherLayerKeys.length; keyIndex += 1) {
            const otherLayerKey = otherLayerKeys[keyIndex]
            draft.layers[otherLayerKey].frames.splice(
              insertionIndex + tweenIndex,
              0,
              cloneFrameTuples(draft.layers[otherLayerKey].frames[frameIndex]),
            )
          }
        }

        if (draft.timeline.loopStartIndex > frameIndex) {
          draft.timeline.loopStartIndex += tweenFrames.length
        }

        if (draft.timeline.introEndIndex > frameIndex) {
          draft.timeline.introEndIndex += tweenFrames.length
        }
      })
      await this.persistActiveProject()
    },

    async updateTextLayer(layerKey, patch) {
      this.mutateActiveProject((draft) => {
        const layer = draft.clock[layerKey]
        Object.assign(layer, patch)
        ensureFontId(layer.font)
      })
      await this.persistActiveProject()
    },

    async updateClockMeta(patch) {
      this.mutateActiveProject((draft) => {
        Object.assign(draft.clock, patch)
      })
      await this.persistActiveProject()
    },

    async updatePreviewTime(value) {
      this.mutateActiveProject((draft) => {
        draft.preview.currentTime = value
      })
      await this.persistActiveProject()
    },

    async updatePreviewDate(value) {
      this.mutateActiveProject((draft) => {
        draft.preview.currentDate = value
      })
      await this.persistActiveProject()
    },

    async updateTimelineMeta(patch) {
      this.mutateActiveProject((draft) => {
        Object.assign(draft.timeline, patch)
      })
      await this.persistActiveProject()
    },

    async updatePublishDraft(patch) {
      this.mutateActiveProject((draft) => {
        Object.assign(draft.publishDraft, patch)
      })
      await this.persistActiveProject()
    },

    async appendStudioAssets(assets) {
      if (!Array.isArray(assets) || assets.length === 0) {
        return
      }

      this.mutateActiveProject((draft) => {
        draft.studio.materials.unshift(...assets)
      })
      await this.persistActiveProject()
    },

    async removeStudioAsset(assetId) {
      this.mutateActiveProject((draft) => {
        draft.studio.materials = draft.studio.materials.filter((asset) => asset.id !== assetId)
        removeStudioAssetReferences(draft, assetId)
      })
      await this.persistActiveProject()
    },

    async addAssetToBackgroundSequence(assetId) {
      this.mutateActiveProject((draft) => {
        if (!draft.studio.scene.background.assetIds.includes(assetId)) {
          draft.studio.scene.background.assetIds.push(assetId)
        }
      })
      await this.persistActiveProject()
    },

    async removeAssetFromBackgroundSequence(assetId) {
      this.mutateActiveProject((draft) => {
        draft.studio.scene.background.assetIds = draft.studio.scene.background.assetIds.filter((id) => id !== assetId)
      })
      await this.persistActiveProject()
    },

    async updateStudioBackgroundRule(patch) {
      this.mutateActiveProject((draft) => {
        Object.assign(draft.studio.scene.background, patch)
      })
      await this.persistActiveProject()
    },

    async addStudioObjectFromAsset(assetId) {
      if (!this.activeProject) {
        return null
      }

      const asset = this.activeProject.studio.materials.find((item) => item.id === assetId)
      if (!asset) {
        return null
      }

      let createdId = ''
      this.mutateActiveProject((draft) => {
        const draftAsset = draft.studio.materials.find((item) => item.id === assetId)
        const object = createStudioObjectFromAsset(draftAsset)
        createdId = object.id
        draft.studio.scene.objects.push(object)
      })
      await this.persistActiveProject()
      return createdId
    },

    async updateStudioObject(objectId, patch) {
      this.mutateActiveProject((draft) => {
        const object = draft.studio.scene.objects.find((item) => item.id === objectId)
        if (!object) {
          return
        }
        Object.assign(object, patch)
      })
      await this.persistActiveProject()
    },

    async removeStudioObject(objectId) {
      this.mutateActiveProject((draft) => {
        draft.studio.scene.objects = draft.studio.scene.objects.filter((item) => item.id !== objectId)
        for (const triggerKey of ['minute', 'hour']) {
          draft.studio.triggers[triggerKey] = draft.studio.triggers[triggerKey].map((action) => {
            if (action.targetObjectId === objectId) {
              return {
                ...action,
                targetObjectId: '',
              }
            }
            return action
          })
        }
      })
      await this.persistActiveProject()
    },

    async addStudioPathPoint(objectId, point) {
      this.mutateActiveProject((draft) => {
        const object = draft.studio.scene.objects.find((item) => item.id === objectId)
        if (!object) {
          return
        }
        object.pathPoints.push({ x: point.x, y: point.y })
      })
      await this.persistActiveProject()
    },

    async clearStudioPath(objectId) {
      this.mutateActiveProject((draft) => {
        const object = draft.studio.scene.objects.find((item) => item.id === objectId)
        if (!object) {
          return
        }
        object.pathPoints = []
      })
      await this.persistActiveProject()
    },

    async updateStudioWidget(widgetKey, patch) {
      this.mutateActiveProject((draft) => {
        const widget = draft.studio.timeWidgets[widgetKey]
        if (!widget) {
          return
        }
        Object.assign(widget, patch)
        ensureFontId(widget.font)
      })
      await this.persistActiveProject()
    },

    async addStudioTriggerAction(triggerKey) {
      let createdId = ''
      this.mutateActiveProject((draft) => {
        const action = createTriggerAction(triggerKey)
        createdId = action.id
        draft.studio.triggers[triggerKey].push(action)
      })
      await this.persistActiveProject()
      return createdId
    },

    async updateStudioTriggerAction(triggerKey, actionId, patch) {
      this.mutateActiveProject((draft) => {
        const action = draft.studio.triggers[triggerKey].find((item) => item.id === actionId)
        if (!action) {
          return
        }
        Object.assign(action, patch)
      })
      await this.persistActiveProject()
    },

    async removeStudioTriggerAction(triggerKey, actionId) {
      this.mutateActiveProject((draft) => {
        draft.studio.triggers[triggerKey] = draft.studio.triggers[triggerKey].filter((item) => item.id !== actionId)
      })
      await this.persistActiveProject()
    },

    async updateStudioNotes(notes) {
      this.mutateActiveProject((draft) => {
        draft.studio.notes = notes
      })
      await this.persistActiveProject()
    },
  },
})

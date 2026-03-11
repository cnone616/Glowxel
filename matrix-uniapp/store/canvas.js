/**
 * 画布草稿状态管理
 * Canvas Draft Store - 管理画布草稿的保存、读取、更新功能
 */

import { reactive } from 'vue'

// 画布草稿数据模型
const CanvasDraft = {
  id: '',              // 草稿ID
  name: '',            // 草稿名称
  pixels: new Map(),   // 像素数据 Map<string, string> key: "x,y", value: color
  width: 32,           // 画布宽度
  height: 32,          // 画布高度
  colors: [],          // 使用的颜色数组
  createdAt: null,     // 创建时间
  updatedAt: null,     // 更新时间
  isPublished: false,  // 是否已发布
  publishedProjectId: null // 发布后的作品ID
}

// 状态管理
const state = reactive({
  currentDraft: null,  // 当前编辑的草稿
  draftList: [],       // 草稿列表
  isLoading: false,    // 加载状态
  error: null          // 错误信息
})

// 工具函数
const utils = {
  // 生成唯一ID
  generateId() {
    return 'draft_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9)
  },

  // 生成草稿名称
  generateDraftName() {
    const now = new Date()
    const dateStr = now.toLocaleDateString('zh-CN')
    const timeStr = now.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
    return `草稿 ${dateStr} ${timeStr}`
  },

  // 序列化像素数据用于存储
  serializePixels(pixelsMap) {
    const obj = {}
    pixelsMap.forEach((color, position) => {
      obj[position] = color
    })
    return obj
  },

  // 反序列化像素数据
  deserializePixels(pixelsObj) {
    const map = new Map()
    if (pixelsObj) {
      Object.entries(pixelsObj).forEach(([position, color]) => {
        map.set(position, color)
      })
    }
    return map
  },

  // 验证草稿数据
  validateDraft(draft) {
    if (!draft.id || !draft.name) {
      throw new Error('草稿ID和名称不能为空')
    }
    if (!draft.width || !draft.height || draft.width <= 0 || draft.height <= 0) {
      throw new Error('画布尺寸必须大于0')
    }
    return true
  }
}

// 存储操作
const storage = {
  // 保存草稿到本地存储
  saveDraft(draft) {
    try {
      utils.validateDraft(draft)
      
      const draftData = {
        ...draft,
        pixels: utils.serializePixels(draft.pixels),
        updatedAt: new Date().toISOString()
      }
      
      uni.setStorageSync(`canvas_draft_${draft.id}`, draftData)
      
      // 更新草稿列表
      this.updateDraftList(draft)
      
      return true
    } catch (error) {
      console.error('保存草稿失败:', error)
      state.error = error.message
      return false
    }
  },

  // 读取草稿
  getDraft(draftId) {
    try {
      const draftData = uni.getStorageSync(`canvas_draft_${draftId}`)
      if (!draftData) {
        return null
      }

      return {
        ...draftData,
        pixels: utils.deserializePixels(draftData.pixels),
        createdAt: new Date(draftData.createdAt),
        updatedAt: new Date(draftData.updatedAt)
      }
    } catch (error) {
      console.error('读取草稿失败:', error)
      state.error = error.message
      return null
    }
  },

  // 删除草稿
  deleteDraft(draftId) {
    try {
      uni.removeStorageSync(`canvas_draft_${draftId}`)
      
      // 从草稿列表中移除
      const draftList = this.getDraftList()
      const updatedList = draftList.filter(draft => draft.id !== draftId)
      uni.setStorageSync('canvas_draft_list', updatedList)
      
      // 更新状态
      state.draftList = updatedList
      
      return true
    } catch (error) {
      console.error('删除草稿失败:', error)
      state.error = error.message
      return false
    }
  },

  // 获取草稿列表
  getDraftList() {
    try {
      const list = uni.getStorageSync('canvas_draft_list') || []
      return list.map(draft => ({
        ...draft,
        createdAt: new Date(draft.createdAt),
        updatedAt: new Date(draft.updatedAt)
      }))
    } catch (error) {
      console.error('获取草稿列表失败:', error)
      return []
    }
  },

  // 更新草稿列表
  updateDraftList(draft) {
    try {
      let draftList = this.getDraftList()
      
      // 查找是否已存在
      const existingIndex = draftList.findIndex(d => d.id === draft.id)
      
      const listItem = {
        id: draft.id,
        name: draft.name,
        width: draft.width,
        height: draft.height,
        createdAt: draft.createdAt,
        updatedAt: draft.updatedAt,
        isPublished: draft.isPublished,
        publishedProjectId: draft.publishedProjectId
      }
      
      if (existingIndex >= 0) {
        // 更新现有项
        draftList[existingIndex] = listItem
      } else {
        // 添加新项
        draftList.unshift(listItem)
      }
      
      // 按更新时间排序
      draftList.sort((a, b) => new Date(b.updatedAt) - new Date(a.updatedAt))
      
      uni.setStorageSync('canvas_draft_list', draftList)
      state.draftList = draftList
      
    } catch (error) {
      console.error('更新草稿列表失败:', error)
    }
  }
}

// 主要操作方法
const actions = {
  // 创建新草稿
  createDraft(options = {}) {
    const draft = {
      ...CanvasDraft,
      id: utils.generateId(),
      name: options.name || utils.generateDraftName(),
      width: options.width || 32,
      height: options.height || 32,
      pixels: new Map(),
      colors: options.colors || [],
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString(),
      isPublished: false,
      publishedProjectId: null
    }

    if (storage.saveDraft(draft)) {
      state.currentDraft = draft
      return draft
    }
    return null
  },

  // 加载草稿
  async loadDraft(draftId) {
    state.isLoading = true
    state.error = null

    try {
      const draft = storage.getDraft(draftId)
      if (draft) {
        state.currentDraft = draft
        return draft
      } else {
        state.error = '草稿不存在'
        return null
      }
    } finally {
      state.isLoading = false
    }
  },

  // 保存当前草稿
  saveCurrentDraft() {
    if (!state.currentDraft) {
      state.error = '没有可保存的草稿'
      return false
    }

    return storage.saveDraft(state.currentDraft)
  },

  // 更新草稿像素
  updatePixel(x, y, color) {
    if (!state.currentDraft) {
      state.error = '没有当前草稿'
      return false
    }

    const position = `${x},${y}`
    if (color) {
      state.currentDraft.pixels.set(position, color)
      
      // 添加到颜色列表
      if (!state.currentDraft.colors.includes(color)) {
        state.currentDraft.colors.push(color)
      }
    } else {
      state.currentDraft.pixels.delete(position)
    }

    state.currentDraft.updatedAt = new Date().toISOString()
    return true
  },

  // 清空画布
  clearCanvas() {
    if (!state.currentDraft) {
      state.error = '没有当前草稿'
      return false
    }

    state.currentDraft.pixels.clear()
    state.currentDraft.colors = []
    state.currentDraft.updatedAt = new Date().toISOString()
    return true
  },

  // 加载草稿列表
  async loadDraftList() {
    state.isLoading = true
    try {
      const list = storage.getDraftList()
      state.draftList = list
      return list
    } finally {
      state.isLoading = false
    }
  },

  // 删除草稿
  async deleteDraft(draftId) {
    const success = storage.deleteDraft(draftId)
    if (success && state.currentDraft && state.currentDraft.id === draftId) {
      state.currentDraft = null
    }
    return success
  },

  // 标记草稿为已发布
  markAsPublished(draftId, projectId) {
    const draft = storage.getDraft(draftId)
    if (draft) {
      draft.isPublished = true
      draft.publishedProjectId = projectId
      draft.updatedAt = new Date().toISOString()
      
      const success = storage.saveDraft(draft)
      if (success && state.currentDraft && state.currentDraft.id === draftId) {
        state.currentDraft = draft
      }
      return success
    }
    return false
  },

  // 获取画布是否为空
  isCanvasEmpty() {
    return !state.currentDraft || state.currentDraft.pixels.size === 0
  },

  // 获取画布统计信息
  getCanvasStats() {
    if (!state.currentDraft) {
      return { pixelCount: 0, colorCount: 0 }
    }

    return {
      pixelCount: state.currentDraft.pixels.size,
      colorCount: state.currentDraft.colors.length
    }
  }
}

// 导出
export default {
  state,
  actions,
  utils
}

// 兼容性导出
export const canvasStore = {
  state,
  ...actions
}
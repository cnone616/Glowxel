import { defineStore } from 'pinia'
import { generateId } from '../utils/index.js'

export const useProjectStore = defineStore('project', {
  state: () => ({
    projects: [],
    currentProject: null
  }),
  
  getters: {
    // 获取项目列表（按最后编辑时间排序）
    sortedProjects: (state) => {
      return [...state.projects].sort((a, b) => {
        return new Date(b.lastEdited) - new Date(a.lastEdited)
      })
    }
  },
  
  actions: {
    // 初始化 - 加载项目列表
    init() {
      this.loadProjects()
    },
    
    // 加载项目列表
    loadProjects() {
      try {
        const data = uni.getStorageSync('projects')
        if (data) {
          this.projects = JSON.parse(data).map(p => ({
            ...p,
            lastEdited: new Date(p.lastEdited)
          }))
        }
      } catch (e) {
        console.error('加载项目失败:', e)
      }
    },
    
    // 保存项目列表
    saveProjects() {
      try {
        uni.setStorageSync('projects', JSON.stringify(this.projects))
      } catch (e) {
        console.error('保存项目失败:', e)
      }
    },
    
    // 添加项目
    addProject(name, width, height, paletteSize, palette, paddedWidth, paddedHeight) {
      const newProject = {
        id: generateId(),
        name,
        width,
        height,
        paddedWidth: paddedWidth || width,  // 填充后的宽度
        paddedHeight: paddedHeight || height,  // 填充后的高度
        progress: 0,
        lastEdited: new Date(),
        paletteSize,
        palette: palette || [],
        thumbnail: '' // 缩略图 base64
      }
      this.projects.unshift(newProject)
      this.saveProjects()
      return newProject.id
    },
    
    // 获取项目
    getProject(id) {
      return this.projects.find(p => p.id === id)
    },
    
    // 更新项目
    updateProject(id, updates) {
      const index = this.projects.findIndex(p => p.id === id)
      if (index !== -1) {
        this.projects[index] = { 
          ...this.projects[index], 
          ...updates, 
          lastEdited: new Date() 
        }
        this.saveProjects()
      }
    },
    
    // 更新项目名称
    updateProjectName(id, name) {
      this.updateProject(id, { name })
    },
    
    // 更新项目缩略图
    updateProjectThumbnail(id, thumbnail) {
      const index = this.projects.findIndex(p => p.id === id)
      if (index !== -1) {
        this.projects[index] = { 
          ...this.projects[index], 
          thumbnail,
          lastEdited: new Date() 
        }
        this.saveProjects()
      }
    },
    
    // 删除项目
    deleteProject(id) {
      this.projects = this.projects.filter(p => p.id !== id)
      // 同时删除项目的像素数据
      try {
        uni.removeStorageSync(`project-pixels-${id}`)
      } catch (e) {
        console.error('删除项目数据失败:', e)
      }
      this.saveProjects()
    },
    
    // 设置当前项目
    setCurrentProject(project) {
      this.currentProject = project
    },
    
    // 获取项目像素数据
    getProjectPixels(projectId) {
      try {
        const data = uni.getStorageSync(`pixels-${projectId}`)
        if (data) {
          return new Map(JSON.parse(data))
        }
      } catch (e) {
        console.error('加载像素数据失败:', e)
      }
      return new Map()
    },
    
    // 保存项目像素数据
    saveProjectPixels(projectId, pixels) {
      try {
        const data = JSON.stringify(Array.from(pixels.entries()))
        uni.setStorageSync(`pixels-${projectId}`, data)
      } catch (e) {
        console.error('保存像素数据失败:', e)
      }
    },
    
    // 获取画布进度
    getBoardProgress(projectId, boardId) {
      try {
        const data = uni.getStorageSync(`board-progress-${projectId}-${boardId}`)
        if (data) {
          return JSON.parse(data)
        }
      } catch (e) {
        console.error('加载画布进度失败:', e)
      }
      return null
    },
    
    // 保存看板进度
    saveBoardProgress(projectId, boardId, progress) {
      try {
        uni.setStorageSync(`board-progress-${projectId}-${boardId}`, JSON.stringify(progress))
      } catch (e) {
        console.error('保存画布进度失败:', e)
      }
    },
    
    // 更新项目总进度
    updateProjectProgress(projectId) {
      const proj = this.getProject(projectId)
      if (!proj) return

      // 使用填充后的尺寸计算板子数量
      const paddedWidth = proj.paddedWidth || proj.width
      const paddedHeight = proj.paddedHeight || proj.height
      
      const boardsX = Math.ceil(paddedWidth / 52)
      const boardsY = Math.ceil(paddedHeight / 52)
      const totalBoards = boardsX * boardsY

      let sumCompletion = 0

      for (let y = 0; y < boardsY; y++) {
        for (let x = 0; x < boardsX; x++) {
          const rowChar = String.fromCharCode(65 + y)
          const colNum = x + 1
          const bId = `${rowChar}${colNum}`
          
          const boardProgress = this.getBoardProgress(projectId, bId)
          if (boardProgress) {
            sumCompletion += boardProgress.completion
          }
        }
      }

      const progressPercent = Math.round((sumCompletion / totalBoards) * 100)
      
      if (progressPercent !== proj.progress) {
        this.updateProject(projectId, { progress: progressPercent })
      }
    },
    
    // 保存看板缩略图
    saveBoardThumbnail(projectId, boardId, thumbnail) {
      try {
        uni.setStorageSync(`board-thumbnail-${projectId}-${boardId}`, thumbnail)
      } catch (e) {
        // 存储空间不足时，不保存缩略图，但不影响使用
        if (e.name === 'QuotaExceededError') {
          console.warn('存储空间不足，跳过看板缩略图保存')
        } else {
          console.error('保存看板缩略图失败:', e)
        }
      }
    },
    
    // 获取看板缩略图
    getBoardThumbnail(projectId, boardId) {
      try {
        return uni.getStorageSync(`board-thumbnail-${projectId}-${boardId}`)
      } catch (e) {
        console.error('加载看板缩略图失败:', e)
        return null
      }
    }
  }
})

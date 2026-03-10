import { defineStore } from 'pinia'
import { generateId } from '../utils/index.js'
import { MockAPI } from '../data/mock/index.js'

export const useProjectStore = defineStore('project', {
  state: () => ({
    projects: [],
    currentProject: null,
    isLoading: false,
    error: null
  }),
  
  getters: {
    // 获取项目列表（按最后编辑时间排序）
    sortedProjects: (state) => {
      return [...state.projects].sort((a, b) => {
        return new Date(b.updateTime || b.lastEdited) - new Date(a.updateTime || a.lastEdited)
      })
    },
    
    // 获取进行中的项目
    inProgressProjects: (state) => {
      return state.projects.filter(p => p.status === 'in_progress')
    },
    
    // 获取已完成的项目
    completedProjects: (state) => {
      return state.projects.filter(p => p.status === 'completed')
    },
    
    // 获取草稿项目
    draftProjects: (state) => {
      return state.projects.filter(p => p.status === 'draft')
    }
  },
  
  actions: {
    // 初始化 - 加载项目列表
    async init() {
      await this.loadProjects()
    },
    
    // 加载项目列表 - 使用Mock数据
    async loadProjects() {
      this.isLoading = true
      this.error = null
      
      try {
        // 首先尝试从本地存储加载
        const localData = uni.getStorageSync('projects')
        if (localData) {
          this.projects = JSON.parse(localData).map(p => ({
            ...p,
            lastEdited: new Date(p.lastEdited || p.updateTime)
          }))
        } else {
          // 如果本地没有数据，使用Mock数据初始化
          const mockProjects = MockAPI.projects.getAll()
          this.projects = mockProjects.map(p => ({
            ...p,
            lastEdited: new Date(p.updateTime),
            // 转换Mock数据格式到本地格式
            width: parseInt(p.size.split('x')[0]),
            height: parseInt(p.size.split('x')[1]),
            paddedWidth: parseInt(p.size.split('x')[0]),
            paddedHeight: parseInt(p.size.split('x')[1]),
            paletteSize: p.colorCount,
            palette: [],
            thumbnail: p.thumbnail
          }))
          // 保存到本地存储
          this.saveToStorage()
        }
      } catch (e) {
        console.error('加载项目失败:', e)
        this.error = '加载项目失败'
        // 使用Mock数据作为备选
        const mockProjects = MockAPI.projects.getAll()
        this.projects = mockProjects.map(p => ({
          ...p,
          lastEdited: new Date(p.updateTime),
          width: parseInt(p.size.split('x')[0]),
          height: parseInt(p.size.split('x')[1]),
          paddedWidth: parseInt(p.size.split('x')[0]),
          paddedHeight: parseInt(p.size.split('x')[1]),
          paletteSize: p.colorCount,
          palette: [],
          thumbnail: p.thumbnail
        }))
      } finally {
        this.isLoading = false
      }
    },
    
    // 保存到本地存储
    saveToStorage() {
      try {
        uni.setStorageSync('projects', JSON.stringify(this.projects))
      } catch (e) {
        console.error('保存项目失败:', e)
      }
    },
    
    // 保存项目列表（兼容旧方法名）
    saveProjects() {
      this.saveToStorage()
    },
    
    // 添加项目
    addProject(name, width, height, paletteSize, palette, paddedWidth, paddedHeight) {
      const newProject = {
        id: generateId(),
        name,
        width,
        height,
        paddedWidth: paddedWidth || width,
        paddedHeight: paddedHeight || height,
        progress: 0,
        lastEdited: new Date(),
        updateTime: new Date().toISOString().split('T')[0],
        createTime: new Date().toISOString().split('T')[0],
        paletteSize,
        palette: palette || [],
        thumbnail: '',
        size: `${width}x${height}`,
        colorCount: paletteSize,
        status: 'draft',
        description: '',
        icon: 'picture'
      }
      this.projects.unshift(newProject)
      this.saveToStorage()
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
          lastEdited: new Date(),
          updateTime: new Date().toISOString().split('T')[0]
        }
        this.saveToStorage()
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
          lastEdited: new Date(),
          updateTime: new Date().toISOString().split('T')[0]
        }
        this.saveToStorage()
      }
    },
    
    // 删除项目
    deleteProject(id) {
      this.projects = this.projects.filter(p => p.id !== id)
      // 同时删除项目的像素数据
      try {
        uni.removeStorageSync(`project-pixels-${id}`)
        uni.removeStorageSync(`pixels-${id}`)
      } catch (e) {
        console.error('删除项目数据失败:', e)
      }
      this.saveToStorage()
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
        // 根据进度更新状态
        let status = 'draft'
        if (progressPercent > 0 && progressPercent < 100) {
          status = 'in_progress'
        } else if (progressPercent === 100) {
          status = 'completed'
        }
        
        this.updateProject(projectId, { 
          progress: progressPercent,
          status: status
        })
      }
    },
    
    // 保存看板缩略图
    saveBoardThumbnail(projectId, boardId, thumbnail) {
      try {
        uni.setStorageSync(`board-thumbnail-${projectId}-${boardId}`, thumbnail)
      } catch (e) {
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
    },
    
    // 获取最近项目
    getRecentProjects(limit = 3) {
      return this.sortedProjects.slice(0, limit)
    }
  }
})
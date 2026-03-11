/**
 * 正式作品状态管理
 * Published Projects Store - 管理正式发布作品的存储和查询
 */

import { reactive } from 'vue'

// 正式作品数据模型
const PublishedProject = {
  id: '',              // 作品ID
  title: '',           // 作品标题 (必填)
  description: '',     // 作品描述 (必填)
  tags: [],            // 标签数组
  pixels: new Map(),   // 像素数据 (只读)
  width: 32,           // 画布宽度
  height: 32,          // 画布高度
  colors: [],          // 使用的颜色数组
  thumbnail: '',       // 缩略图base64
  createdAt: null,     // 发布时间
  authorId: '',        // 作者ID
  likes: 0,            // 点赞数
  views: 0,            // 浏览数
  isPublic: true,      // 是否公开
  canEdit: false       // 发布后不可编辑
}

// 状态管理
const state = reactive({
  myProjects: [],      // 我的作品列表
  currentProject: null, // 当前查看的作品
  isLoading: false,    // 加载状态
  error: null,         // 错误信息
  totalCount: 0        // 作品总数
})

// 工具函数
const utils = {
  // 生成唯一作品ID
  generateProjectId() {
    return 'project_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9)
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

  // 生成缩略图
  generateThumbnail(pixels, width, height) {
    // 创建canvas生成缩略图
    // 这里简化处理，实际应该生成真实的缩略图
    const pixelCount = pixels.size
    const colorCount = new Set(Array.from(pixels.values())).size
    
    return `data:image/svg+xml;base64,${btoa(`
      <svg width="64" height="64" xmlns="http://www.w3.org/2000/svg">
        <rect width="64" height="64" fill="#f0f0f0"/>
        <text x="32" y="32" text-anchor="middle" dy=".3em" font-size="12" fill="#666">
          ${pixelCount}px
        </text>
      </svg>
    `)}`
  },

  // 验证作品数据
  validateProject(projectData) {
    if (!projectData.title || projectData.title.trim() === '') {
      throw new Error('作品标题不能为空')
    }
    if (!projectData.description || projectData.description.trim() === '') {
      throw new Error('作品描述不能为空')
    }
    if (!projectData.pixels || projectData.pixels.size === 0) {
      throw new Error('作品内容不能为空')
    }
    if (!projectData.width || !projectData.height || projectData.width <= 0 || projectData.height <= 0) {
      throw new Error('画布尺寸必须大于0')
    }
    return true
  },

  // 清理HTML标签
  sanitizeText(text) {
    return text.replace(/<[^>]*>/g, '').trim()
  }
}

// 存储操作
const storage = {
  // 保存作品到本地存储
  saveProject(project) {
    try {
      utils.validateProject(project)
      
      const projectData = {
        ...project,
        pixels: utils.serializePixels(project.pixels),
        title: utils.sanitizeText(project.title),
        description: utils.sanitizeText(project.description),
        canEdit: false, // 强制设置为不可编辑
        createdAt: project.createdAt || new Date().toISOString()
      }
      
      // 保存单个作品
      uni.setStorageSync(`published_project_${project.id}`, projectData)
      
      // 更新作品列表
      this.updateProjectList(projectData)
      
      return true
    } catch (error) {
      console.error('保存作品失败:', error)
      state.error = error.message
      return false
    }
  },

  // 读取作品
  getProject(projectId) {
    try {
      const projectData = uni.getStorageSync(`published_project_${projectId}`)
      if (!projectData) {
        return null
      }

      return {
        ...projectData,
        pixels: utils.deserializePixels(projectData.pixels),
        createdAt: new Date(projectData.createdAt),
        canEdit: false // 确保不可编辑
      }
    } catch (error) {
      console.error('读取作品失败:', error)
      state.error = error.message
      return null
    }
  },

  // 删除作品
  deleteProject(projectId) {
    try {
      uni.removeStorageSync(`published_project_${projectId}`)
      
      // 从作品列表中移除
      const projectList = this.getProjectList()
      const updatedList = projectList.filter(project => project.id !== projectId)
      uni.setStorageSync('published_project_list', updatedList)
      
      // 更新状态
      state.myProjects = updatedList
      state.totalCount = updatedList.length
      
      return true
    } catch (error) {
      console.error('删除作品失败:', error)
      state.error = error.message
      return false
    }
  },

  // 获取作品列表
  getProjectList() {
    try {
      const list = uni.getStorageSync('published_project_list') || []
      return list.map(project => ({
        ...project,
        createdAt: new Date(project.createdAt),
        canEdit: false
      }))
    } catch (error) {
      console.error('获取作品列表失败:', error)
      return []
    }
  },

  // 更新作品列表
  updateProjectList(project) {
    try {
      let projectList = this.getProjectList()
      
      // 查找是否已存在
      const existingIndex = projectList.findIndex(p => p.id === project.id)
      
      const listItem = {
        id: project.id,
        title: project.title,
        description: project.description,
        tags: project.tags,
        thumbnail: project.thumbnail,
        width: project.width,
        height: project.height,
        colors: project.colors,
        createdAt: project.createdAt,
        authorId: project.authorId,
        likes: project.likes,
        views: project.views,
        isPublic: project.isPublic,
        canEdit: false
      }
      
      if (existingIndex >= 0) {
        // 更新现有项（但保持不可编辑）
        projectList[existingIndex] = { ...projectList[existingIndex], ...listItem, canEdit: false }
      } else {
        // 添加新项
        projectList.unshift(listItem)
      }
      
      // 按创建时间排序
      projectList.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt))
      
      uni.setStorageSync('published_project_list', projectList)
      state.myProjects = projectList
      state.totalCount = projectList.length
      
    } catch (error) {
      console.error('更新作品列表失败:', error)
    }
  }
}

// 主要操作方法
const actions = {
  // 发布作品
  publishProject(projectData) {
    try {
      // 验证数据
      utils.validateProject(projectData)
      
      const project = {
        ...PublishedProject,
        ...projectData,
        id: utils.generateProjectId(),
        title: utils.sanitizeText(projectData.title),
        description: utils.sanitizeText(projectData.description),
        thumbnail: projectData.thumbnail || utils.generateThumbnail(projectData.pixels, projectData.width, projectData.height),
        createdAt: new Date().toISOString(),
        authorId: projectData.authorId || 'current_user', // 实际应该从用户状态获取
        likes: 0,
        views: 0,
        canEdit: false // 强制不可编辑
      }

      if (storage.saveProject(project)) {
        state.currentProject = project
        return project
      }
      return null
    } catch (error) {
      state.error = error.message
      return null
    }
  },

  // 加载我的作品列表
  async loadMyProjects() {
    state.isLoading = true
    state.error = null

    try {
      const projects = storage.getProjectList()
      state.myProjects = projects
      state.totalCount = projects.length
      return projects
    } finally {
      state.isLoading = false
    }
  },

  // 加载单个作品
  async loadProject(projectId) {
    state.isLoading = true
    state.error = null

    try {
      const project = storage.getProject(projectId)
      if (project) {
        state.currentProject = project
        // 增加浏览数
        this.incrementViews(projectId)
        return project
      } else {
        state.error = '作品不存在'
        return null
      }
    } finally {
      state.isLoading = false
    }
  },

  // 删除作品
  async deleteProject(projectId) {
    const success = storage.deleteProject(projectId)
    if (success && state.currentProject && state.currentProject.id === projectId) {
      state.currentProject = null
    }
    return success
  },

  // 点赞作品
  async likeProject(projectId) {
    const project = storage.getProject(projectId)
    if (project) {
      project.likes += 1
      const success = storage.saveProject(project)
      if (success) {
        // 更新当前作品状态
        if (state.currentProject && state.currentProject.id === projectId) {
          state.currentProject.likes = project.likes
        }
        // 更新列表中的作品
        const projectInList = state.myProjects.find(p => p.id === projectId)
        if (projectInList) {
          projectInList.likes = project.likes
        }
      }
      return success
    }
    return false
  },

  // 增加浏览数
  async incrementViews(projectId) {
    const project = storage.getProject(projectId)
    if (project) {
      project.views += 1
      storage.saveProject(project)
    }
  },

  // 搜索作品
  searchProjects(keyword) {
    if (!keyword || keyword.trim() === '') {
      return state.myProjects
    }

    const lowerKeyword = keyword.toLowerCase()
    return state.myProjects.filter(project => 
      project.title.toLowerCase().includes(lowerKeyword) ||
      project.description.toLowerCase().includes(lowerKeyword) ||
      project.tags.some(tag => tag.toLowerCase().includes(lowerKeyword))
    )
  },

  // 按标签筛选作品
  filterByTag(tag) {
    if (!tag) {
      return state.myProjects
    }
    return state.myProjects.filter(project => project.tags.includes(tag))
  },

  // 获取所有使用的标签
  getAllTags() {
    const tagSet = new Set()
    state.myProjects.forEach(project => {
      project.tags.forEach(tag => tagSet.add(tag))
    })
    return Array.from(tagSet).sort()
  },

  // 获取作品统计信息
  getProjectStats() {
    const totalLikes = state.myProjects.reduce((sum, project) => sum + project.likes, 0)
    const totalViews = state.myProjects.reduce((sum, project) => sum + project.views, 0)
    const publicCount = state.myProjects.filter(project => project.isPublic).length
    
    return {
      totalProjects: state.totalCount,
      totalLikes,
      totalViews,
      publicCount,
      privateCount: state.totalCount - publicCount
    }
  },

  // 检查作品是否可编辑（始终返回false）
  canEditProject(projectId) {
    return false // 发布后的作品不可编辑
  },

  // 获取推荐标签
  getRecommendedTags() {
    return ['像素艺术', '创意设计', '游戏', '动漫', '风景', '人物', '抽象', '几何']
  }
}

// 导出
export default {
  state,
  actions,
  utils
}

// 兼容性导出
export const projectsStore = {
  state,
  ...actions
}
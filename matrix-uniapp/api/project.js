// 项目相关云开发 API（直接使用数据库，不需要云函数）
import cloud from '../utils/cloud.js'

const COLLECTION_NAME = 'projects'

export default {
  // 获取用户所有项目（云开发自动根据 _openid 过滤）
  async getUserProjects() {
    try {
      const db = cloud.getDB()
      const res = await db.collection(COLLECTION_NAME)
        .where({
          deleted: false
        })
        .orderBy('updatedAt', 'desc')
        .get()
      
      return {
        success: true,
        data: res.data
      }
    } catch (error) {
      console.error('获取项目列表失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  },

  // 创建项目（云开发自动添加 _openid）
  async createProject(projectData) {
    try {
      const db = cloud.getDB()
      const now = Date.now()
      
      const res = await db.collection(COLLECTION_NAME).add({
        data: {
          ...projectData,
          createdAt: now,
          updatedAt: now,
          deleted: false
        }
      })
      
      return {
        success: true,
        id: res._id
      }
    } catch (error) {
      console.error('创建项目失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  },

  // 更新项目
  async updateProject(projectId, updateData) {
    try {
      const db = cloud.getDB()
      
      const res = await db.collection(COLLECTION_NAME).doc(projectId).update({
        data: {
          ...updateData,
          updatedAt: Date.now()
        }
      })
      
      return {
        success: true,
        stats: res.stats
      }
    } catch (error) {
      console.error('更新项目失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  },

  // 删除项目（软删除）
  async deleteProject(projectId) {
    try {
      const db = cloud.getDB()
      
      const res = await db.collection(COLLECTION_NAME).doc(projectId).update({
        data: {
          deleted: true,
          deletedAt: Date.now()
        }
      })
      
      return {
        success: true,
        stats: res.stats
      }
    } catch (error) {
      console.error('删除项目失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  },

  // 获取单个项目详情
  async getProject(projectId) {
    try {
      const db = cloud.getDB()
      const res = await db.collection(COLLECTION_NAME).doc(projectId).get()
      
      return {
        success: true,
        data: res.data[0]
      }
    } catch (error) {
      console.error('获取项目详情失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  },

  // 上传项目缩略图
  async uploadThumbnail(projectId, tempFilePath) {
    try {
      const cloudPath = `projects/${projectId}/thumbnail_${Date.now()}.png`
      const uploadRes = await cloud.uploadFile(cloudPath, tempFilePath)
      
      if (!uploadRes.success) {
        return uploadRes
      }
      
      // 更新项目记录
      await this.updateProject(projectId, {
        thumbnailFileID: uploadRes.fileID
      })
      
      return {
        success: true,
        fileID: uploadRes.fileID
      }
    } catch (error) {
      console.error('上传缩略图失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  }
}

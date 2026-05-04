// 云开发工具类
class CloudService {
  constructor() {
    this.isInitialized = false
  }

  // 初始化云开发
  init() {
    if (this.isInitialized) return
    
    // #ifdef MP-WEIXIN
    if (!wx.cloud) {
      console.error('请使用 2.2.3 或以上的基础库以使用云能力')
      return
    }
    
    wx.cloud.init({
      env: 'cloud1-8gyb5ui915c88712', // 云环境 ID
      traceUser: true
    })
    
    this.db = wx.cloud.database()
    this._ = this.db.command
    this.isInitialized = true
    // #endif
  }

  // 获取数据库实例
  getDB() {
    if (!this.isInitialized) {
      this.init()
    }
    // #ifdef MP-WEIXIN
    return wx.cloud.database()
    // #endif
    
    // #ifndef MP-WEIXIN
    return null
    // #endif
  }

  // 获取集合
  collection(name) {
    return this.getDB().collection(name)
  }

  // 调用云函数
  async callFunction(name, data = {}) {
    try {
      // #ifdef MP-WEIXIN
      if (!this.isInitialized) {
        this.init()
      }
      
      const res = await wx.cloud.callFunction({
        name,
        data
      })
      
      return {
        success: true,
        data: res.result
      }
      // #endif
      
      // #ifndef MP-WEIXIN
      return {
        success: false,
        error: '仅支持微信小程序'
      }
      // #endif
    } catch (error) {
      console.error('云函数调用失败:', error)
      return {
        success: false,
        error: error.message || error.errMsg || '调用失败'
      }
    }
  }

  // 上传文件到云存储
  async uploadFile(cloudPath, filePath) {
    try {
      // #ifdef MP-WEIXIN
      const res = await wx.cloud.uploadFile({
        cloudPath,
        filePath
      })
      return {
        success: true,
        fileID: res.fileID
      }
      // #endif
      
      // #ifndef MP-WEIXIN
      return {
        success: false,
        error: '仅支持微信小程序'
      }
      // #endif
    } catch (error) {
      console.error('文件上传失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  }

  // 下载文件
  async downloadFile(fileID) {
    try {
      // #ifdef MP-WEIXIN
      const res = await wx.cloud.downloadFile({
        fileID
      })
      return {
        success: true,
        tempFilePath: res.tempFilePath
      }
      // #endif
      
      // #ifndef MP-WEIXIN
      return {
        success: false,
        error: '仅支持微信小程序'
      }
      // #endif
    } catch (error) {
      console.error('文件下载失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  }

  // 删除文件
  async deleteFile(fileIDs) {
    try {
      // #ifdef MP-WEIXIN
      const res = await wx.cloud.deleteFile({
        fileList: Array.isArray(fileIDs) ? fileIDs : [fileIDs]
      })
      return {
        success: true,
        data: res.fileList
      }
      // #endif
      
      // #ifndef MP-WEIXIN
      return {
        success: false,
        error: '仅支持微信小程序'
      }
      // #endif
    } catch (error) {
      console.error('文件删除失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  }

  // 获取临时文件链接
  async getTempFileURL(fileIDs) {
    try {
      // #ifdef MP-WEIXIN
      const res = await wx.cloud.getTempFileURL({
        fileList: Array.isArray(fileIDs) ? fileIDs : [fileIDs]
      })
      return {
        success: true,
        fileList: res.fileList
      }
      // #endif
      
      // #ifndef MP-WEIXIN
      return {
        success: false,
        error: '仅支持微信小程序'
      }
      // #endif
    } catch (error) {
      console.error('获取临时链接失败:', error)
      return {
        success: false,
        error: error.message
      }
    }
  }
}

// 导出单例
export default new CloudService()

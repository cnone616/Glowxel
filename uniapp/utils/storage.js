/**
 * 统一的存储工具类
 * 封装 uni.storage API
 */

export const storage = {
  // 同步设置
  set(key, value) {
    try {
      uni.setStorageSync(key, JSON.stringify(value))
      return true
    } catch (e) {
      console.error('存储失败:', e)
      return false
    }
  },
  
  // 同步获取
  get(key) {
    try {
      const value = uni.getStorageSync(key)
      return value ? JSON.parse(value) : null
    } catch (e) {
      console.error('读取失败:', e)
      return null
    }
  },
  
  // 同步删除
  remove(key) {
    try {
      uni.removeStorageSync(key)
      return true
    } catch (e) {
      console.error('删除失败:', e)
      return false
    }
  },
  
  // 清空所有
  clear() {
    try {
      uni.clearStorageSync()
      return true
    } catch (e) {
      console.error('清空失败:', e)
      return false
    }
  }
}

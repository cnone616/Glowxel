/**
 * 用户模块 API
 */
import { get, post, put } from './request.js'

// 微信登录
export async function login(code) {
  return post('/api/user/login', { code }, { auth: false })
}

// 获取当前用户信息
export async function getProfile() {
  return get('/api/user/profile')
}

// 更新个人资料
export async function updateProfile(data) {
  return put('/api/user/profile', data)
}

// 获取他人主页
export async function getUserById(userId) {
  return get(`/api/user/${userId}`)
}

// 获取用户作品列表
export async function getUserArtworks(userId, page = 1, limit = 20) {
  return get(`/api/user/${userId}/artworks`, { page, limit })
}

// 获取用户统计
export async function getUserStats() {
  return get('/api/user/stats')
}

// 获取推荐用户
export async function getRecommendedUsers(limit = 8) {
  return get('/api/user/recommended', { limit })
}

// 搜索用户
export async function searchUsers(keyword) {
  return get('/api/user/search', { keyword })
}

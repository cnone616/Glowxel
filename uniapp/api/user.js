/**
 * 用户模块 API
 */
import { get, post, put, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 微信登录
export async function login(code) {
  if (isMockMode()) {
    return { success: true, data: { token: 'mock_token', user: MockAPI.users.getCurrent() } }
  }
  return post('/api/user/login', { code }, { auth: false })
}

// 获取当前用户信息
export async function getProfile() {
  if (isMockMode()) {
    return { success: true, data: MockAPI.users.getCurrent() }
  }
  return get('/api/user/profile')
}

// 更新个人资料
export async function updateProfile(data) {
  if (isMockMode()) {
    const user = MockAPI.users.getCurrent()
    Object.assign(user, data)
    return { success: true, data: user }
  }
  return put('/api/user/profile', data)
}

// 获取他人主页
export async function getUserById(userId) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.users.getById(userId) }
  }
  return get(`/api/user/${userId}`)
}

// 获取用户作品列表
export async function getUserArtworks(userId, page = 1, limit = 20) {
  if (isMockMode()) {
    const artworks = MockAPI.artworks.getByUserId(userId)
    return { success: true, data: { list: artworks, total: artworks.length } }
  }
  return get(`/api/user/${userId}/artworks`, { page, limit })
}

// 获取用户统计
export async function getUserStats() {
  if (isMockMode()) {
    const user = MockAPI.users.getCurrent()
    return { success: true, data: user.stats || {} }
  }
  return get('/api/user/stats')
}

// 获取推荐用户
export async function getRecommendedUsers(limit = 8) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.users.getRecommended(limit) }
  }
  return get('/api/user/recommended', { limit })
}

// 搜索用户
export async function searchUsers(keyword) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.users.search(keyword) }
  }
  return get('/api/user/search', { keyword })
}


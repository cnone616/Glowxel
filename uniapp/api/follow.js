/**
 * 关注模块 API
 */
import { get, post, del, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 关注用户
export async function followUser(userId) {
  if (isMockMode()) {
    const result = MockAPI.users.toggleFollow(userId)
    return { success: true, data: result }
  }
  return post(`/api/follow/${userId}`)
}

// 取消关注
export async function unfollowUser(userId) {
  if (isMockMode()) {
    const result = MockAPI.users.toggleFollow(userId)
    return { success: true, data: result }
  }
  return del(`/api/follow/${userId}`)
}

// 切换关注状态
export async function toggleFollow(userId) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.users.toggleFollow(userId) }
  }
  return post(`/api/follow/${userId}/toggle`)
}

// 获取关注列表
export async function getFollowing(userId, page = 1, limit = 20) {
  if (isMockMode()) {
    const list = MockAPI.users.getFollowing(userId, page, limit)
    return { success: true, data: { list, total: list.length } }
  }
  return get(`/api/follow/${userId}/following`, { page, limit })
}

// 获取粉丝列表
export async function getFollowers(userId, page = 1, limit = 20) {
  if (isMockMode()) {
    const list = MockAPI.users.getFollowers(userId, page, limit)
    return { success: true, data: { list, total: list.length } }
  }
  return get(`/api/follow/${userId}/followers`, { page, limit })
}


/**
 * 关注模块 API
 */
import { get, post, del } from './request.js'

export async function followUser(userId) {
  return post(`/api/follow/${userId}`)
}

export async function unfollowUser(userId) {
  return del(`/api/follow/${userId}`)
}

export async function toggleFollow(userId) {
  return post(`/api/follow/${userId}/toggle`)
}

export async function getFollowing(userId, page = 1, limit = 20) {
  return get(`/api/follow/${userId}/following`, { page, limit })
}

export async function getFollowers(userId, page = 1, limit = 20) {
  return get(`/api/follow/${userId}/followers`, { page, limit })
}

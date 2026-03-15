/**
 * 点赞模块 API
 */
import { post, del, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 点赞作品
export async function likeArtwork(artworkId) {
  if (isMockMode()) {
    return MockAPI.likes.like(artworkId)
  }
  return post(`/api/like/${artworkId}`)
}

// 取消点赞
export async function unlikeArtwork(artworkId) {
  if (isMockMode()) {
    return MockAPI.likes.unlike(artworkId)
  }
  return del(`/api/like/${artworkId}`)
}

// 检查是否已点赞
export async function isLiked(artworkId) {
  if (isMockMode()) {
    return MockAPI.likes.isLiked(artworkId)
  }
  const res = await import('./request.js').then(m => m.get(`/api/like/${artworkId}/check`))
  return res.success ? res.data : false
}
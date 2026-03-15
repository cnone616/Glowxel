/**
 * 评论模块 API
 */
import { get, post, del, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 获取评论列表
export async function getComments(artworkId, page = 1, limit = 20) {
  if (isMockMode()) {
    const list = MockAPI.comments.getByArtworkId(artworkId)
    return { success: true, data: { list, total: list.length } }
  }
  return get(`/api/comment/${artworkId}`, { page, limit })
}

// 发表评论
export async function addComment(artworkId, content, replyTo = null) {
  if (isMockMode()) {
    const comment = MockAPI.comments.add({ artworkId, content, replyTo })
    return { success: true, data: comment }
  }
  return post(`/api/comment/${artworkId}`, { content, replyTo })
}

// 删除评论
export async function deleteComment(commentId) {
  if (isMockMode()) {
    MockAPI.comments.delete(commentId)
    return { success: true }
  }
  return del(`/api/comment/${commentId}`)
}

// 评论点赞
export async function likeComment(commentId) {
  if (isMockMode()) {
    MockAPI.comments.like(commentId)
    return { success: true }
  }
  return post(`/api/comment/${commentId}/like`)
}


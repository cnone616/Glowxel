/**
 * 评论模块 API
 */
import { get, post, del } from './request.js'

export async function getComments(artworkId, page = 1, limit = 20) {
  return get(`/api/comment/${artworkId}`, { page, limit })
}

export async function addComment(artworkId, content, replyTo = null) {
  return post(`/api/comment/${artworkId}`, { content, replyTo })
}

export async function deleteComment(commentId) {
  return del(`/api/comment/${commentId}`)
}

export async function likeComment(commentId) {
  return post(`/api/comment/${commentId}/like`)
}

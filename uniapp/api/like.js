/**
 * 点赞模块 API
 */
import { post, del, get } from './request.js'

export async function likeArtwork(artworkId) {
  return post(`/api/like/${artworkId}`)
}

export async function unlikeArtwork(artworkId) {
  return del(`/api/like/${artworkId}`)
}

export async function isLiked(artworkId) {
  const res = await get(`/api/like/${artworkId}/check`)
  return res.success ? res.data : false
}

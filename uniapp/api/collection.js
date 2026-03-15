/**
 * 收藏模块 API
 */
import { get, post, del } from './request.js'

export async function collectArtwork(artworkId) {
  return post(`/api/collect/${artworkId}`)
}

export async function uncollectArtwork(artworkId) {
  return del(`/api/collect/${artworkId}`)
}

export async function isCollected(artworkId) {
  const res = await get(`/api/collect/${artworkId}/check`)
  return res.success ? res.data : false
}

export async function getMyCollections(page = 1, limit = 20) {
  return get('/api/collect/list', { page, limit })
}

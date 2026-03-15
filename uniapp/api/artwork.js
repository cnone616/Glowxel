/**
 * 作品模块 API
 */
import { get, post, del } from './request.js'

export async function getArtworks(params = {}) {
  const { page = 1, limit = 20, sort = 'latest', tag } = params
  return get('/api/artwork/list', { page, limit, sort, tag })
}

export async function getArtworkById(id) {
  return get(`/api/artwork/${id}`)
}

export async function getPopularArtworks(page = 1, limit = 20) {
  return get('/api/artwork/popular', { page, limit })
}

export async function getLatestArtworks(page = 1, limit = 20) {
  return get('/api/artwork/latest', { page, limit })
}

export async function getFollowingArtworks(page = 1, limit = 20) {
  return get('/api/artwork/following', { page, limit })
}

export async function getRelatedArtworks(artworkId) {
  return get(`/api/artwork/${artworkId}/related`)
}

export async function searchArtworks(keyword, tag, page = 1) {
  return get('/api/artwork/search', { keyword, tag, page })
}

export async function searchByTag(tag) {
  return get('/api/artwork/search', { tag })
}

export async function getArtworkPixels(artworkId) {
  return get(`/api/artwork/${artworkId}/pixels`)
}

export async function publishArtwork(data) {
  return post('/api/artwork/publish', data)
}

export async function deleteArtwork(artworkId) {
  return del(`/api/artwork/${artworkId}`)
}

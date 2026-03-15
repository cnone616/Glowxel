/**
 * 作品模块 API
 */
import { get, post, del, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 获取作品列表
export async function getArtworks(params = {}) {
  const { page = 1, limit = 20, sort = 'latest', tag } = params
  if (isMockMode()) {
    let list
    switch (sort) {
      case 'popular': list = MockAPI.artworks.getPopular(); break
      case 'latest': list = MockAPI.artworks.getLatest(); break
      default: list = MockAPI.artworks.getAll()
    }
    if (tag) list = list.filter(a => a.tags && a.tags.includes(tag))
    return { success: true, data: { list, total: list.length } }
  }
  return get('/api/artwork/list', { page, limit, sort, tag })
}

// 获取作品详情
export async function getArtworkById(id) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.artworks.getById(id) }
  }
  return get(`/api/artwork/${id}`)
}

// 获取热门作品
export async function getPopularArtworks(page = 1, limit = 20) {
  if (isMockMode()) {
    return { success: true, data: { list: MockAPI.artworks.getPopular(), total: 20 } }
  }
  return get('/api/artwork/popular', { page, limit })
}

// 获取最新作品
export async function getLatestArtworks(page = 1, limit = 20) {
  if (isMockMode()) {
    return { success: true, data: { list: MockAPI.artworks.getLatest(), total: 20 } }
  }
  return get('/api/artwork/latest', { page, limit })
}

// 获取关注用户的作品
export async function getFollowingArtworks(page = 1, limit = 20) {
  if (isMockMode()) {
    return { success: true, data: { list: MockAPI.artworks.getPopular().slice(0, 10), total: 10 } }
  }
  return get('/api/artwork/following', { page, limit })
}

// 获取相关推荐
export async function getRelatedArtworks(artworkId) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.artworks.getRelated(artworkId) }
  }
  return get(`/api/artwork/${artworkId}/related`)
}

// 搜索作品
export async function searchArtworks(keyword, tag, page = 1) {
  if (isMockMode()) {
    const list = tag ? MockAPI.artworks.searchByTag(tag) : MockAPI.artworks.getAll()
    return { success: true, data: { list, total: list.length } }
  }
  return get('/api/artwork/search', { keyword, tag, page })
}

// 按标签搜索
export async function searchByTag(tag) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.artworks.searchByTag(tag) }
  }
  return get('/api/artwork/search', { tag })
}

// 获取作品像素数据（收藏时用）
export async function getArtworkPixels(artworkId) {
  if (isMockMode()) {
    // Mock 模式暂无像素数据
    return { success: true, data: { pixels: [], palette: [], width: 0, height: 0 } }
  }
  return get(`/api/artwork/${artworkId}/pixels`)
}

// 发布作品
export async function publishArtwork(data) {
  if (isMockMode()) {
    return { success: true, data: { artworkId: Date.now() } }
  }
  return post('/api/artwork/publish', data)
}

// 删除作品
export async function deleteArtwork(artworkId) {
  if (isMockMode()) {
    return { success: true }
  }
  return del(`/api/artwork/${artworkId}`)
}


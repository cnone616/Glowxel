/**
 * 收藏模块 API
 */
import { get, post, del, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 收藏作品
export async function collectArtwork(artworkId) {
  if (isMockMode()) {
    return MockAPI.collections.collect(artworkId)
  }
  return post(`/api/collect/${artworkId}`)
}

// 取消收藏
export async function uncollectArtwork(artworkId) {
  if (isMockMode()) {
    return MockAPI.collections.uncollect(artworkId)
  }
  return del(`/api/collect/${artworkId}`)
}

// 检查是否已收藏
export async function isCollected(artworkId) {
  if (isMockMode()) {
    return MockAPI.collections.isCollected(artworkId)
  }
  const res = await get(`/api/collect/${artworkId}/check`)
  return res.success ? res.data : false
}

// 获取我的收藏列表
export async function getMyCollections(page = 1, limit = 20) {
  if (isMockMode()) {
    // Mock 模式返回热门作品作为收藏
    const list = MockAPI.artworks.getPopular().slice(0, 6)
    return { success: true, data: { list, total: list.length } }
  }
  return get('/api/collect/list', { page, limit })
}


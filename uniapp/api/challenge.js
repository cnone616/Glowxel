/**
 * 挑战模块 API
 */
import { get, post, isMockMode } from './request.js'
import { MockAPI } from '../data/mock/index.js'

// 获取挑战列表
export async function getChallenges(params = {}) {
  const { status, page = 1, limit = 20 } = params
  if (isMockMode()) {
    let list
    if (status) {
      list = MockAPI.challenges.getByStatus(status)
    } else {
      list = MockAPI.challenges.getAll()
    }
    return { success: true, data: { list, total: list.length } }
  }
  return get('/api/challenge/list', { status, page, limit })
}

// 获取挑战详情
export async function getChallengeById(id) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.getById(id) }
  }
  return get(`/api/challenge/${id}`)
}

// 获取进行中的挑战
export async function getActiveChallenges() {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.getActive() }
  }
  return get('/api/challenge/list', { status: 'active' })
}

// 获取即将开始的挑战
export async function getUpcomingChallenges() {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.getUpcoming() }
  }
  return get('/api/challenge/list', { status: 'upcoming' })
}

// 获取热门挑战
export async function getPopularChallenges() {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.getPopular() }
  }
  return get('/api/challenge/popular')
}

// 参加挑战
export async function joinChallenge(challengeId) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.join(challengeId) }
  }
  return post(`/api/challenge/${challengeId}/join`)
}

// 提交作品到挑战
export async function submitToChallenge(challengeId, artworkId) {
  if (isMockMode()) {
    return { success: true, data: MockAPI.challenges.submit(challengeId, artworkId) }
  }
  return post(`/api/challenge/${challengeId}/submit`, { artworkId })
}


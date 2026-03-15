/**
 * 挑战模块 API
 */
import { get, post } from './request.js'

export async function getChallenges(params = {}) {
  const { status, page = 1, limit = 20 } = params
  return get('/api/challenge/list', { status, page, limit })
}

export async function getChallengeById(id) {
  return get(`/api/challenge/${id}`)
}

export async function getActiveChallenges() {
  return get('/api/challenge/list', { status: 'active' })
}

export async function getUpcomingChallenges() {
  return get('/api/challenge/list', { status: 'upcoming' })
}

export async function getPopularChallenges() {
  return get('/api/challenge/popular')
}

export async function joinChallenge(challengeId) {
  return post(`/api/challenge/${challengeId}/join`)
}

export async function submitToChallenge(challengeId, artworkId) {
  return post(`/api/challenge/${challengeId}/submit`, { artworkId })
}

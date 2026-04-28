import { get, post } from './request.js'

export const getList = (params) => get('/api/challenge/list', params)
export const getPopular = (limit = 10) => get('/api/challenge/popular', { limit })
export const getDetail = (id) => get(`/api/challenge/${id}`)
export const join = (id) => post(`/api/challenge/${id}/join`)
export const submit = (id, artworkId) => post(`/api/challenge/${id}/submit`, { artworkId })
export const getSubmissions = (id, params) => get(`/api/challenge/${id}/submissions`, params)

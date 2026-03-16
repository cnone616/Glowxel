import { get, post, del } from './request.js'

export const getList = (params) => get('/api/artwork/list', params)
export const getDetail = (id) => get(`/api/artwork/${id}`)
export const getPopular = (limit = 20) => get('/api/artwork/popular', { limit })
export const getLatest = (limit = 20) => get('/api/artwork/latest', { limit })
export const search = (params) => get('/api/artwork/search', params)
export const publish = (data) => post('/api/artwork/publish', data)
export const remove = (id) => del(`/api/artwork/${id}`)
export const getPixels = (id) => get(`/api/artwork/${id}/pixels`)
export const getRelated = (id) => get(`/api/artwork/${id}/related`)
export const getUserArtworks = (userId, params) => get(`/api/user/${userId}/artworks`, params)


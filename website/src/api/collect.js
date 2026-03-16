import { post, del, get } from './request.js'

export const collect = (artworkId) => post(`/api/collect/${artworkId}`)
export const uncollect = (artworkId) => del(`/api/collect/${artworkId}`)
export const check = (artworkId) => get(`/api/collect/${artworkId}/check`)
export const getMyCollections = (params) => get('/api/collect/list', params)


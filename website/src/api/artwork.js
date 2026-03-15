import { get, post, del } from './request.js'

export const getList = (params) => get('/api/artwork/list', params)
export const getDetail = (id) => get(`/api/artwork/${id}`)
export const publish = (data) => post('/api/artwork/publish', data)
export const remove = (id) => del(`/api/artwork/${id}`)
export const getUserArtworks = (userId, params) => get(`/api/artwork/user/${userId}`, params)


import { get, post, del } from './request.js'

export const getList = (artworkId, params) => get(`/api/comment/${artworkId}`, params)
export const add = (data) => post('/api/comment', data)
export const remove = (id) => del(`/api/comment/${id}`)


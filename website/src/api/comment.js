import { del, get, post } from './request.js'

export const getList = (artworkId, params) => get(`/api/comment/${artworkId}`, params)
export const add = (artworkId, content) => post(`/api/comment/${artworkId}`, { content })
export const remove = (id) => del(`/api/comment/${id}`)

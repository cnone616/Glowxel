import { post, del, get } from './request.js'

export const like = (artworkId) => post(`/api/like/${artworkId}`)
export const unlike = (artworkId) => del(`/api/like/${artworkId}`)
export const check = (artworkId) => get(`/api/like/${artworkId}/check`)


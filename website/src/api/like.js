import { post, del } from './request.js'

export const like = (artworkId) => post(`/api/like/${artworkId}`)
export const unlike = (artworkId) => del(`/api/like/${artworkId}`)


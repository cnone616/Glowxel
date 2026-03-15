import { post, del } from './request.js'

export const collect = (artworkId) => post(`/api/collect/${artworkId}`)
export const uncollect = (artworkId) => del(`/api/collect/${artworkId}`)


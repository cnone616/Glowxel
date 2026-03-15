import { post, del, get } from './request.js'

export const follow = (userId) => post(`/api/follow/${userId}`)
export const unfollow = (userId) => del(`/api/follow/${userId}`)
export const getFollowers = (userId, params) => get(`/api/follow/${userId}/followers`, params)
export const getFollowing = (userId, params) => get(`/api/follow/${userId}/following`, params)


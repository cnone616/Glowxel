import { get, post } from './request.js'

export const getList = (params) => get('/api/challenge/list', params)
export const getDetail = (id) => get(`/api/challenge/${id}`)
export const join = (id) => post(`/api/challenge/${id}/join`)


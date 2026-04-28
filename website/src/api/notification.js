import { get, put } from './request.js'

export const getList = (params) => get('/api/notification/list', params)
export const readAll = () => put('/api/notification/read-all')
export const read = (id) => put(`/api/notification/${id}/read`)
export const getUnreadCount = () => get('/api/notification/unread-count')

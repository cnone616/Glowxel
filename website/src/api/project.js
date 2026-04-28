import { del, get, post } from './request.js'

export const getList = (params) => get('/api/project/list', params)
export const getDetail = (id) => get(`/api/project/${id}`)
export const sync = (project, pixels) => post('/api/project/sync', { project, pixels })
export const remove = (id) => del(`/api/project/${id}`)
export const getSyncStatus = () => get('/api/project/sync-status')
export const batchSync = (projects) => post('/api/project/batch-sync', { projects })

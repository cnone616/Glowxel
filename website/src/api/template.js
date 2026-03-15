import { get } from './request.js'

export const getList = (params) => get('/api/template/list', params)
export const getDetail = (id) => get(`/api/template/${id}`)


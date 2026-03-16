import { get, post } from './request.js'

export const getList = (params) => get('/api/template/list', params)
export const getDetail = (id) => get(`/api/template/${id}`)
export const getPopular = (limit = 10) => get('/api/template/popular', { limit })
export const getCategories = () => get('/api/template/categories')
export const search = (keyword) => get('/api/template/search', { keyword })
export const use = (id) => post(`/api/template/${id}/use`)


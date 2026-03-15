/**
 * 模板模块 API
 */
import { get, post } from './request.js'

export async function getTemplates(params = {}) {
  const { category, difficulty, page = 1, limit = 20 } = params
  return get('/api/template/list', { category, difficulty, page, limit })
}

export async function getTemplateById(id) {
  return get(`/api/template/${id}`)
}

export async function getPopularTemplates(limit = 10) {
  return get('/api/template/popular', { limit })
}

export async function getCategories() {
  return get('/api/template/categories')
}

export async function searchTemplates(keyword) {
  return get('/api/template/search', { keyword })
}

export async function useTemplate(templateId) {
  return post(`/api/template/${templateId}/use`)
}
